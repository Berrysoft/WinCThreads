/**WinCThreads threads.h
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
#pragma once
#ifndef _INC_THREADS
#define _INC_THREADS

#ifndef __STDC_NO_THREADS__

#ifdef __cplusplus
#define BEGIN_EXTERN_C \
    extern "C"         \
    {
#define END_EXTERN_C }
#else
#define BEGIN_EXTERN_C
#define END_EXTERN_C
#endif // __cplusplus

// Use this header to make the code simple.
// Therefore I needn't write thrd, mtx or cnd myself.
#include <thr/xthreads.h>
#include <time.h>

BEGIN_EXTERN_C

enum
{
    thrd_success = _Thrd_success,
    thrd_nomem = _Thrd_nomem,
    thrd_timedout = _Thrd_timedout,
    thrd_busy = _Thrd_busy,
    thrd_error = _Thrd_error
};

// Thread

typedef int(__cdecl* thrd_start_t)(void*);

typedef _Thrd_t thrd_t;

int __cdecl thrd_create(_Out_ thrd_t* thr, _In_ thrd_start_t func, _In_opt_ void* arg);
inline int __cdecl thrd_equal(_In_ thrd_t lhs, _In_ thrd_t rhs) { return lhs._Id == rhs._Id; }
thrd_t __cdecl thrd_current(void);
int __cdecl thrd_sleep(_In_ const struct timespec* duration, struct timespec* remaining);
inline void __cdecl thrd_yield(void) { _Thrd_yield(); }
__declspec(noreturn) void __cdecl thrd_exit(_In_ int res);
inline int __cdecl thrd_detach(_In_ thrd_t thr) { return _Thrd_detach(thr); }
inline int __cdecl thrd_join(_In_ thrd_t thr, _Out_opt_ int* res) { return _Thrd_join(thr, res); }

// Mutex

enum
{
    mtx_plain = _Mtx_plain,
    mtx_recursive = _Mtx_recursive,
    mtx_timed = _Mtx_timed
};

typedef _Mtx_t mtx_t;

inline int __cdecl mtx_init(_Out_ mtx_t* mutex, _In_ int type) { return _Mtx_init(mutex, type); }
inline int __cdecl mtx_lock(_In_ mtx_t* mutex) { return _Mtx_lock(*mutex); }
inline int __cdecl mtx_timedlock(_In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point) { return _Mtx_timedlock(*mutex, (const xtime*)time_point); }
inline int __cdecl mtx_unlock(_In_ mtx_t* mutex) { return _Mtx_unlock(*mutex); }
inline void __cdecl mtx_destroy(_In_ mtx_t* mutex) { _Mtx_destroy(*mutex); }

// Call-once

// The declaration or INIT_ONCE
typedef union {
    void* _Ptr;
} once_flag;

#define ONCE_FLAG_INIT \
    {                  \
        NULL           \
    }

void __cdecl call_once(_In_ once_flag* flag, _In_ void(__cdecl* func)(void));

// Condition varible

typedef _Cnd_t cnd_t;

inline int __cdecl cnd_init(_Out_ cnd_t* cond) { return _Cnd_init(cond); }
inline int __cdecl cnd_signal(_In_ cnd_t* cond) { return _Cnd_signal(*cond); }
inline int __cdecl cnd_broadcast(_In_ cnd_t* cond) { return _Cnd_broadcast(*cond); }
inline int __cdecl cnd_wait(_In_ cnd_t* cond, _In_ mtx_t* mutex) { return _Cnd_wait(*cond, *mutex); }
inline int __cdecl cnd_timedwait(_In_ cnd_t* __restrict cond, _In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point) { return _Cnd_timedwait(*cond, *mutex, (const xtime*)time_point); }
inline void __cdecl cnd_destroy(_In_ cnd_t* cond) { _Cnd_destroy(*cond); }

// This keyword hasn't been implemented by MSVC
#define _Thread_local __declspec(thread)
// There's already thread_local in C++
#ifndef __cpluscplus
#define thread_local _Thread_local
#endif // !__cpluscplus

// Thread special storage

#define TSS_DTOR_ITERATIONS 4

typedef void(__cdecl* tss_dtor_t)(void*);

typedef unsigned long tss_t;

int __cdecl tss_create(_Out_ tss_t* tss_key, _In_opt_ tss_dtor_t destructor);
void* __cdecl tss_get(_In_ tss_t tss_key);
int __cdecl tss_set(_In_ tss_t tss_id, _In_opt_ void* val);
void __cdecl tss_delete(_In_ tss_t tss_id);

END_EXTERN_C

#endif // !__STDC_NO_THREADS__
#endif // !_INC_THREADS
