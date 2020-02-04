/**WinCThreads threads.h
 * 
 * MIT License
 * 
 * Copyright (c) 2019-2020 Berrysoft
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
#pragma once
#ifndef _INC_THREADS
#define _INC_THREADS

#ifndef __STDC_NO_THREADS__

#ifdef WINCTHREADS_EXPOTRS
#define THREADS_API __declspec(dllexport)
#else
#define THREADS_API __declspec(dllimport)
#endif

#ifdef __cplusplus
#define BEGIN_EXTERN_C \
    extern "C"         \
    {
#define END_EXTERN_C }
#else
#define BEGIN_EXTERN_C
#define END_EXTERN_C
#endif // __cplusplus

#include <Windows.h>
#include <stdbool.h>
#include <time.h>

BEGIN_EXTERN_C

enum
{
    thrd_success,
    thrd_nomem,
    thrd_timedout,
    thrd_busy,
    thrd_error
};

// Thread

typedef int(__cdecl* thrd_start_t)(void*);

typedef HANDLE thrd_t;

THREADS_API int __cdecl thrd_create(_Out_ thrd_t* thr, _In_ thrd_start_t func, _In_opt_ void* arg);
THREADS_API int __cdecl thrd_equal(_In_ thrd_t lhs, _In_ thrd_t rhs);
THREADS_API thrd_t __cdecl thrd_current(void);
THREADS_API int __cdecl thrd_sleep(_In_ const struct timespec* duration, struct timespec* remaining);
THREADS_API void __cdecl thrd_yield(void);
THREADS_API __declspec(noreturn) void __cdecl thrd_exit(_In_ int res);
THREADS_API int __cdecl thrd_detach(_In_ thrd_t thr);
THREADS_API int __cdecl thrd_join(_In_ thrd_t thr, int* res);

// Mutex

enum
{
    mtx_plain = 0x1,
    _Mtx_shared = 0x2,
    mtx_timed = 0x3,
    mtx_recursive = 0x4
};

#ifdef _MSC_VER
#pragma warning(push) // Avoid bug warning
#pragma warning(disable : 4214) // Bit field types other than int
#endif // _MSC_VER
typedef struct
{
    union {
        CRITICAL_SECTION cs;
        HANDLE mutex;
        SRWLOCK shared;
    } obj;
    unsigned int basetype : 2;
    bool recursive : 1;
    bool locked : 1;
} mtx_t;
#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

THREADS_API int __cdecl mtx_init(_Out_ mtx_t* mutex, _In_ int type);
THREADS_API int __cdecl mtx_lock(_In_ mtx_t* mutex);
THREADS_API int __cdecl _Mtx_slock(_In_ mtx_t* mutex);
THREADS_API int __cdecl mtx_timedlock(_In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point);
THREADS_API int __cdecl mtx_trylock(_In_ mtx_t* mutex);
THREADS_API int __cdecl _Mtx_tryslock(_In_ mtx_t* mutex);
THREADS_API int __cdecl mtx_unlock(_In_ mtx_t* mutex);
THREADS_API int __cdecl _Mtx_sunlock(_In_ mtx_t* mutex);
THREADS_API void __cdecl mtx_destroy(_In_ mtx_t* mutex);

// Call-once

typedef INIT_ONCE once_flag;

#define ONCE_FLAG_INIT INIT_ONCE_STATIC_INIT

THREADS_API void __cdecl call_once(_In_ once_flag* flag, _In_ void(__cdecl* func)(void));

// Condition varible

typedef struct
{
    CONDITION_VARIABLE cv;
    CRITICAL_SECTION cs;
} cnd_t;

THREADS_API int __cdecl cnd_init(_Out_ cnd_t* cond);
THREADS_API int __cdecl cnd_signal(_In_ cnd_t* cond);
THREADS_API int __cdecl cnd_broadcast(_In_ cnd_t* cond);
THREADS_API int __cdecl cnd_wait(_In_ cnd_t* __restrict cond, _In_ mtx_t* __restrict mutex);
THREADS_API int __cdecl cnd_timedwait(_In_ cnd_t* __restrict cond, _In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point);
THREADS_API int __cdecl _Cnd_swait(_In_ cnd_t* __restrict cond, _In_ mtx_t* __restrict mutex);
THREADS_API int __cdecl _Cnd_stimedwait(_In_ cnd_t* __restrict cond, _In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point);
THREADS_API void __cdecl cnd_destroy(_In_ cnd_t* cond);

// Semaphore

typedef HANDLE _Smph_t;
THREADS_API int __cdecl _Smph_init(_Out_ _Smph_t* sem, int max_count, int count);
THREADS_API int __cdecl _Smph_wait(_In_ _Smph_t* sem);
THREADS_API int __cdecl _Smph_timedwait(_In_ _Smph_t* __restrict sem, _In_ const struct timespec* __restrict time_point);
THREADS_API int __cdecl _Smph_trywait(_In_ _Smph_t* sem);
THREADS_API int __cdecl _Smph_post(_In_ _Smph_t* sem);
THREADS_API int __cdecl _Smph_multipost(_In_ _Smph_t* sem, int count);
THREADS_API int __cdecl _Smph_get(_In_ _Smph_t* __restrict sem, int* __restrict count);
THREADS_API void __cdecl _Smph_destroy(_In_ _Smph_t* sem);

#ifdef _MSC_VER
// This keyword hasn't been implemented by MSVC
#define _Thread_local __declspec(thread)
#endif // _MSC_VER

// There's already thread_local in C++
#ifndef __cpluscplus
#define thread_local _Thread_local
#endif // !__cpluscplus

// Thread special storage

#define TSS_DTOR_ITERATIONS 4

typedef void(__cdecl* tss_dtor_t)(void*);

typedef DWORD tss_t;

THREADS_API int __cdecl tss_create(_Out_ tss_t* tss_key, _In_opt_ tss_dtor_t destructor);
THREADS_API void* __cdecl tss_get(tss_t tss_key);
THREADS_API int __cdecl tss_set(tss_t tss_id, _In_opt_ void* val);
THREADS_API void __cdecl tss_delete(tss_t tss_id);

END_EXTERN_C

#endif // !__STDC_NO_THREADS__
#endif // !_INC_THREADS
