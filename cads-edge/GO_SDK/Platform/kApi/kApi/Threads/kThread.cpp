/** 
 * @file    kThread.cpp
 *
 * @internal
 * Copyright (C) 2005-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#define K_PLATFORM
#include <kApi/Threads/kThread.h>
#include <kApi/Threads/kSemaphore.h>
#include <kApi/Threads/kTimer.h>
#include <kApi/Data/kMath.h>
#include <kApi/Utils/kUtils.h>

kBeginClassEx(k, kThread)
    kAddPrivateVMethod(kThread, kObject, VRelease)
kEndClassEx()

kFx(kStatus) kThread_Construct(kThread *thread, kAlloc allocator)
{
    kAlloc alloc = kAlloc_Fallback(allocator);
    kStatus status; 

    kCheck(kAlloc_GetObject(alloc, kTypeOf(kThread), thread)); 

    if (!kSuccess(status = xkThread_Init(*thread, kTypeOf(kThread), alloc)))
    {
        kAlloc_FreeRef(alloc, thread); 
    }

    return status; 
} 

kFx(kStatus) kThread_Start(kThread thread, kThreadFx function, kPointer context)
{
    return kThread_StartEx(thread, function, context, 0, kNULL, 0); 
}

kFx(kStatus) kThread_SleepAtLeast(k64u duration)
{
    k64u startTime = kTimer_Now(); 
    k64u endTime = startTime + duration; 
    k64u currentTime; 

    while ((currentTime = kTimer_Now()) < endTime)
    {
        kCheck(kThread_Sleep(endTime - currentTime)); 
    }

    return kOK; 
}

kFx(kThreadFx) xkThread_Handler(kThread thread)
{
    kObj(kThread, thread);

    return obj->function; 
}

kFx(kPointer) xkThead_HandlerContext(kThread thread)
{
    kObj(kThread, thread);

    return obj->context;
}

kFx(kBool) kThread_IsSelf(kThread other)
{
    return xkThread_CurrentId() == xkThread_Id(other);
}

#if defined(K_WINDOWS)

kFx(kSize) kThread_ProcessorCount()
{
    SYSTEM_INFO sysInfo = { 0 }; 

    GetSystemInfo(&sysInfo); 

    return (kSize) sysInfo.dwNumberOfProcessors; 
}

kFx(k32s) kThread_OsPriority(k32s priority)
{
    if      (priority <= -3)  return THREAD_PRIORITY_IDLE; 
    else if (priority == -2)  return THREAD_PRIORITY_LOWEST; 
    else if (priority == -1)  return THREAD_PRIORITY_BELOW_NORMAL; 
    else if (priority ==  0)  return THREAD_PRIORITY_NORMAL; 
    else if (priority ==  1)  return THREAD_PRIORITY_ABOVE_NORMAL; 
    else if (priority ==  2)  return THREAD_PRIORITY_HIGHEST; 
    else                      return THREAD_PRIORITY_TIME_CRITICAL; 
}

kFx(kStatus) kThread_Sleep(k64u duration)
{
    DWORD osDuration = (DWORD) xkTimeToKernelTime(duration); 
    
    Sleep(osDuration); 
    
    return kOK; 
}

kFx(kStatus) xkThread_Init(kThread thread, kType type, kAlloc allocator)
{
    kObjR(kThread, thread);

    kCheck(kObject_Init(thread, type, allocator)); 
    
    obj->id = 0;
    obj->handle = kNULL;
    obj->function = kNULL; 
    obj->context = kNULL;
    
    return kOK; 
}

kFx(kStatus) xkThread_VRelease(kThread thread)
{
    kCheck(kThread_Join(thread, kINFINITE, kNULL)); 

    kCheck(kObject_VRelease(thread)); 

    return kOK; 
}

//name argument not supported at this time
kFx(kStatus) kThread_StartEx(kThread thread, kThreadFx function, kPointer context, 
                             kSize stackSize, const kChar* name, k32s priority)
{
    kObj(kThread, thread); 
    kStatus exception; 

    kCheckState(kIsNull(obj->function)); 

    obj->function = function; 
    obj->context = context; 

    kTry
    {
        if (kIsNull(obj->handle = (HANDLE)_beginthreadex(0, (unsigned)stackSize, xkThread_EntryPoint, thread, CREATE_SUSPENDED, &obj->id)))
        {
            kThrow(kERROR_OS); 
        }

        if (!SetThreadPriority(obj->handle, kThread_OsPriority(priority)))
        {
            kThrow(kERROR_OS); 
        }

        if (ResumeThread(obj->handle) == k32U_MAX)
        {
            kThrow(kERROR_OS); 
        }
    }
    kCatch(&exception)
    {
        if (!kIsNull(obj->handle))
        {
            CloseHandle(obj->handle); 
        }

        obj->handle = kNULL; 
        obj->context = kNULL; 
        obj->id = 0; 
        obj->function = kNULL; 

        kEndCatch(exception); 
    }

    return kOK; 
}

kFx(kStatus) kThread_Join(kThread thread, k64u timeout, kStatus* exitCode)
{
    kObj(kThread, thread); 
    DWORD osTimeout = (DWORD) xkTimeToKernelTime(timeout); 
    DWORD waitResult; 
    DWORD threadExit;

    if (obj->function)
    {
        waitResult = WaitForSingleObject(obj->handle, osTimeout); 
        
        if (waitResult == WAIT_TIMEOUT)
        {
            return kERROR_TIMEOUT; 
        }
        else if (waitResult != WAIT_OBJECT_0)
        {
            return kERROR_OS; 
        }       
        
        if (!GetExitCodeThread(obj->handle, &threadExit))
        {
            return kERROR; 
        }

        if (!CloseHandle(obj->handle))
        {
            return kERROR;
        }
        
        obj->function = kNULL; 
        obj->context = kNULL; 

        if (exitCode)
        {
            *exitCode = (kStatus)threadExit; 
        }
    }
    
    return kOK;
}

unsigned int __stdcall xkThread_EntryPoint(void *arg)
{
    kObj(kThread, arg); 
    
    return (unsigned int) obj->function(obj->context);
}

kFx(kThreadId) xkThread_Id(kThread thread)
{
    kObj(kThread, thread); 
    return obj->id;     
}

kFx(kThreadId) xkThread_CurrentId()
{
    return (kThreadId) GetCurrentThreadId(); 
}

kFx(kBool) xkThread_CompareId(kThreadId a, kThreadId b)
{
    return a == b; 
}

#elif defined (K_TI_BIOS)

kFx(kSize) kThread_ProcessorCount()
{
    //simple; OS only supports one CPU
    return 1; 
}

kFx(k32s) kThread_OsPriority(k32s priority)
{
    k32s osPriority = xkTHREAD_DEFAULT_PRIORITY + priority; 

    return kMath_Clamp_(osPriority, xkTHREAD_MIN_OS_PRIORITY, ti_sysbios_knl_Task_numPriorities); 
}

kFx(kStatus) kThread_Sleep(k64u duration)
{
    k32u osDuration = xkTimeToKernelTime(duration); 
    
    if (osDuration > 0)
    {
        ti_sysbios_knl_Task_sleep(osDuration);
    }
    else
    {
        ti_sysbios_knl_Task_yield(); 
    }
    
    return kOK; 
}

kFx(kStatus) xkThread_Init(kThread thread, kType type, kAlloc allocator)
{
    kObjR(kThread, thread); 
    kStatus exception; 
    
    kCheck(kObject_Init(thread, type, allocator)); 

    kZero(obj->handle); 
    obj->name[0] = 0;
    obj->joinSem = kNULL; 
    kAtomic32s_Init(&obj->exitCode, kOK); 
    obj->function = kNULL; 
    obj->context = kNULL;
   
    kTry
    {   
        kTest(kStrCopy(obj->name, kCountOf(obj->name), "kThread instance")); 
        kTest(kSemaphore_Construct(&obj->joinSem, 0, allocator)); 
    }
    kCatch(&exception)
    {
        xkThread_VRelease(thread); 
        kEndCatch(exception); 
    }

    return kOK; 
}

kFx(kStatus) xkThread_VRelease(kThread thread)
{
    kObj(kThread, thread); 

    kCheck(kThread_Join(thread, kINFINITE, kNULL)); 
    
    kCheck(kObject_Destroy(obj->joinSem)); 

    kCheck(kObject_VRelease(thread)); 

    return kOK; 
}

kFx(kStatus) kThread_StartEx(kThread thread, kThreadFx function, kPointer context, 
                             kSize stackSize, const kChar* name, k32s priority)
{
    kObj(kThread, thread); 
    xdc_runtime_Error_Block eb; 
    ti_sysbios_knl_Task_Params params; 
    kStatus exception; 

    kCheckState(kIsNull(obj->function)); 

    obj->function = function; 
    obj->context = context; 

    kTry
    {
        if (!kIsNull(name))
        {
            kTest(kStrCopy(obj->name, kCountOf(obj->name), name)); 
        }

        xdc_runtime_Error_init(&eb);

        ti_sysbios_knl_Task_Params_init(&params);
        params.arg0 = (UArg) thread; 
        params.priority =  kThread_OsPriority(priority); 
        params.stackSize = (stackSize > 0) ? stackSize : params.stackSize;
        
        if (kIsNull(obj->handle = ti_sysbios_knl_Task_create(xkThread_EntryPoint, &params, &eb)))
        {
            kThrow(kERROR_OS); 
        }
    }
    kCatch(&exception)
    {
        obj->function = kNULL; 
        obj->context = kNULL; 
        obj->handle = kNULL; 

        kEndCatch(exception); 
    }

    return kOK; 
}

kFx(kStatus) kThread_Join(kThread thread, k64u timeout, kStatus* exitCode)
{
    kObj(kThread, thread); 

    if (obj->function)
    {
        kCheck(kSemaphore_Wait(obj->joinSem, timeout)); 

        ti_sysbios_knl_Task_delete(&obj->handle); 
        
        obj->function = kNULL; 
        obj->context = kNULL; 

        if (exitCode)
        {
            *exitCode = kAtomic32s_Get(&obj->exitCode); 
        }
    }
    
    return kOK;
}

void xkThread_EntryPoint(UArg arg0, UArg arg1)
{
    kThread thread = (kThread) arg0; 
    kObj(kThread, thread);
    ti_sysbios_knl_Task_Stat taskStats;

    //required by SYS/BIOS NDK, in order to use sockets
    if (fdOpenSession(ti_sysbios_knl_Task_self()) != 1)    
    {
        kAtomic32s_Exchange(&obj->exitCode, kERROR_OS); 
        kAssert(kFALSE);  
        return; 
    }

    kAtomic32s_Exchange(&obj->exitCode, obj->function(obj->context)); 

    fdCloseSession(ti_sysbios_knl_Task_self());
 
    kSemaphore_Post(obj->joinSem); 

    ti_sysbios_knl_Task_stat(ti_sysbios_knl_Task_self(), &taskStats); 

    kAssert(taskStats.used <= taskStats.stackSize); 

    return; 
}

kFx(kThreadId) xkThread_Id(kThread thread)
{
    kObj(kThread, thread); 
    return obj->handle; 
}

kFx(kThreadId) xkThread_CurrentId()
{
    return ti_sysbios_knl_Task_self(); 
}

kFx(kBool) xkThread_CompareId(kThreadId a, kThreadId b)
{
    return a == b; 
}

#elif defined(K_VX_KERNEL)

kFx(kSize) kThread_ProcessorCount()
{
    return (kSize) vxCpuConfiguredGet(); 
}

kFx(k32s) kThread_OsPriority(k32s priority)
{
    k32s defaultPriority = xkTHREAD_DEFAULT_PRIORITY;      
    k32s osPriority = defaultPriority - priority;   //higher logical priorities are represented by lower priority numbers  

    return kMath_Clamp_(osPriority, xkTHREAD_MIN_OS_PRIORITY, xkTHREAD_MAX_OS_PRIORITY); 
}

kFx(kStatus) kThread_Sleep(k64u duration)
{
    _Vx_ticks_t osDuration = (_Vx_ticks_t) xkTimeToKernelTime(duration); 
    
    if (taskDelay(osDuration) != OK)
    {
        return kERROR_OS; 
    }
    
    return kOK; 
}

kFx(kStatus) xkThread_Init(kThread thread, kType type, kAlloc allocator)
{
    kObjR(kThread, thread); 
    kStatus exception; 
            
    kCheck(kObject_Init(thread, type, allocator)); 

    obj->id = TASK_ID_ERROR; 
    obj->joinSem = kNULL; 
    kAtomic32s_Init(&obj->exitCode, kOK); 
    obj->function = kNULL; 
    obj->context = kNULL;
 
    kTry
    {   
        kTest(kSemaphore_Construct(&obj->joinSem, 0, allocator)); 
    }
    kCatch(&exception)
    {
        xkThread_VRelease(thread); 
        kEndCatch(exception); 
    }

    return kOK; 
}        
        
kFx(kStatus) xkThread_VRelease(kThread thread)
{
    kObj(kThread, thread); 

    kCheck(kThread_Join(thread, kINFINITE, kNULL)); 
    
    kCheck(kObject_Destroy(obj->joinSem)); 

    kCheck(kObject_VRelease(thread)); 

    return kOK; 
}

kFx(kStatus) kThread_StartEx(kThread thread, kThreadFx function, kPointer context, 
                             kSize stackSize, const kChar* name, k32s priority)
{
    kObj(kThread, thread); 
    k32s osPriority = kThread_OsPriority(priority); 
    kSize osStackSize = (stackSize == 0) ? xkTHREAD_DEFAULT_STACK_SIZE : stackSize; 
    kText128 nameBuffer; 
    kStatus exception; 

    kCheckState(kIsNull(obj->function)); 

    obj->function = function; 
    obj->context = context; 

    kTry
    {    
        kStrCopy(nameBuffer, sizeof(nameBuffer), name); 

        obj->id = taskSpawn(kIsNull(name) ? NULL : nameBuffer, osPriority, xkTHREAD_DEFAULT_OPTIONS, osStackSize, (FUNCPTR)xkThread_EntryPoint, (_Vx_usr_arg_t)thread, 0, 0, 0, 0, 0, 0, 0, 0, 0); 

        if (obj->id == TASK_ID_ERROR)
        {
            kThrow(kERROR_OS);   
        }
    }
    kCatch(&exception)
    {
        obj->function = kNULL; 
        obj->context = kNULL; 
        obj->id = TASK_ID_ERROR; 

        kEndCatch(exception); 
    }

    return kOK; 
}

kFx(kStatus) kThread_Join(kThread thread, k64u timeout, kStatus* exitCode)
{
    kObj(kThread, thread);

    if (obj->function)
    {
        kCheck(kSemaphore_Wait(obj->joinSem, timeout)); 
                
        obj->function = kNULL;         
        obj->context = kNULL; 

        if (exitCode)
        {
            *exitCode = kAtomic32s_Get(&obj->exitCode); 
        }
    }
    
    return kOK;   
}        
       
int xkThread_EntryPoint(_Vx_usr_arg_t arg0)
{
    kThread thread = (kThread) arg0; 
    kObj(kThread, thread);
       
    kAtomic32s_Exchange(&obj->exitCode, obj->function(obj->context)); 
    
    kSemaphore_Post(obj->joinSem); 
    
    return kOK;  
}

kFx(kThreadId) xkThread_Id(kThread thread)
{
    kObj(kThread, thread); 
    return obj->id;     
}

kFx(kThreadId) xkThread_CurrentId()
{
    return taskIdSelf(); 
}

kFx(kBool) xkThread_CompareId(kThreadId a, kThreadId b)
{
    return a == b; 
}

#elif defined(K_POSIX)

kFx(kSize) kThread_ProcessorCount()
{
    long cpuCount = -1; 

#ifdef _SC_NPROCESSORS_ONLN
    cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    return (cpuCount > 0) ? (kSize) cpuCount : 1; 
}

kFx(kStatus) kThread_Sleep(k64u duration)
{
    useconds_t durationMs = (useconds_t) ((duration + 999)/1000); 
    
    usleep(durationMs*1000); 
    
    return kOK; 
}

kFx(kStatus) xkThread_Init(kThread thread, kType type, kAlloc alloc)
{
    kObjR(kThread, thread); 
    kStatus exception; 
    
    kCheck(kObject_Init(thread, type, alloc)); 

    obj->hasJoined = kNULL; 
    kZero(obj->handle); 
    obj->function = kNULL; 
    obj->context = kNULL; 

    kTry
    {
        kTest(kSemaphore_Construct(&obj->hasJoined, 0, alloc)); 
    }
    kCatch(&exception)
    {
        xkThread_VRelease(thread); 
        kEndCatch(exception); 
    }

    return kOK; 
}

kFx(kStatus) xkThread_VRelease(kThread thread)
{
    kObj(kThread, thread); 

    kCheck(kThread_Join(thread, kINFINITE, kNULL)); 

    kCheck(kObject_Destroy(obj->hasJoined)); 

    kCheck(kObject_VRelease(thread)); 

    return kOK; 
}

//stackSize, name, and priority arguments not supported at this time
kFx(kStatus) kThread_StartEx(kThread thread, kThreadFx function, kPointer context, 
                             kSize stackSize, const kChar* name, k32s priority)
{
    kObj(kThread, thread); 

    kCheckState(kIsNull(obj->function)); 

    obj->function = function; 
    obj->context = context; 

    if (pthread_create(&obj->handle, kNULL, xkThread_EntryPoint, thread) != 0)
    {
        obj->function = kNULL; 
        return kERROR; 
    }    

    return kOK; 
}

kFx(kStatus) kThread_Join(kThread thread, k64u timeout, kStatus* exitCode)
{
    kObj(kThread, thread); 
    void* threadExit; 
    
    if (obj->function)
    {
        if (timeout != kINFINITE)
        {
            kCheck(kSemaphore_Wait(obj->hasJoined, timeout)); 
        }

        if (pthread_join(obj->handle, (void**)&threadExit) != 0)
        {
            return kERROR; 
        }

        obj->function = kNULL; 
        obj->context = kNULL; 

        if (exitCode)
        {
            *exitCode = (kStatus) (kSSize) threadExit; 
        }
    }
    
    return kOK; 
}

void* xkThread_EntryPoint(void* arg)
{
    kObj(kThread, arg); 
    kStatus result; 

    result = obj->function(obj->context);

    //signal thread completion
    kSemaphore_Post(obj->hasJoined); 
        
    return (void*) (kSSize) result;  
}

kFx(kThreadId) xkThread_Id(kThread thread)
{
    kObj(kThread, thread); 
    return obj->handle;      
}

kFx(kThreadId) xkThread_CurrentId()
{
    return pthread_self(); 
}

kFx(kBool) xkThread_CompareId(kThreadId a, kThreadId b)
{
    return (pthread_equal(a, b) != 0); 
}

#endif
