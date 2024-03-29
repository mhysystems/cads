/** 
 * @file    kArray2.x.h
 *
 * @internal
 * Copyright (C) 2005-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#ifndef K_API_ARRAY_2_X_H
#define K_API_ARRAY_2_X_H

typedef struct kArray2Class
{
    kObjectClass base; 
    kAlloc dataAlloc;           //allocator used for data
    kType itemType;             //item type
    kSize itemSize;             //item size, in bytes
    kSize allocSize;            //size of allocated array memory, in bytes
    void* items;                //array memory 
    kSize length[2];            //array length per dimension, in items
    kBool isAttached;           //is array memory externally owned?
} kArray2Class;
    
kDeclareClassEx(k, kArray2, kObject) 

/* 
* Forward declarations. 
*/

kFx(kStatus) kArray2_Attach(kArray2 array, void* items, kType itemType, kSize length0, kSize length1); 
kFx(kStatus) kArray2_SetItem(kArray2 array, kSize index0, kSize index1, const void* item); 
kFx(kStatus) kArray2_Item(kArray2 array, kSize index0, kSize index1, void* item); 
kInlineFx(kSize) kArray2_Count(kArray2 array);
kInlineFx(kSize) kArray2_Length(kArray2 array, kSize dimension);
kInlineFx(kType) kArray2_ItemType(kArray2 array); 
kInlineFx(kSize) kArray2_ItemSize(kArray2 array);
kInlineFx(void*) kArray2_Data(kArray2 array);
kInlineFx(void*) kArray2_DataAt(kArray2 array, kSSize index0, kSSize index1);
kInlineFx(void*) kArray2_At(kArray2 array, kSize index0, kSize index1);

/* 
* Private methods. 
*/

kFx(kStatus) xkArray2_Init(kArray2 array, kType classType, kType itemType, kSize length0, kSize length1, kAlloc controlAlloc, kAlloc dataAlloc);
kFx(kStatus) xkArray2_VInitClone(kArray2 array, kArray2 source, kAlloc allocator); 

kFx(kStatus) xkArray2_VRelease(kArray2 array); 
kFx(kStatus) xkArray2_VDisposeItems(kArray2 array); 

kFx(kSize) xkArray2_VSize(kArray2 array); 
kFx(kBool) xkArray2_VHasForeignData(kArray2 array);

kFx(kStatus) xkArray2_WriteDat5V1(kArray2 array, kSerializer serializer); 
kFx(kStatus) xkArray2_ReadDat5V1(kArray2 array, kSerializer serializer, kAlloc allocator); 
kFx(kStatus) xkArray2_WriteDat6V0(kArray2 array, kSerializer serializer); 
kFx(kStatus) xkArray2_ReadDat6V0(kArray2 array, kSerializer serializer, kAlloc allocator); 

kFx(kStatus) xkArray2_Realloc(kArray2 array, kSize length0, kSize length1);

kFx(kIterator) xkArray2_GetIterator(kArray2 array); 
kFx(kBool) xkArray2_HasNext(kArray2 array, kIterator iterator); 
kFx(void*) xkArray2_Next(kArray2 array, kIterator* iterator); 

kInlineFx(kStatus) xkArray2_AttachT(kArray2 array, void* items, kType itemType, kSize length0, kSize length1, kSize itemSize)
{
    kAssert(xkType_IsPointerCompatible(itemType, itemSize)); 

    return kArray2_Attach(array, items, itemType, length0, length1);
} 

kInlineFx(kStatus) xkArray2_SetItemT(kArray2 array, kSize index0, kSize index1, const void* item, kSize itemSize)
{
    kAssert(xkType_IsPointerCompatible(kArray2_ItemType(array), itemSize)); 

    return kArray2_SetItem(array, index0, index1, item);
} 

kInlineFx(kStatus) xkArray2_ItemT(kArray2 array, kSize index0, kSize index1, void* item, kSize itemSize)
{
    kAssert(xkType_IsPointerCompatible(kArray2_ItemType(array), itemSize)); 

    return kArray2_Item(array, index0, index1, item);
} 

kInlineFx(void*) xkArray2_DataT(kArray2 array, kSize itemSize)
{
    kAssert(xkType_IsPointerCompatible(kArray2_ItemType(array), itemSize)); 

    return kArray2_Data(array);
} 

kInlineFx(void*) xkArray2_DataAtT(kArray2 array, kSSize index0, kSSize index1, kSize itemSize)
{
    kAssert(xkType_IsPointerCompatible(kArray2_ItemType(array), itemSize)); 

    return kArray2_DataAt(array, index0, index1);
} 

kInlineFx(void*) xkArray2_AtT(kArray2 array, kSize index0, kSize index1, kSize itemSize)
{
    kAssert(xkType_IsPointerCompatible(kArray2_ItemType(array), itemSize)); 

    return kArray2_At(array, index0, index1);
} 

kInlineFx(void*) xkArray2_AsT(kArray2 array, kSize index0, kSize index1, kSize itemSize)
{
    kAssert(itemSize == kArray2_ItemSize(array)); 

    return kArray2_At(array, index0, index1);
}


#endif
