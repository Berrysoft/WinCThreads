/**WinCThreads threads.c
 * 
 * MIT License
 * 
 * Copyright (c) 2019 Berrysoft
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */
#include "threads.h"
#include <assert.h>
#include <stdlib.h>

// TLS counts is limited to 1088 = 64 + 1024
#define _DTORS_COUNT (64 + 1024)

// An array of all the destructors
static tss_dtor_t _Dtors[_DTORS_COUNT];

// A list of all keys
typedef struct _Tss_dtor_id_node
{
    tss_t _Key;
    struct _Tss_dtor_id_node* _Next;
} _Tss_dtor_id_node;

static thread_local _Tss_dtor_id_node* _All_dtor_head = NULL;

// Add a destructor and its key to the list
static void _Add_dtor_id(_In_ tss_t key)
{
    _Tss_dtor_id_node* node = malloc(sizeof(_Tss_dtor_id_node));
    if (node)
    {
        node->_Key = key;
        node->_Next = _All_dtor_head;
        _All_dtor_head = node;
    }
}

// Clear all remained data TSS_DTOR_ITERATIONS times,
// And free them
static void _Tss_clear_all(void)
{
    bool again = true;
    for (int i = 0; i < TSS_DTOR_ITERATIONS && again; i++)
    {
        again = false;
        for (_Tss_dtor_id_node* current = _All_dtor_head; current; current = current->_Next)
        {
            if (_Dtors[current->_Key])
            {
                void* value = TlsGetValue(current->_Key);
                if (value)
                {
                    BOOL r = TlsSetValue(current->_Key, NULL);
                    assert(r);
                    again = true;
                    _Dtors[current->_Key](value);
                }
            }
        }
    }
    _Tss_dtor_id_node* current = _All_dtor_head;
    while (current)
    {
        _Tss_dtor_id_node* next = current->_Next;
        BOOL r = TlsFree(current->_Key);
        assert(r);
        free(current);
        current = next;
    }
}

// Parameter for _Thrd_start
typedef struct
{
    thrd_start_t _Func;
    void* _Arg;
} _Thrd_start_arg;

// Promise that all data will be destructed by calling thrd_exit
static DWORD WINAPI _Thrd_start(void* arg)
{
    _Thrd_start_arg* start_arg = arg;
    thrd_start_t func = start_arg->_Func;
    void* thrd_arg = start_arg->_Arg;
    free(arg);

    int res = func(thrd_arg);
    thrd_exit(res);
}

int __cdecl thrd_create(_Out_ thrd_t* thr, _In_ thrd_start_t func, _In_opt_ void* arg)
{
    *thr = NULL;
    _Thrd_start_arg* start_arg = malloc(sizeof(_Thrd_start_arg));
    if (start_arg)
    {
        start_arg->_Func = func;
        start_arg->_Arg = arg;
        *thr = CreateThread(NULL, 0, _Thrd_start, start_arg, 0, NULL);
    }
    if (!*thr)
    {
        // If it failed to create, the start_arg should be freed here
        if (start_arg)
            free(start_arg);
        if (GetLastError() == ERROR_OUTOFMEMORY)
            return thrd_nomem;
        else
            return thrd_error;
    }
    return thrd_success;
}

int __cdecl thrd_equal(_In_ thrd_t lhs, _In_ thrd_t rhs)
{
    return GetThreadId(lhs) == GetThreadId(rhs);
}

thrd_t __cdecl thrd_current(void)
{
    return GetCurrentThread();
}

void _Timespec_setvalid(struct timespec* t)
{
    while (t->tv_nsec < 0)
    {
        t->tv_sec--;
        t->tv_nsec += 1000000000;
    }
}

struct timespec _Timespec_duration(const struct timespec* time_point)
{
    struct timespec current;
    if (!timespec_get(&current, TIME_UTC))
    {
        current.tv_sec = 0;
        current.tv_nsec = 0;
        return current;
    }
    struct timespec span;
    span.tv_sec = time_point->tv_sec - current.tv_sec;
    span.tv_nsec = time_point->tv_nsec - current.tv_nsec;
    _Timespec_setvalid(&span);
    return span;
}

