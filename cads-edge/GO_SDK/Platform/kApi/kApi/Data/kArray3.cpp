/** 
 * @file    kArray3.cpp
 *
 * @internal
 * Copyright (C) 2006-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#include <kApi/Data/kArray3.h>
#include <kApi/Data/kCollection.h>
#include <kApi/Io/kSerializer.h>

kBeginClassEx(k, kArray3) 
    
    //serialization versions
    kAddPrivateVersion(kArray3, "kdat5", "5.0.0.0", "26-0", WriteDat5V0, ReadDat5V0)
    kAddPrivateVersion(kArray3, "kdat6", "5.7.1.0", "kArray3-0", WriteDat6V0, ReadDat6V0)

    //virtual methods
    kAddPrivateVMethod(kArray3, kObject, VRelease)
    kAddPrivateVMethod(kArray3, kObject, VDisposeItems)
    kAddPrivateVMethod(kArray3, kObject, VInitClone)
    kAddPrivateVMethod(kArray3, kObject, VSize)
    kAddPrivateVMethod(kArray3, kObject, VHasForeignData)

    //collection interface 
    kAddInterface(kArray3, kCollection)
    kAddPrivateIVMethod(kArray3, kCollection, VGetIterator, GetIterator)
    kAddIVMethod(kArray3, kCollection, VItemType, ItemType)
    kAddIVMethod(kArray3, kCollection, VCount, Count)
    kAddPrivateIVMethod(kArray3, kCollection, VHasNext, HasNext)
    kAddPrivateIVMethod(kArray3, kCollection, VNext, Next)

kEndClassEx() 

kFx(kStatus) kArray3_ConstructEx(kArray3* array, kType itemType, kSize length0, kSize length1, kSize length2,  kAlloc allocator, kAlloc dataAllocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kAlloc dataAlloc = kAlloc_Fallback(dataAllocator);
    kStatus status; 

    kCheck(kAlloc_GetObject(alloc, kTypeOf(kArray3), array)); 

    if (!kSuccess(status = xkArray3_Init(*array, kTypeOf(kArray3), itemType, length0, length1, length2, alloc, dataAlloc)))
    {
        kAlloc_FreeRef(alloc, array); 
    }

    return status; 
} 

kFx(kStatus) kArray3_Construct(kArray3* array, kType itemType, kSize length0, kSize length1, kSize length2, kAlloc allocator)
{
    return kArray3_ConstructEx(array, itemType, length0, length1, length2, allocator, allocator);
} 

kFx(kStatus) xkArray3_Init(kArray3 array, kType classType, kType itemType, kSize length0, kSize length1, kSize length2, kAlloc alloc, kAlloc dataAlloc)
{
    kObjR(kArray3, array);
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
    obj->length[2] = 0; 
    obj->isAttached = kFALSE; 

    kTry
    {
        kTest(xkArray3_Realloc(array, length0, length1, length2)); 
    }
    kCatch(&status)
    {
        xkArray3_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray3_VInitClone(kArray3 array, kArray3 source, kAlloc allocator)
{
    kObjR(kArray3, array);
    kObjN(kArray3, sourceObj, source);
    kStatus status = kOK; 

    kCheck(kObject_Init(array, kObject_Type(source), allocator));     

    obj->dataAlloc = allocator;
    obj->itemType = sourceObj->itemType; 
    obj->itemSize = sourceObj->itemSize; 
    obj->allocSize = 0; 
    obj->items = kNULL; 
    obj->length[0] = 0; 
    obj->length[1] = 0; 
    obj->length[2] = 0; 
    obj->isAttached = kFALSE; 

    kTry
    {
        kTest(xkArray3_Realloc(array, sourceObj->length[0], sourceObj->length[1], sourceObj->length[2])); 
        kTest(kCloneItemsEx(obj->itemType, obj->items, sourceObj->items, kArray3_Count(source), obj->dataAlloc, sourceObj->dataAlloc));  
    }
    kCatch(&status)
    {
        xkArray3_VDisposeItems(array); 
        xkArray3_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray3_VRelease(kArray3 array)
{
    kObj(kArray3, array);

    if (!obj->isAttached)
    {
        kCheck(kAlloc_Free(obj->dataAlloc, obj->items)); 
    }

    kCheck(kObject_VRelease(array)); 

    return kOK; 
}

kFx(kStatus) xkArray3_VDisposeItems(kArray3 array)
{
    kObj(kArray3, array);

    kCheck(kDisposeItems(obj->itemType, obj->items, kArray3_Count(array))); 

    return kOK; 
}

kFx(kSize) xkArray3_VSize(kArray3 array)
{
    kObj(kArray3, array);
    kSize dataSize = (!obj->isAttached) ? obj->allocSize : kArray3_DataSize(array); 
    kSize size = sizeof(kArray3Class) + dataSize; 

    size += kMeasureItems(obj->itemType, obj->items, kArray3_Count(array)); 

    return size; 
}

kFx(kBool) xkArray3_VHasForeignData(kArray3 array)
{
    kObj(kArray3, array);

    return kAlloc_IsForeign(obj->dataAlloc) || kHasForeignData(obj->itemType, obj->items, kArray3_Count(array));
}

kFx(kStatus) xkArray3_WriteDat5V0(kArray3 array, kSerializer serializer)
{
    kObj(kArray3, array);
    kTypeVersion itemVersion; 
    kSize count = kArray3_Count(array); 

    kCheckState(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kSerializer_WriteSize(serializer, obj->length[0])); 
    kCheck(kSerializer_WriteSize(serializer, obj->length[1])); 
    kCheck(kSerializer_WriteSize(serializer, obj->length[2])); 
    kCheck(kSerializer_WriteSize(serializer, count)); 
    kCheck(kSerializer_WriteType(serializer, obj->itemType, &itemVersion));
    kCheck(kSerializer_WriteItems(serializer, obj->itemType, itemVersion, obj->items, count)); 

    return kOK; 
}

kFx(kStatus) xkArray3_ReadDat5V0(kArray3 array, kSerializer serializer, kAlloc allocator)
{
    kObjR(kArray3, array);
    kSize length0 = 0, length1 = 0, length2 = 0; 
    k32u count = 0; 
    kTypeVersion itemVersion;
    kType itemType = kNULL;            
    kStatus status; 

    kCheck(kSerializer_ReadSize(serializer, &length0)); 
    kCheck(kSerializer_ReadSize(serializer, &length1)); 
    kCheck(kSerializer_ReadSize(serializer, &length2)); 

    kCheck(kSerializer_Read32u(serializer, &count));
    kCheck(kSerializer_ReadType(serializer, &itemType, &itemVersion)); 

    kCheck(xkArray3_Init(array, kTypeOf(kArray3), itemType, length0, length1, length2, allocator, allocator)); 

    if (!kSuccess(status = kSerializer_ReadItems(serializer, itemType, itemVersion, obj->items, count)))
    {
        xkArray3_VDisposeItems(array); 
        xkArray3_VRelease(array);
        return status;
    }

    return kOK; 
}

kFx(kStatus) xkArray3_WriteDat6V0(kArray3 array, kSerializer serializer)
{
    kObj(kArray3, array);
    kTypeVersion itemVersion; 

    kCheckState(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kSerializer_WriteType(serializer, obj->itemType, &itemVersion));
    kCheck(kSerializer_WriteSize(serializer, obj->length[0])); 
    kCheck(kSerializer_WriteSize(serializer, obj->length[1])); 
    kCheck(kSerializer_WriteSize(serializer, obj->length[2])); 
    kCheck(kSerializer_WriteItems(serializer, obj->itemType, itemVersion, obj->items, kArray3_Count(array))); 

    return kOK; 
}

kFx(kStatus) xkArray3_ReadDat6V0(kArray3 array, kSerializer serializer, kAlloc allocator)
{
    kObjR(kArray3, array);
    kType itemType = kNULL;            
    kTypeVersion itemVersion; 
    kSize length0 = 0, length1 = 0, length2 = 0; 
    kStatus status; 

    kCheck(kSerializer_ReadType(serializer, &itemType, &itemVersion));
    kCheck(kSerializer_ReadSize(serializer, &length0)); 
    kCheck(kSerializer_ReadSize(serializer, &length1)); 
    kCheck(kSerializer_ReadSize(serializer, &length2)); 

    kCheck(xkArray3_Init(array, kTypeOf(kArray3), itemType, length0, length1, length2, allocator, allocator)); 

    kTry
    {
        kTest(kSerializer_ReadItems(serializer, itemType, itemVersion, obj->items, kArray3_Count(array))); 
    }
    kCatch(&status)
    {
        xkArray3_VDisposeItems(array); 
        xkArray3_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray3_Realloc(kArray3 array, kSize length0, kSize length1, kSize length2)
{
    kObj(kArray3, array);
    kSize newSize = kMax_(obj->allocSize, length0*length1*length2*obj->itemSize); 

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
        kMemSet(obj->items, 0, length0*length1*length2*obj->itemSize);  
    }

    obj->length[0] = length0; 
    obj->length[1] = length1; 
    obj->length[2] = length2; 
    
    return kOK; 
}

kFx(kStatus) kArray3_Allocate(kArray3 array, kType itemType, kSize length0, kSize length1, kSize length2)
{
    kObj(kArray3, array);

    if (obj->isAttached)
    {
        obj->items = kNULL; 
        obj->isAttached = kFALSE; 
    }
    
    obj->itemType = kIsNull(itemType) ? kTypeOf(kVoid) : itemType; 
    obj->itemSize = kType_Size(obj->itemType); 
    obj->length[0] = 0; 
    obj->length[1] = 0; 
    obj->length[2] = 0; 

    kCheck(xkArray3_Realloc(array, length0, length1, length2)); 

    return kOK; 
}

kFx(kStatus) kArray3_Attach(kArray3 array, void* items, kType itemType, kSize length0, kSize length1, kSize length2)
{
    kObj(kArray3, array);

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
    obj->length[2] = length2; 
    obj->isAttached = kTRUE; 

    return kOK; 
}

kFx(kStatus) kArray3_Assign(kArray3 array, kArray3 source)
{
    kObj(kArray3, array);
    kObjN(kArray3, sourceObj, source);

    kCheckArgs(!kAlloc_IsForeign(obj->dataAlloc) && !kAlloc_IsForeign(sourceObj->dataAlloc));

    kCheck(kArray3_Allocate(array, sourceObj->itemType, sourceObj->length[0], sourceObj->length[1], sourceObj->length[2])); 
    kCheck(kCopyItems(obj->itemType, obj->items, sourceObj->items, kArray3_Count(array))); 

    return kOK;   
}

kFx(kStatus) kArray3_Zero(kArray3 array)
{
    kObj(kArray3, array);

    kCheckArgs(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kZeroItems(obj->itemType, obj->items, kArray3_Count(array)));

    return kOK; 
}

kFx(kStatus) kArray3_SetItem(kArray3 array, kSize index0, kSize index1, kSize index2, const void* item)
{
    kObj(kArray3, array);

    kAssert(!kAlloc_IsForeign(obj->dataAlloc));
    kCheckArgs((index0 < obj->length[0]) && (index1 < obj->length[1]) && (index2 < obj->length[2])); 

    kValue_Import(obj->itemType, kArray3_DataAt(array, (kSSize)index0, (kSSize)index1, (kSSize)index2), item); 

    return kOK; 
}

kFx(kStatus) kArray3_Item(kArray3 array, kSize index0, kSize index1, kSize index2, void* item)
{
    kObj(kArray3, array);

    kAssert(!kAlloc_IsForeign(obj->dataAlloc));
    kCheckArgs((index0 < obj->length[0]) && (index1 < obj->length[1]) && (index2 < obj->length[2])); 

    kItemCopy(item, kArray3_DataAt(array, (kSSize)index0, (kSSize)index1, (kSSize)index2), obj->itemSize); 

    return kOK; 
}

kFx(kIterator) xkArray3_GetIterator(kArray3 array)
{
    kObj(kArray3, array);

    return obj->items; 
}

kFx(kBool) xkArray3_HasNext(kArray3 array, kIterator iterator)
{
    kObj(kArray3, array);
    void* end = (kByte*)obj->items + kArray3_DataSize(array); 

    return (iterator != end); 
}

kFx(void*) xkArray3_Next(kArray3 array, kIterator* iterator)
{
    kObj(kArray3, array);
    void* next = *iterator; 
   
    *iterator = (kByte*)*iterator + obj->itemSize; 

    return next; 
}
