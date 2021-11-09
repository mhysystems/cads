/** 
 * @file    kAlloc.x.h
 *
 * @internal
 * Copyright (C) 2005-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#ifndef K_API_ALLOC_X_H
#define K_API_ALLOC_X_H

typedef struct kAllocStatic
{
    kAlloc systemAlloc;             //Allocates directly from the underlying system (no wrappers).
    kAlloc appAlloc;                //Created for use by application (layered on systemAlloc).
    kAssembly assembly;             //kApi assembly reference. 
} kAllocStatic;

typedef struct kAllocClass
{
    kObjectClass base; 
    kBool isForeign;                //[Protected] Reports whether the allocator will allocate memory in a foreign address space.
    kBool canGetObject;             //[Protected] Reports whether the allocator is suitable for allocating kObject headers. 
} kAllocClass;

typedef struct kAllocVTable
{
    kObjectVTable base; 
    kStatus (kCall* VGet)(kAlloc alloc, kSize size, void* mem); 
    kStatus (kCall* VFree)(kAlloc alloc, void* mem); 
    kStatus (kCall* VExport)(kAlloc alloc, void* dest, const void* src, kSize size); 
} kAllocVTable; 

kDeclareFullClassEx(k, kAlloc, kObject)

/*
 * Forward declarations.
 */

kInlineFx(kBool) kAlloc_CanGetObject(kAlloc alloc);

/* 
 * Private methods. 
 */

kFx(kStatus) xkAlloc_InitStatic(); 
kFx(kStatus) xkAlloc_EndInitStatic(); 
kFx(kStatus) xkAlloc_ReleaseStatic(); 

kFx(kStatus) xkAlloc_DefaultConstructAppAlloc(kAlloc* appAlloc, kAlloc systemAlloc); 
kFx(kStatus) xkAlloc_DefaultDestroyAppAlloc(kAlloc appAlloc); 

kFx(kStatus) xkAlloc_OnAssemblyUnloaded(kPointer unused, kAssembly assembly, kPointer args);  

kFx(kStatus) xkAlloc_FinalizeDebug(kAlloc* alloc); 

#endif