DWORD _Timespec_ms(const struct timespec* span)
{
    time_t ms = span->tv_sec * 1000 + span->tv_nsec / 1000000;
    if (ms < 0) ms = 0;
    return (DWORD)ms;
}

int __cdecl thrd_sleep(_In_ const struct timespec* duration, struct timespec* remaining)
{
    struct timespec t1;
    if (!timespec_get(&t1, TIME_UTC)) remaining = NULL;
    DWORD r = SleepEx(_Timespec_ms(duration), TRUE);
    if (!r) return 0;
    if (remaining)
    {
        struct timespec t2;
        if (timespec_get(&t2, TIME_UTC))
        {
            remaining->tv_sec = t2.tv_sec - t1.tv_sec - duration->tv_sec;
            remaining->tv_nsec = t2.tv_nsec - t1.tv_nsec - duration->tv_nsec;
            _Timespec_setvalid(remaining);
        }
    }
    return r == WAIT_IO_COMPLETION ? -1 : -2;
}

void __cdecl thrd_yield(void)
{
    Sleep(0);
}

__declspec(noreturn) void __cdecl thrd_exit(_In_ int res)
{
    // Clear all data before exit
    _Tss_clear_all();
    ExitThread((DWORD)res);
}

int __cdecl thrd_detach(_In_ thrd_t thr)
{
    if (CloseHandle(thr))
        return thrd_success;
    else
        return thrd_error;
}

