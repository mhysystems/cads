/** 
 * @file    kAlloc.h
 * @brief   Declares the kAlloc class. 
 *
 * @internal
 * Copyright (C) 2005-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#include <kApi/kApiDef.h>   //--inclusion order controlled by kApiDef

#ifndef K_API_ALLOC_H
#define K_API_ALLOC_H

#include <kApi/kAlloc.x.h>

/**
 * @class   kAlloc
 * @extends kObject
 * @ingroup kApi
 * @brief   Abstract base class for memory allocator types. 
 */

/*
* Public 
*/

/** 
 * Allocates a block of memory. 
 *
 * @public              @memberof kAlloc
 * @param   alloc       Allocator object. 
 * @param   size        Size of memory to allocate, in bytes.
 * @param   mem         Receives pointer to allocated memory (pointer to a pointer).
 * @return              Operation status. 
 */
kInlineFx(kStatus) kAlloc_Get(kAlloc alloc, kSize size, void* mem)
{   
    return xkAlloc_VTable(alloc)->VGet(alloc, size, mem);
}

/** 
 * Allocates a block of memory and zero-initializes the block. 
 *
 * @public              @memberof kAlloc
 * @param   alloc       Allocator object. 
 * @param   size        Size of memory to allocate, in bytes.
 * @param   mem         Receives pointer to allocated memory (pointer to a pointer). 
 * @return              Operation status. 
 */
kInlineFx(kStatus) kAlloc_GetZero(kAlloc alloc, kSize size, void* mem)
{
    kCheck(kAlloc_Get(alloc, size, mem)); 
 
    kCheck(kMemSet(kPointer_ReadAs(mem, kPointer), 0, size)); 

    return kOK; 
}

/** 
 * Allocates a block of memory large enough for an object of the specified type.
 *
 * @public              @memberof kAlloc
 * @param   alloc       Allocator object. 
 * @param   type        Object type for which memory should be allocated. 
 * @param   mem         Receives pointer to allocated memory (pointer to a pointer).
 * @return              Operation status. 
 */
kInlineFx(kStatus) kAlloc_GetObject(kAlloc alloc, kType type, void* mem)
{
    kCheckArgs(kAlloc_CanGetObject(alloc));

    return kAlloc_Get(alloc, kType_InnerSize(type), mem);
}

/** 
 * Frees a block of memory. 
 *
 * @public              @memberof kAlloc
 * @param   alloc       Allocator object. 
 * @param   mem         Pointer to memory to free (or kNULL). 
 * @return              Operation status. 
 */
kInlineFx(kStatus) kAlloc_Free(kAlloc alloc, void* mem)
{   
    return xkAlloc_VTable(alloc)->VFree(alloc, mem);
}

/** 
 * Frees a block of memory and sets the memory pointer to kNULL. 
 *
 * @public              @memberof kAlloc
 * @param   alloc       Allocator object. 
 * @param   mem         Pointer to pointer to memory to free (or pointer to kNULL). 
 * @return              Operation status. 
 */
kInlineFx(kStatus) kAlloc_FreeRef(kAlloc alloc, void* mem)
{
    kCheck(kAlloc_Free(alloc, *(void**)mem)); 
    
    kPointer_WriteAs(mem, kNULL, kPointer); 

    return kOK; 
}

/** 
 * Copies memory from the allocator's address space to the host address space.
 * 
 * @protected           @memberof kAlloc
 * @param   alloc       Allocator associated with source memory. 
 * @param   dest        Destination for the memory copy (host memory domain).
 * @param   src         Source for the memory copy.
 * @param   size        Size of memory block to be copied, in bytes.
 * @return              Operation status. 
 */
kInlineFx(kStatus) kAlloc_Export(kAlloc alloc, void* dest, const void* src, kSize size)
{   
    return xkAlloc_VTable(alloc)->VExport(alloc, dest, src, size);
}

/** 
 * Reports whether this allocator is suitable for allocating objects. 
 * 
 * @public              @memberof kAlloc
 * @param   alloc       Allocator object. 
 * @return              kTRUE if this allocator can allocate objects. 
 */
