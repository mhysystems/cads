/** 
 * @file    kArray1.cpp
 *
 * @internal
 * Copyright (C) 2005-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#include <kApi/Data/kArray1.h>
#include <kApi/Data/kCollection.h>
#include <kApi/Io/kSerializer.h>

kBeginClassEx(k, kArray1) 
    
    //serialization versions
    kAddPrivateVersion(kArray1, "kdat5", "5.0.0.0", "24-1", WriteDat5V1, ReadDat5V1)
    kAddPrivateVersion(kArray1, "kdat6", "5.7.1.0", "kArray1-0", WriteDat6V0, ReadDat6V0)

    //virtual methods
    kAddPrivateVMethod(kArray1, kObject, VRelease)
    kAddPrivateVMethod(kArray1, kObject, VDisposeItems)
    kAddPrivateVMethod(kArray1, kObject, VInitClone)
    kAddPrivateVMethod(kArray1, kObject, VSize)
    kAddPrivateVMethod(kArray1, kObject, VHasForeignData)

    //collection interface 
    kAddInterface(kArray1, kCollection)
    kAddPrivateIVMethod(kArray1, kCollection, VGetIterator, GetIterator)
    kAddIVMethod(kArray1, kCollection, VItemType, ItemType)
    kAddIVMethod(kArray1, kCollection, VCount, Count)
    kAddPrivateIVMethod(kArray1, kCollection, VHasNext, HasNext)
    kAddPrivateIVMethod(kArray1, kCollection, VNext, Next)

kEndClassEx() 

kFx(kStatus) kArray1_ConstructEx(kArray1* array, kType itemType, kSize length, kAlloc allocator, kAlloc dataAllocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kAlloc dataAlloc = kAlloc_Fallback(dataAllocator);
    kStatus status; 

    kCheck(kAlloc_GetObject(alloc, kTypeOf(kArray1), array)); 

    if (!kSuccess(status = xkArray1_Init(*array, kTypeOf(kArray1), itemType, length, alloc, dataAlloc)))
    {
        kAlloc_FreeRef(alloc, array); 
    }

    return status; 
} 

kFx(kStatus) kArray1_Construct(kArray1* array, kType itemType, kSize length, kAlloc allocator)
{
    return kArray1_ConstructEx(array, itemType, length, allocator, allocator);
}

kFx(kStatus) xkArray1_Init(kArray1 array, kType classType, kType itemType, kSize length, kAlloc alloc, kAlloc dataAlloc)
{
    kObjR(kArray1, array);
    kType resolvedItemType = kIsNull(itemType) ? kTypeOf(kVoid) : itemType; 
    kStatus status = kOK; 

    kCheckArgs(!kAlloc_IsForeign(dataAlloc) || kType_IsValue(resolvedItemType));

    kCheck(kObject_Init(array, classType, alloc)); 

    obj->dataAlloc = dataAlloc;
    obj->itemType = resolvedItemType;
    obj->itemSize = kType_Size(obj->itemType); 
    obj->allocSize = 0; 
    obj->items = kNULL;
    obj->length = 0; 
    obj->isAttached = kFALSE; 

    kTry
    {
        kTest(xkArray1_Realloc(array, length)); 
    }
    kCatch(&status)
    {
        xkArray1_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray1_VInitClone(kArray1 array, kArray1 source, kAlloc allocator)
{
    kObjR(kArray1, array);
    kObjN(kArray1, sourceObj, source);  
    kStatus status = kOK; 

    kCheck(kObject_Init(array, kObject_Type(source), allocator));     

    obj->dataAlloc = allocator;
    obj->itemType = sourceObj->itemType; 
    obj->itemSize = sourceObj->itemSize; 
    obj->allocSize = 0; 
    obj->items = kNULL; 
    obj->length = 0; 
    obj->isAttached = kFALSE; 

    kTry
    {
        kTest(xkArray1_Realloc(array, sourceObj->length)); 

        kTest(kCloneItemsEx(obj->itemType, obj->items, sourceObj->items, obj->length, obj->dataAlloc, sourceObj->dataAlloc)); 
    }
    kCatch(&status)
    {
        xkArray1_VDisposeItems(array); 
        xkArray1_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray1_VRelease(kArray1 array)
{
    kObj(kArray1, array);

    if (!obj->isAttached)
    {
        kCheck(kAlloc_Free(obj->dataAlloc, obj->items)); 
    }

    kCheck(kObject_VRelease(array)); 

    return kOK; 
}

kFx(kStatus) xkArray1_VDisposeItems(kArray1 array)
{
    kObj(kArray1, array);

    kCheck(kDisposeItems(obj->itemType, obj->items, obj->length)); 

    return kOK; 
}

kFx(kSize) xkArray1_VSize(kArray1 array)
{
    kObj(kArray1, array);
    kSize dataSize = (!obj->isAttached) ? obj->allocSize : kArray1_DataSize(array); 
    kSize size = sizeof(kArray1Class) + dataSize; 

    size += kMeasureItems(obj->itemType, obj->items, obj->length); 

    return size; 
}

kFx(kBool) xkArray1_VHasForeignData(kArray1 array)
{
    kObj(kArray1, array);

    return kAlloc_IsForeign(obj->dataAlloc) || kHasForeignData(obj->itemType, obj->items, obj->length);
}

kFx(kStatus) xkArray1_WriteDat5V1(kArray1 array, kSerializer serializer)
{
    kObj(kArray1, array);
    kTypeVersion itemVersion; 

    kCheckState(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kSerializer_WriteSize(serializer, obj->length)); 
    kCheck(kSerializer_WriteType(serializer, obj->itemType, &itemVersion));
    kCheck(kSerializer_WriteItems(serializer, obj->itemType, itemVersion, obj->items, obj->length)); 

    return kOK; 
}

kFx(kStatus) xkArray1_ReadDat5V1(kArray1 array, kSerializer serializer, kAlloc allocator)
{
    kObjR(kArray1, array);
    kSize length = 0; 
    kTypeVersion itemVersion;
    kType itemType = kNULL;            
    kStatus status; 

    kCheck(kSerializer_ReadSize(serializer, &length));
    kCheck(kSerializer_ReadType(serializer, &itemType, &itemVersion)); 

    kCheck(xkArray1_Init(array, kTypeOf(kArray1), itemType, length, allocator, allocator)); 
  
    if (!kSuccess(status = kSerializer_ReadItems(serializer, itemType, itemVersion, obj->items, length)))
    {
        xkArray1_VDisposeItems(array); 
        xkArray1_VRelease(array); 
        return status;
    }

    return kOK; 
}

kFx(kStatus) xkArray1_WriteDat6V0(kArray1 array, kSerializer serializer)
{
    kObj(kArray1, array);
    kTypeVersion itemVersion; 

    kCheckState(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kSerializer_WriteType(serializer, obj->itemType, &itemVersion));
    kCheck(kSerializer_WriteSize(serializer, obj->length)); 
    kCheck(kSerializer_WriteItems(serializer, obj->itemType, itemVersion, obj->items, obj->length)); 

    return kOK; 
}

kFx(kStatus) xkArray1_ReadDat6V0(kArray1 array, kSerializer serializer, kAlloc allocator)
{
    kObjR(kArray1, array);
    kType itemType = kNULL;            
    kTypeVersion itemVersion; 
    kStatus status; 
    kSize length = 0; 

    kCheck(kSerializer_ReadType(serializer, &itemType, &itemVersion));
    kCheck(kSerializer_ReadSize(serializer, &length)); 

    kCheck(xkArray1_Init(array, kTypeOf(kArray1), itemType, length, allocator, allocator)); 

    kTry
    {
        kTest(kSerializer_ReadItems(serializer, itemType, itemVersion, obj->items, length)); 
    }
    kCatch(&status)
    {
        xkArray1_VDisposeItems(array); 
        xkArray1_VRelease(array); 
        kEndCatch(status); 
    }

    return kOK; 
}

kFx(kStatus) xkArray1_Realloc(kArray1 array, kSize length)
{
    kObj(kArray1, array);
    kSize newSize = kMax_(obj->allocSize, length*obj->itemSize); 

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
        kMemSet(obj->items, 0, obj->itemSize*length);  
    }

    obj->length = length; 
    
    return kOK; 
}

kFx(kStatus) kArray1_Allocate(kArray1 array, kType itemType, kSize length)
{
    kObj(kArray1, array);

    if (obj->isAttached)
    {
        obj->items = kNULL; 
        obj->isAttached = kFALSE; 
    }
    
    obj->itemType = kIsNull(itemType) ? kTypeOf(kVoid) : itemType; 
    obj->itemSize = kType_Size(obj->itemType); 
    obj->length = 0; 

    kCheck(xkArray1_Realloc(array, length)); 

    return kOK; 
}

kFx(kStatus) kArray1_Attach(kArray1 array, void* items, kType itemType, kSize length)
{
    kObj(kArray1, array);

    if (!obj->isAttached)
    {
        kCheck(kAlloc_FreeRef(obj->dataAlloc, &obj->items));
    }
    
    obj->itemType = kIsNull(itemType) ? kTypeOf(kVoid) : itemType; 
    obj->itemSize = kType_Size(obj->itemType); 
    obj->allocSize = 0; 
    obj->items = items; 
    obj->length = length; 
    obj->isAttached = kTRUE; 

    return kOK; 
}

kFx(kStatus) kArray1_Assign(kArray1 array, kArray1 source)
{
    kObj(kArray1, array);
    kObjN(kArray1, sourceObj, source);

    kCheckArgs(!kAlloc_IsForeign(obj->dataAlloc) && !kAlloc_IsForeign(sourceObj->dataAlloc));

    kCheck(kArray1_Allocate(array, sourceObj->itemType, sourceObj->length)); 
    kCheck(kCopyItems(obj->itemType, obj->items, sourceObj->items, obj->length)); 

    return kOK;   
}

kFx(kStatus) kArray1_Zero(kArray1 array)
{
    kObj(kArray1, array);

    kCheckArgs(!kAlloc_IsForeign(obj->dataAlloc));

    kCheck(kZeroItems(obj->itemType, obj->items, obj->length));

    return kOK; 
}

kFx(kStatus) kArray1_SetItem(kArray1 array, kSize index, const void* item)
{
    kObj(kArray1, array);

    kAssert(!kAlloc_IsForeign(obj->dataAlloc));
    kCheckArgs(index < obj->length);    

    kValue_Import(obj->itemType, kArray1_DataAt(array, (kSSize)index), item); 

    return kOK; 
}

kFx(kStatus) kArray1_Item(kArray1 array, kSize index, void* item)
{
    kObj(kArray1, array);

    kAssert(!kAlloc_IsForeign(obj->dataAlloc));
    kCheckArgs(index < obj->length); 

    kItemCopy(item, kArray1_DataAt(array, (kSSize)index), obj->itemSize); 

    return kOK; 
}

kFx(kIterator) xkArray1_GetIterator(kArray1 array)
{
    kObj(kArray1, array);
    return obj->items; 
}

kFx(kBool) xkArray1_HasNext(kArray1 array, kIterator iterator)
{
    kObj(kArray1, array);
    void* end = (kByte*)obj->items + obj->length*obj->itemSize;

    return (iterator != end); 
}

kFx(void*) xkArray1_Next(kArray1 array, kIterator* iterator)
{
    kObj(kArray1, array);
    void* next = *iterator; 
   
    *iterator = (kByte*)*iterator + obj->itemSize; 

    return next; 
}