int __cdecl thrd_join(_In_ thrd_t thr, int* res)
{
    if (WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
        return thrd_error;
    if (res)
    {
        DWORD dres;
        if (GetExitCodeThread(thr, &dres))
            *res = (int)dres;
        else
            return thrd_error;
    }
    return thrd_detach(thr);
}

int __cdecl mtx_init(_Out_ mtx_t* mutex, _In_ int type)
{
    mutex->timed = type & mtx_timed;
    mutex->recursive = type & mtx_recursive;
    mutex->locked = false;
    if (mutex->timed)
    {
        mutex->obj.mutex = CreateMutex(NULL, FALSE, NULL);
        if (!mutex->obj.mutex)
            return thrd_error;
    }
    else
    {
        InitializeCriticalSection(&mutex->obj.cs);
    }
    return thrd_success;
}

int _Mtx_non_recursive_lock(_In_ mtx_t* mutex)
{
    if (!mutex->recursive)
    {
        while (mutex->locked) Sleep(1);
        mutex->locked = true;
    }
    return thrd_success;
}

int __cdecl mtx_lock(_In_ mtx_t* mutex)
{
    if (mutex->timed)
    {
        if (WaitForSingleObject(mutex->obj.mutex, INFINITE))
            return thrd_error;
    }
    else
    {
        EnterCriticalSection(&mutex->obj.cs);
    }
    return _Mtx_non_recursive_lock(mutex);
}

int __cdecl mtx_timedlock(_In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point)
{
    if (!mutex->timed) return thrd_error;
    struct timespec span = _Timespec_duration(time_point);
    DWORD r = WaitForSingleObject(mutex->obj.mutex, _Timespec_ms(&span));
    if (r)
    {
        if (r == WAIT_TIMEOUT)
            return thrd_timedout;
        else
            return thrd_error;
    }
    return _Mtx_non_recursive_lock(mutex);
}

int __cdecl mtx_trylock(_In_ mtx_t* mutex)
{
    int r;
    if (mutex->timed)
    {
        r = WaitForSingleObject(mutex->obj.mutex, 0) ? thrd_busy : thrd_success;
    }
    else
    {
        r = TryEnterCriticalSection(&mutex->obj.cs) ? thrd_success : thrd_busy;
        if (!mutex->recursive)
        {
            if (!r) mutex->locked = true;
            r = thrd_busy;
        }
    }
    return r;
}

int __cdecl mtx_unlock(_In_ mtx_t* mutex)
{
    mutex->locked = false;
    if (mutex->timed)
    {
        if (!ReleaseMutex(mutex->obj.mutex))
            return thrd_error;
    }
    else
    {
        LeaveCriticalSection(&mutex->obj.cs);
    }
    return thrd_success;
}

void __cdecl mtx_destroy(_In_ mtx_t* mutex)
{
    if (mutex->timed)
    {
        BOOL r = CloseHandle(mutex->obj.mutex);
        assert(r);
    }
    else
    {
        DeleteCriticalSection(&mutex->obj.cs);
    }
}

static BOOL WINAPI _Init_once_callback(PINIT_ONCE initOnce, PVOID parameter, PVOID* context)
{
    // Ignore some params
    (void)initOnce;
    (void)context;
    // Call the function
    void (*func)(void) = (void (*)(void))parameter;
    func();
    return TRUE;
}

void __cdecl call_once(_In_ once_flag* flag, _In_ void(__cdecl* func)(void))
{
    BOOL r = InitOnceExecuteOnce((PINIT_ONCE)flag, _Init_once_callback, (void*)func, NULL);
    assert(r);
}

int __cdecl cnd_init(_Out_ cnd_t* cond)
{
    InitializeConditionVariable(&cond->cv);
    InitializeCriticalSection(&cond->cs);
    return thrd_success;
}

int __cdecl cnd_signal(_In_ cnd_t* cond)
{
    WakeConditionVariable(&cond->cv);
    return thrd_success;
}

int __cdecl cnd_broadcast(_In_ cnd_t* cond)
{
    WakeAllConditionVariable(&cond->cv);
    return thrd_success;
}

int _Cnd_wait(_In_ cnd_t* cond, _In_ mtx_t* mutex, DWORD ms)
{
    if (mtx_unlock(mutex)) return thrd_error;
    int ret;
    EnterCriticalSection(&cond->cs);
    if (SleepConditionVariableCS(&cond->cv, &cond->cs, ms))
        ret = thrd_success;
    else
    {
        DWORD r = GetLastError();
        if (r == ERROR_TIMEOUT)
            ret = thrd_timedout;
        else
            ret = thrd_error;
    }
    LeaveCriticalSection(&cond->cs);
    if (mtx_lock(mutex)) return thrd_error;
    return ret;
}

int __cdecl cnd_wait(_In_ cnd_t* cond, _In_ mtx_t* mutex)
{
    return _Cnd_wait(cond, mutex, INFINITE);
}

int __cdecl cnd_timedwait(_In_ cnd_t* __restrict cond, _In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point)
{
    struct timespec span = _Timespec_duration(time_point);
    return _Cnd_wait(cond, mutex, _Timespec_ms(&span));
}

void __cdecl cnd_destroy(_In_ cnd_t* cond)
{
    DeleteCriticalSection(&cond->cs);
}

int __cdecl tss_create(_Out_ tss_t* tss_key, _In_opt_ tss_dtor_t destructor)
{
    *tss_key = TlsAlloc();
    if (*tss_key == TLS_OUT_OF_INDEXES)
        return thrd_error;
    else
    {
        assert(*tss_key < _DTORS_COUNT);
        _Dtors[*tss_key] = destructor;
        if (destructor) _Add_dtor_id(*tss_key);
        return thrd_success;
    }
}

void* __cdecl tss_get(_In_ tss_t tss_key)
{
    return TlsGetValue(tss_key);
}

int __cdecl tss_set(_In_ tss_t tss_id, _In_opt_ void* val)
{
    if (TlsSetValue(tss_id, val))
        return thrd_success;
    else
        return thrd_error;
}

void __cdecl tss_delete(_In_ tss_t tss_id)
{
    _Dtors[tss_id] = NULL;
    BOOL r = TlsSetValue(tss_id, NULL);
    assert(r);
    r = TlsFree(tss_id);
    assert(r);
}
