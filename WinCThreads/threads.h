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

typedef int(__stdcall* thrd_start_t)(void*);

typedef _Thrd_t thrd_t;

int thrd_create(_Out_ thrd_t* thr, _In_ thrd_start_t func, _In_opt_ void* arg);
int thrd_equal(_In_ thrd_t lhs, _In_ thrd_t rhs);
thrd_t thrd_current(void);
int thrd_sleep(_In_ const struct timespec* duration, _Out_opt_ struct timespec* remaining);
inline void thrd_yield(void) { _Thrd_yield(); }
__declspec(noreturn) void thrd_exit(_In_ int res);
inline int thrd_detach(_In_ thrd_t thr) { return _Thrd_detach(thr); }
inline int thrd_join(_In_ thrd_t thr, _Out_opt_ int* res) { return _Thrd_join(thr, res); }

enum
{
    mtx_plain = _Mtx_plain,
    mtx_recursive = _Mtx_recursive,
    mtx_timed = _Mtx_timed
};

typedef _Mtx_t mtx_t;

inline int mtx_init(_Out_ mtx_t* mutex, _In_ int type) { return _Mtx_init(mutex, type); }
inline int mtx_lock(_In_ mtx_t* mutex) { return _Mtx_lock(*mutex); }
inline int mtx_timedlock(_In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point) { return _Mtx_timedlock(*mutex, (const xtime*)time_point); }
inline int mtx_unlock(_In_ mtx_t* mutex) { return _Mtx_unlock(*mutex); }
inline void mtx_destroy(_In_ mtx_t* mutex) { _Mtx_destroy(*mutex); }

typedef union {
    void* _Ptr;
} once_flag;

#define ONCE_FLAG_INIT \
    {                  \
        0              \
    }

void call_once(_In_ once_flag* flag, _In_ void (*func)(void));

typedef _Cnd_t cnd_t;

inline int cnd_init(_Out_ cnd_t* cond) { return _Cnd_init(cond); }
inline int cnd_signal(_In_ cnd_t* cond) { return _Cnd_signal(*cond); }
inline int cnd_broadcast(_In_ cnd_t* cond) { return _Cnd_broadcast(*cond); }
inline int cnd_wait(_In_ cnd_t* cond, _In_ mtx_t* mutex) { return _Cnd_wait(*cond, *mutex); }
inline int cnd_timedwait(_In_ cnd_t* __restrict cond, _In_ mtx_t* __restrict mutex, _In_ const struct timespec* __restrict time_point) { return _Cnd_timedwait(*cond, *mutex, (const xtime*)time_point); }
inline void cnd_destroy(_In_ cnd_t* cond) { _Cnd_destroy(*cond); }

#define _Thread_local __declspec(thread)
#ifndef __cpluscplus
#define thread_local _Thread_local
#endif // !__cpluscplus

#define TSS_DTOR_ITERATIONS 256

typedef void (*tss_dtor_t)(void*);

typedef unsigned long tss_t;

int tss_create(_Out_ tss_t* tss_key, _In_opt_ tss_dtor_t destructor);
void* tss_get(_In_ tss_t tss_key);
int tss_set(_In_ tss_t tss_id, _In_opt_ void* val);
void tss_delete(_In_ tss_t tss_id);

END_EXTERN_C

#endif // !__STDC_NO_THREADS__
#endif // !_INC_THREADS