kInlineFx(kBool) kAlloc_CanGetObject(kAlloc alloc)
{
    kObj(kAlloc, alloc);

    return obj->canGetObject;
}

/** 
 * Reports whether memory will be allocated in a foreign address space.   
 * 
 * @public              @memberof kAlloc
 * @param   alloc       Allocator object. 
 * @return              kTRUE if memory will be allocated in a foreign address spaces. 
 */
kInlineFx(kBool) kAlloc_IsForeign(kAlloc alloc)
{
    kObj(kAlloc, alloc);

    return obj->isForeign;
}


/** 
 * Gets the allocator that should normally be used by applications to request memory.
 *
 * The App alloctor is typically implemented by layering additional allocators on the 
 * System allocator (e.g., kPoolAlloc, kDebugAlloc). 
 *
 * @public      @memberof kAlloc
 * @return      Application memory allocator. 
 */
kInlineFx(kAlloc) kAlloc_App()
{
    return xkAlloc_Static()->appAlloc;
}

/** 
 * Gets the system allocator. 
 * 
 * The System allocator is a kUserAlloc instance that allocates directly from main system memory. 
 * 
 * For most purposes, the App allocator should be prefered to the System allocator. Use of the System 
 * allocator may be appropriate when the additional layers provided by the App allocator are undesirable.
 *
 * @public      @memberof kAlloc
 * @return      System memory allocator. 
 */
kInlineFx(kAlloc) kAlloc_System()
{
    return xkAlloc_Static()->systemAlloc;
}

/** 
 * Returns the passed allocator, or if null, the App allocator. 
 *
 * @public              @memberof kAlloc
 * @param   alloc       Allocator. 
 * @return              Allocator. 
 */
kInlineFx(kAlloc) kAlloc_Fallback(kAlloc alloc)
{
    return kIsNull(alloc) ? kAlloc_App() : alloc; 
}

/*
* Protected
*/

/**
 * Protected method called by derived classes to initialize the kAlloc base class. 
 * 
 * This method should typically be called as the first statement in derived initializer methods. 
 *
 * @protected           @memberof kAlloc
 * @param   alloc       Allocator instance (not yet initialized). 
 * @param   type        Object type (required).
 * @param   allocator   Memory allocator for the allocator itself (required).
 * @return              Operation status.  
*/
kFx(kStatus) kAlloc_Init(kAlloc alloc, kType type, kAlloc allocator); 

/**
 * Protected virtual method that deallocates any resources owned by the object. 
 * 
 * @protected           @memberof kAlloc
 * @param   alloc       Allocator. 
 * @return              Operation status.
 * @see                 kObject_VRelease.
*/
kFx(kStatus) kAlloc_VRelease(kAlloc alloc); 

/** 
 * Protected virtual method that allocates memory. 
 *
 * This method can be overridden in derived classes to implement custom memory allocation.
 * 
 * @protected           @memberof kAlloc
 * @param   alloc       Allocator. 
 * @param   size        Size of memory to allocate, in bytes.
 * @param   mem         Receives pointer to allocated memory (pointer to a pointer).
 * @return              Operation status. 
 */
kFx(kStatus) kAlloc_VGet(kAlloc alloc, kSize size, void* mem); 

/** 
 * Protected virtual method that frees memory. 
 *
 * This method can be overridden in derived classes to implement custom memory deallocation.
 * 
 * @protected           @memberof kAlloc
 * @param   alloc       Allocator. 
 * @param   mem         Pointer to memory to free (or kNULL). 
 * @return              Operation status. 
 */
kFx(kStatus) kAlloc_VFree(kAlloc alloc, void* mem); 

/** 
 * Protected virtual method that copies memory from the allocator's address space to the host address space.
 * 
 * @protected           @memberof kAlloc
 * @param   alloc       Allocator.
 * @param   dest        Destination for the memory copy (in host memory domain).
 * @param   src         Source for the memory copy (in allocator memory domain).
 * @param   size        Size of memory block to be copied, in bytes.
 * @return              Operation status. 
 */
kFx(kStatus) kAlloc_VExport(kAlloc alloc, void* dest, const void* src, kSize size);

#endif
