/** 
 * @file    kPeriodic.x.h
 *
 * @internal
 * Copyright (C) 2010-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#ifndef K_API_PERIODIC_X_H
#define K_API_PERIODIC_X_H

typedef struct kPeriodicClass
{
    kObjectClass base; 
    kLock lock; 
    kThread thread; 
    k64u startTime; 
    kSemaphore semaphore; 
    kAtomic32s quit; 
    kBool enabled; 
    k64u period; 
    k64u counter; 
    kBool inProgress; 
    kPeriodicElapsedFx onElapsed; 
    kPointer onElapsedContext;
} kPeriodicClass;

kDeclareClassEx(k, kPeriodic, kObject)

/* 
* Private methods.
*/

kFx(kStatus) xkPeriodic_Init(kPeriodic timer, kType type, kSize stackSize, const kChar* name, k32s priority, kAlloc allocator);
kFx(kStatus) xkPeriodic_VRelease(kPeriodic timer); 

kFx(kStatus) xkPeriodic_ThreadEntry(kPeriodic timer); 

kFx(kPeriodicElapsedFx) xkPeriodic_Handler(kPeriodic timer);
kFx(kPointer) xkPeriodic_HandlerContext(kPeriodic timer);

#endif
