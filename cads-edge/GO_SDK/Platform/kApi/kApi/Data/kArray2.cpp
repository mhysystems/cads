/** 
 * @file    kArray2.cpp
 *
 * @internal
 * Copyright (C) 2005-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#include <kApi/Data/kArray2.h>
#include <kApi/Data/kCollection.h>
#include <kApi/Io/kSerializer.h>

kBeginClassEx(k, kArray2) 
    
    //serialization versions
    kAddPrivateVersion(kArray2, "kdat5", "5.0.0.0", "25-1", WriteDat5V1, ReadDat5V1)
    kAddPrivateVersion(kArray2, "kdat6", "5.7.1.0", "kArray2-0", WriteDat6V0, ReadDat6V0)

    //virtual methods
    kAddPrivateVMethod(kArray2, kObject, VRelease)
    kAddPrivateVMethod(kArray2, kObject, VDisposeItems)
    kAddPrivateVMethod(kArray2, kObject, VInitClone)
    kAddPrivateVMethod(kArray2, kObject, VSize)
    kAddPrivateVMethod(kArray2, kObject, VHasForeignData)

    //collection interface 
    kAddInterface(kArray2, kCollection)
    kAddPrivateIVMethod(kArray2, kCollection, VGetIterator, GetIterator)
    kAddIVMethod(kArray2, kCollection, VItemType, ItemType)
    kAddIVMethod(kArray2, kCollection, VCount, Count)
    kAddPrivateIVMethod(kArray2, kCollection, VHasNext, HasNext)
    kAddPrivateIVMethod(kArray2, kCollection, VNext, Next)

kEndClassEx() 


kFx(kStatus) kArray2_ConstructEx(kArray2* array, kType itemType, kSize length0, kSize length1, kAlloc allocator, kAlloc dataAllocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kAlloc dataAlloc = kAlloc_Fallback(dataAllocator);
    kStatus status; 

    kCheck(kAlloc_GetObject(alloc, kTypeOf(kArray2), array)); 

    if (!kSuccess(status = xkArray2_Init(*array, kTypeOf(kArray2), itemType, length0, length1, alloc, dataAlloc)))
    {
        kAlloc_FreeRef(alloc, array); 
    }

    return status; 
} 

kFx(kStatus) kArray2_Construct(kArray2* array, kType itemType, kSize length0, kSize length1, kAlloc allocator)
{
    return kArray2_ConstructEx(array, itemType, length0, length1, allocator, allocator);
} 

kFx(kStatus) xkArray2_Init(kArray2 array, kType classType, kType itemType, kSize length0, kSize length1, kAlloc alloc, kAlloc dataAlloc)
{
    kObjR(kArray2, array);
    kType resolvedItemType = kIsNull(itemType) ? kTypeOf(kVoid) : itemType; 
    kStatus status = kOK; 

    kCheckArgs(!kAlloc_IsForeign(dataAlloc) || kType_IsValue(resolvedItemType));

    kCheck(kObject_Init(array, classType, alloc)); 

    obj->dataAlloc = dataAlloc;
    obj->itemType = resolvedItemType;
    obj->itemSize = kType_Size(obj->itemType); 
    obj->allocSize = 0; 
    obj->items = kNULL;
    obj->length[0] = 0; 
    obj->length[1] = 0; 
    obj->isAttached = kFALSE; 

    kTry
    {
        kTest(xkArray2_Realloc(array, length0, length1)); 
    }
    kCatch(&status)
    {
        xkArray2_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray2_VInitClone(kArray2 array, kArray2 source, kAlloc allocator)
{
    kObjR(kArray2, array);
    kObjN(kArray2, sourceObj, source);
    kStatus status = kOK; 
    kSize count = kArray2_Count(source); 

    kCheck(kObject_Init(array, kObject_Type(source), allocator));     

    obj->dataAlloc = allocator;
    obj->itemType = sourceObj->itemType; 
    obj->itemSize = sourceObj->itemSize; 
    obj->allocSize = 0; 
    obj->items = kNULL; 
    obj->length[0] = 0; 
    obj->length[1] = 0; 
    obj->isAttached = kFALSE; 

    kTry
    {
        kTest(xkArray2_Realloc(array, sourceObj->length[0], sourceObj->length[1])); 

        kTest(kCloneItemsEx(obj->itemType, obj->items, sourceObj->items, count, obj->dataAlloc, sourceObj->dataAlloc)); 
    }
    kCatch(&status)
    {
        xkArray2_VDisposeItems(array); 
        xkArray2_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray2_VRelease(kArray2 array)
{
    kObj(kArray2, array);

    if (!obj->isAttached)
    {
        kCheck(kAlloc_Free(obj->dataAlloc, obj->items)); 
    }

    kCheck(kObject_VRelease(array)); 

    return kOK; 
}

kFx(kStatus) xkArray2_VDisposeItems(kArray2 array)
{
    kObj(kArray2, array);

    kCheck(kDisposeItems(obj->itemType, obj->items, kArray2_Count(array))); 

    return kOK; 
}

kFx(kSize) xkArray2_VSize(kArray2 array)
{
    kObj(kArray2, array);
    kSize dataSize = (!obj->isAttached) ? obj->allocSize : kArray2_DataSize(array); 
    kSize size = sizeof(kArray2Class) + dataSize; 

    size += kMeasureItems(obj->itemType, obj->items, kArray2_Count(array)); 

    return size; 
}

kFx(kBool) xkArray2_VHasForeignData(kArray2 array)
{
    kObj(kArray2, array);

    return kAlloc_IsForeign(obj->dataAlloc) || kHasForeignData(obj->itemType, obj->items, kArray2_Count(array));
}

kFx(kStatus) xkArray2_WriteDat5V1(kArray2 array, kSerializer serializer)
{
    kObj(kArray2, array);
    kTypeVersion itemVersion; 
    kSize count = kArray2_Count(array); 

    kCheckState(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kSerializer_WriteSize(serializer, obj->length[0])); 
    kCheck(kSerializer_WriteSize(serializer, obj->length[1])); 
    kCheck(kSerializer_WriteSize(serializer, count)); 
    kCheck(kSerializer_WriteType(serializer, obj->itemType, &itemVersion));
    kCheck(kSerializer_WriteItems(serializer, obj->itemType, itemVersion, obj->items, count)); 

    return kOK; 
}

kFx(kStatus) xkArray2_ReadDat5V1(kArray2 array, kSerializer serializer, kAlloc allocator)
{
    kObjR(kArray2, array);
    kSize length0 = 0, length1 = 0, count = 0; 
    kTypeVersion itemVersion;
    kType itemType = kNULL;            
    kStatus status; 

    kCheck(kSerializer_ReadSize(serializer, &length0)); 
    kCheck(kSerializer_ReadSize(serializer, &length1)); 

    kCheck(kSerializer_ReadSize(serializer, &count));
    kCheck(kSerializer_ReadType(serializer, &itemType, &itemVersion)); 

    kCheck(xkArray2_Init(array, kTypeOf(kArray2), itemType, length0, length1, allocator, allocator)); 
  
    if (!kSuccess(status = kSerializer_ReadItems(serializer, itemType, itemVersion, obj->items, count)))
    {
        xkArray2_VDisposeItems(array); 
        xkArray2_VRelease(array); 
        return status;
    }

    return kOK; 
}

kFx(kStatus) xkArray2_WriteDat6V0(kArray2 array, kSerializer serializer)
{
    kObj(kArray2, array);
    kTypeVersion itemVersion; 

    kCheckState(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kSerializer_WriteType(serializer, obj->itemType, &itemVersion));
    kCheck(kSerializer_WriteSize(serializer, obj->length[0])); 
    kCheck(kSerializer_WriteSize(serializer, obj->length[1])); 
    kCheck(kSerializer_WriteItems(serializer, obj->itemType, itemVersion, obj->items, kArray2_Count(array))); 

    return kOK; 
}

kFx(kStatus) xkArray2_ReadDat6V0(kArray2 array, kSerializer serializer, kAlloc allocator)
{
    kObjR(kArray2, array);
    kType itemType = kNULL;            
    kTypeVersion itemVersion; 
    kSize length0 = 0, length1 = 0; 
    kStatus status; 

    kCheck(kSerializer_ReadType(serializer, &itemType, &itemVersion));
    kCheck(kSerializer_ReadSize(serializer, &length0)); 
    kCheck(kSerializer_ReadSize(serializer, &length1)); 

    kCheck(xkArray2_Init(array, kTypeOf(kArray2), itemType, length0, length1, allocator, allocator)); 

    kTry
    {
        kTest(kSerializer_ReadItems(serializer, itemType, itemVersion, obj->items, kArray2_Count(array))); 
    }
    kCatch(&status)
    {
        xkArray2_VDisposeItems(array); 
        xkArray2_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray2_Realloc(kArray2 array, kSize length0, kSize length1)
{
    kObj(kArray2, array);
    kSize newSize = kMax_(obj->allocSize, length0*length1*obj->itemSize); 

    if (newSize > obj->allocSize)
    {
        void* oldItems = obj->items; 
        void* newItems = kNULL; 

        kCheck(kAlloc_Get(obj->dataAlloc, newSize, &newItems));
        
        obj->items = newItems; 
        obj->allocSize = newSize; 

        kCheck(kAlloc_Free(obj->dataAlloc, oldItems));
    }
        
    if (kType_IsReference(obj->itemType))
    {
        kMemSet(obj->items, 0, length0*length1*obj->itemSize);  
    }

    obj->length[0] = length0; 
    obj->length[1] = length1; 
    
    return kOK; 
}

kFx(kStatus) kArray2_Allocate(kArray2 array, kType itemType, kSize length0, kSize length1)
{
    kObj(kArray2, array);

    if (obj->isAttached)
    {
        obj->items = kNULL; 
        obj->isAttached = kFALSE; 
    }
    
    obj->itemType = kIsNull(itemType) ? kTypeOf(kVoid) : itemType; 
    obj->itemSize = kType_Size(obj->itemType); 
    obj->length[0] = 0; 
    obj->length[1] = 0; 

    kCheck(xkArray2_Realloc(array, length0, length1)); 

    return kOK; 
}

kFx(kStatus) kArray2_Attach(kArray2 array, void* items, kType itemType, kSize length0, kSize length1)
{
    kObj(kArray2, array);

    if (!obj->isAttached)
    {
        kCheck(kAlloc_FreeRef(obj->dataAlloc, &obj->items));
    }
    
    obj->itemType = kIsNull(itemType) ? kTypeOf(kVoid) : itemType; 
    obj->itemSize = kType_Size(obj->itemType); 
    obj->allocSize = 0; 
    obj->items = items; 
    obj->length[0] = length0; 
    obj->length[1] = length1; 
    obj->isAttached = kTRUE; 

    return kOK; 
}

kFx(kStatus) kArray2_Assign(kArray2 array, kArray2 source)
{
    kObj(kArray2, array);
    kObjN(kArray2, sourceObj, source);

    kCheckArgs(!kAlloc_IsForeign(obj->dataAlloc) && !kAlloc_IsForeign(sourceObj->dataAlloc));

    kCheck(kArray2_Allocate(array, sourceObj->itemType, sourceObj->length[0], sourceObj->length[1])); 
    kCheck(kCopyItems(obj->itemType, obj->items, sourceObj->items, kArray2_Count(array))); 

    return kOK;   
}

kFx(kStatus) kArray2_Zero(kArray2 array)
{
    kObj(kArray2, array);

    kCheckArgs(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kZeroItems(obj->itemType, obj->items, kArray2_Count(array)));

    return kOK; 
}

kFx(kStatus) kArray2_SetItem(kArray2 array, kSize index0, kSize index1, const void* item)
{
    kObj(kArray2, array);

    kAssert(!kAlloc_IsForeign(obj->dataAlloc));
    kCheckArgs((index0 < obj->length[0]) && (index1 < obj->length[1])); 

    kValue_Import(obj->itemType, kArray2_DataAt(array, (kSSize)index0, (kSSize)index1), item); 

    return kOK; 
}

kFx(kStatus) kArray2_Item(kArray2 array, kSize index0, kSize index1, void* item)
{
    kObj(kArray2, array);

    kAssert(!kAlloc_IsForeign(obj->dataAlloc));
    kCheckArgs((index0 < obj->length[0]) && (index1 < obj->length[1])); 

    kItemCopy(item, kArray2_DataAt(array, (kSSize)index0, (kSSize)index1), obj->itemSize); 

    return kOK; 
}

kFx(kIterator) xkArray2_GetIterator(kArray2 array)
{
    kObj(kArray2, array);
    return obj->items; 
}

kFx(kBool) xkArray2_HasNext(kArray2 array, kIterator iterator)
{
    kObj(kArray2, array);
    void* end = (kByte*)obj->items + kArray2_DataSize(array); 

    return (iterator != end); 
}

kFx(void*) xkArray2_Next(kArray2 array, kIterator* iterator)
{
    kObj(kArray2, array);
    void* next = *iterator; 
   
    *iterator = (kByte*)*iterator + obj->itemSize; 

    return next; 
}

