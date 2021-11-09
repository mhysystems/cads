/** 
 * @file    kObject.cpp
 *
 * @internal
 * Copyright (C) 2005-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#include <kApi/kObject.h>
#include <kApi/Utils/kObjectPool.h>

kBeginVirtualClassEx(k, kObject)

    kAddFlags(kObject, kTYPE_FLAGS_ABSTRACT)

    //serialization versions
    kAddAbstractVersion(kObject, "kdat5", "5.0.0.0", "22-0")
    kAddAbstractVersion(kObject, "kdat6", "5.7.1.0", "kObject-0")

    //virtual methods
    kAddVMethod(kObject, kObject, VRelease)
    kAddVMethod(kObject, kObject, VDisposeItems)
    kAddVMethod(kObject, kObject, VInitClone)
    kAddVMethod(kObject, kObject, VHashCode)
    kAddVMethod(kObject, kObject, VEquals)
    kAddVMethod(kObject, kObject, VSize)
    kAddVMethod(kObject, kObject, VHasForeignData)

kEndVirtualClassEx()

kFx(kStatus) xkObject_CloneImpl(kObject* object, kObject source, kAlloc allocator)
{
    kObjN(kObject, sourceObj, source);
    kAlloc alloc = kAlloc_Fallback(allocator);
    kType type = sourceObj->type;
    kStatus status;

    kCheckArgs(!kAlloc_IsForeign(alloc));

    kCheck(kAlloc_GetObject(alloc, type, object));

    if (!kSuccess(status = xkObject_VTable(source)->VInitClone(*object, source, alloc)))
    {
        kAlloc_FreeRef(alloc, object);
    }

    return status;
}

kFx(kStatus) xkObject_DestroyImpl(kObject object, kBool dispose)
{
    kObj(kObject, object);

    if (kAtomic32s_Decrement(&obj->refCount) == 0)
    {
        if (!kIsNull(obj->pool))
        {
            obj->refCount = 1;

            kCheck(kObjectPool_Reclaim(obj->pool, object));
        }
        else
        {
            kAlloc alloc = obj->alloc;

            if (dispose)
            {
                 kCheck(xkObject_VTable(object)->VDisposeItems(object)); 
            }

            kCheck(xkObject_VTable(object)->VRelease(object)); 

            if (!kIsNull(alloc))
            {
                kCheck(kAlloc_Free(alloc, obj));
            }
            else
            {
                kCheck(xkSysMemFree(obj));
            }
        }
    }

    return kOK; 
}
