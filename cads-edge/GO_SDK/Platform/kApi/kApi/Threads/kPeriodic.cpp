/** 
 * @file    kPeriodic.cpp
 *
 * @internal
 * Copyright (C) 2010-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#include <kApi/Threads/kPeriodic.h>
#include <kApi/Threads/kLock.h>
#include <kApi/Threads/kThread.h>
#include <kApi/Threads/kTimer.h>
#include <kApi/Threads/kSemaphore.h>

kBeginClassEx(k, kPeriodic)
    kAddPrivateVMethod(kPeriodic, kObject, VRelease)
kEndClassEx()

kFx(kStatus) kPeriodic_Construct(kPeriodic* timer, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status; 

    kCheck(kAlloc_GetObject(alloc, kTypeOf(kPeriodic), timer)); 

    if (!kSuccess(status = xkPeriodic_Init(*timer, kTypeOf(kPeriodic),  0, kNULL, 0, alloc)))
    {
        kAlloc_FreeRef(alloc, timer); 
    }

    return status; 
} 

kFx(kStatus) kPeriodic_ConstructEx(kPeriodic* timer, kSize stackSize, const kChar* name, k32s priority, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status; 

    kCheck(kAlloc_GetObject(alloc, kTypeOf(kPeriodic), timer)); 

    if (!kSuccess(status = xkPeriodic_Init(*timer, kTypeOf(kPeriodic), stackSize, name, priority, alloc)))
    {
        kAlloc_FreeRef(alloc, timer); 
    }

    return status; 
} 

kFx(kStatus) xkPeriodic_Init(kPeriodic timer, kType type, kSize stackSize, const kChar* name, k32s priority, kAlloc allocator)
{
    kObjR(kPeriodic, timer); 
    kStatus status; 

    kCheck(kObject_Init(timer, type, allocator)); 

    obj->lock = kNULL;
    obj->thread = kNULL;
    obj->startTime = 0;
    obj->semaphore = kNULL;
    obj->quit = 0;
    obj->enabled = kFALSE;
    obj->period = 0;
    obj->counter = 0;
    obj->inProgress = kFALSE;
    obj->onElapsed = kNULL;
    obj->onElapsedContext = kNULL;

    obj->startTime = kTimer_Now(); 
  
    kTry
    {
        //we could add an "ex" constructor argument for priority inheritance. but it's currently a platform-private feature
        //and the approach below provides sensible default behavior that aligns with our intended use
        kTest(kLock_ConstructEx(&obj->lock, (priority < 0) ? xkLOCK_OPTION_PRIORITY_INHERITANCE : kLOCK_OPTION_NONE, allocator)); 
        kTest(kSemaphore_Construct(&obj->semaphore, 0, allocator)); 
        
        kTest(kThread_Construct(&obj->thread, allocator)); 
        kTest(kThread_StartEx(obj->thread, xkPeriodic_ThreadEntry, obj, stackSize, name, priority)); 
    }
    kCatch(&status)
    {
        xkPeriodic_VRelease(timer); 
        kEndCatch(status); 
    }    
    
    return kOK; 
}

kFx(kStatus) xkPeriodic_VRelease(kPeriodic timer)
{
    kObj(kPeriodic, timer); 

    if (obj->thread)
    {
        kAtomic32s_Exchange(&obj->quit, kTRUE); 
        kCheck(kSemaphore_Post(obj->semaphore)); 
        kCheck(kThread_Join(obj->thread, kINFINITE, kNULL)); 
    }

    kCheck(kDestroyRef(&obj->thread)); 
    kCheck(kDestroyRef(&obj->lock)); 
    kCheck(kDestroyRef(&obj->semaphore)); 

    kCheck(kObject_VRelease(timer)); 

    return kOK; 
}

kFx(kStatus) kPeriodic_Start(kPeriodic timer, k64u period, kPeriodicElapsedFx onElapsed, kPointer context)
{
    kObj(kPeriodic, timer); 
    kBool shouldPost = kFALSE; 

    kLock_Enter(obj->lock); 
    {
        if (!obj->inProgress)
        {
            obj->enabled = kTRUE; 
            obj->counter = 0; 
            obj->startTime = kTimer_Now(); 

            shouldPost = kTRUE; 
        }

        obj->period = period;
        obj->onElapsed = onElapsed;
        obj->onElapsedContext = context;

    }
    kLock_Exit(obj->lock); 

    if (shouldPost)
    {
        kSemaphore_Post(obj->semaphore); 
    }

    return kOK; 
}

kFx(kStatus) kPeriodic_Stop(kPeriodic timer)
{
    kObj(kPeriodic, timer); 

    kLock_Enter(obj->lock); 
    {
        obj->enabled = kFALSE; 
        obj->onElapsed = kNULL; 
        obj->onElapsedContext = kNULL; 
    }
    kLock_Exit(obj->lock); 

    return kOK; 
}

kFx(kBool) kPeriodic_Enabled(kPeriodic timer)
{
    kObj(kPeriodic, timer); 
    kBool enabled = kFALSE; 

    kLock_Enter(obj->lock); 
    {
        enabled = obj->enabled; 
    }
    kLock_Exit(obj->lock); 

    return enabled; 
}

kFx(k64u) kPeriodic_Period(kPeriodic timer)
{
    kObj(kPeriodic, timer); 
    k64u period = 0; 

    kLock_Enter(obj->lock); 
    {
        period = obj->period; 
    }
    kLock_Exit(obj->lock); 

    return period; 
}

kFx(kStatus) xkPeriodic_ThreadEntry(kPeriodic timer)
{
    kObj(kPeriodic, timer); 
    k64u timeout = kINFINITE;
    
    while (!kAtomic32s_Get(&obj->quit))
    {
        kBool timedOut = (kSemaphore_Wait(obj->semaphore, timeout) == kERROR_TIMEOUT); 
        k64u elapsed = 0;

        kLock_Enter(obj->lock); 
        {
            timeout = kINFINITE; 

            if (obj->enabled)
            {
                if (obj->counter == 0)
                {
                    elapsed = kTimer_Now() - obj->startTime;
                }

                if (((obj->counter > 0) && timedOut) || (elapsed >= obj->period))
                {
                    obj->inProgress = kTRUE; 
                    {
                        obj->onElapsed(obj->onElapsedContext, timer);
                    }
                    obj->inProgress = kFALSE; 

                    obj->counter++; 
                    timeout = obj->period; 
                }
                else
                {
                    timeout = (obj->period - elapsed);
                }
            }
        }
        kLock_Exit(obj->lock); 
    }

    return kOK; 
}

kFx(kPeriodicElapsedFx) xkPeriodic_Handler(kPeriodic timer)
{
    kObj(kPeriodic, timer);

    return obj->onElapsed;
}

kFx(kPointer) xkPeriodic_HandlerContext(kPeriodic timer)
{
    kObj(kPeriodic, timer);

    return obj->onElapsedContext; 
}
