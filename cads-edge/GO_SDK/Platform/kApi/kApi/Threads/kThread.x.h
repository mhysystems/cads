/** 
 * @file    kThread.x.h
 *
 * @internal
 * Copyright (C) 2005-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#ifndef K_API_THREAD_X_H
#define K_API_THREAD_X_H

kDeclareClassEx(k, kThread, kObject)

#if defined(K_PLATFORM) 

#if defined(K_WINDOWS)
    
#   define xkThreadPlatformFields()                                  \
        HANDLE handle;      /* thread handle */                     \
        unsigned id;        /* unique thread id from os */

    unsigned int __stdcall xkThread_EntryPoint(void* arg);

#elif defined(K_TI_BIOS)

#   define xkTHREAD_MIN_OS_PRIORITY     (1)          /* minimum valid priority (0 reserved for idle task) */
#   define xkTHREAD_DEFAULT_PRIORITY    (7)          /* default priority assigned to threads created with this class */

#   define xkThreadPlatformFields()                                              \
        ti_sysbios_knl_Task_Handle handle;  /* thread handle */                 \
        kText64 name;                       /* descriptive name */              \
        kSemaphore joinSem;                 /* implements join behaviour */     \
        kAtomic32s exitCode;                /* result of thread execution */

    void xkThread_EntryPoint(UArg arg0, UArg arg1);

#elif defined(K_VX_KERNEL)

#   define xkTHREAD_MIN_OS_PRIORITY     (0)              /* minimum valid priority value (highest relative priority) */
#   define xkTHREAD_MAX_OS_PRIORITY     (255)            /* maximum valid priority value (lowest relative priority) */
#   define xkTHREAD_DEFAULT_PRIORITY    (128)            /* default priority assigned to threads created with this class */
#   define xkTHREAD_DEFAULT_STACK_SIZE  (0x8000)         /* default stack size assigned to threads created with this class */
#   define xkTHREAD_DEFAULT_OPTIONS     (VX_FP_TASK)     /* default thread creation options */

#   define xkThreadPlatformFields()                                      \
        TASK_ID id;                 /* thread identifier */             \
        kSemaphore joinSem;         /* implements join behaviour */     \
        kAtomic32s exitCode;        /* result of thread execution */

    int xkThread_EntryPoint(_Vx_usr_arg_t arg0);
            
#elif defined(K_POSIX)

#   define xkThreadPlatformFields()                                      \
        kSemaphore hasJoined;       /* supports join wait argument */   \
        pthread_t handle;           /* thead handle */

    void* xkThread_EntryPoint(void* arg);

#endif

typedef struct kThreadClass
{
    kObjectClass base;    
    kThreadFx function;          // entry-point function
    kPointer context;            // entry-point context
    xkThreadPlatformFields()
} kThreadClass;

/* 
* Private methods. 
*/

kFx(kStatus) xkThread_Init(kThread thread, kType type, kAlloc alloc); 
kFx(kStatus) xkThread_VRelease(kThread thread); 


/** 
 * Gets a unique identifier that represents the currently executing thread.
 * 
 * @public              @memberof kThread
 * @return              Unique thread identifier. 
 */
kFx(kThreadId) xkThread_CurrentId(); 

/** 
 * Gets a unique identifier representing the thread.
 * 
 * This field is only valid after the thread has been successfully started, and 
 * before the thread has been successfully joined.
 *
 * @public              @memberof kThread
 * @param   thread      Thread object. 
 * @return              Unique thread identifier. 
 */
kFx(kThreadId) xkThread_Id(kThread thread); 

/** 
 * Compares two thread identifiers.
 *
 * @public          @memberof kThread
 * @param   a       First thread identifier.
 * @param   b       Second thread identifier.
 * @return          kTRUE if the threads are identical; kFALSE otherwise. 
 */
kFx(kBool) xkThread_CompareId(kThreadId a, kThreadId b); 

#endif

/* 
* Private methods. 
*/

kFx(kThreadFx) xkThread_Handler(kThread thread);
kFx(kPointer)  xkThead_HandlerContext(kThread thread);

#endif
