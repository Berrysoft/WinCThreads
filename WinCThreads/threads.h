#pragma once

#ifndef __STDC_NO_THREADS__

#include <Windows.h>
#include <thr/xthreads.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define _Thread_local __declspec(thread)

    enum
    {
        thrd_success = _Thrd_success,
        thrd_nomem = _Thrd_nomem,
        thrd_timedout = _Thrd_timedout,
        thrd_busy = _Thrd_busy,
        thrd_error = _Thrd_error
    };

    typedef int(WINAPI* thrd_start_t)(void*);

    typedef _Thrd_t thrd_t;

    int thrd_create(thrd_t* thr, thrd_start_t func, void* arg);
    int thrd_equal(thrd_t lhs, thrd_t rhs);
    thrd_t thrd_current(void);
    int thrd_sleep(const struct timespec* duration, struct timespec* remaining);
    inline void thrd_yield(void) { _Thrd_yield(); }
    __declspec(noreturn) void thrd_exit(int res);
    inline int thrd_detach(thrd_t thr) { return _Thrd_detach(thr); }
    inline int thrd_join(thrd_t thr, int* res) { return _Thrd_join(thr, res); }

    enum
    {
        mtx_plain = _Mtx_plain,
        mtx_recursive = _Mtx_recursive,
        mtx_timed = _Mtx_timed
    };

    typedef _Mtx_t mtx_t;

    inline int mtx_init(mtx_t* mutex, int type) { return _Mtx_init(mutex, type); }
    inline int mtx_lock(mtx_t* mutex) { return _Mtx_lock(*mutex); }
    inline int mtx_timedlock(mtx_t* __restrict mutex, const struct timespec* __restrict time_point) { return _Mtx_timedlock(*mutex, (const xtime*)time_point); }
    inline int mtx_unlock(mtx_t* mutex) { return _Mtx_unlock(*mutex); }
    inline void mtx_destroy(mtx_t* mutex) { _Mtx_destroy(*mutex); }

    typedef INIT_ONCE once_flag;
#define ONCE_FLAG_INIT \
    {                  \
    }

    void call_once(once_flag* flag, void (*func)(void));

    typedef _Cnd_t cnd_t;

    inline int cnd_init(cnd_t* cond) { return _Cnd_init(cond); }
    inline int cnd_signal(cnd_t* cond) { return _Cnd_signal(*cond); }
    inline int cnd_broadcast(cnd_t* cond) { return _Cnd_broadcast(*cond); }
    inline int cnd_wait(cnd_t* cond, mtx_t* mutex) { return _Cnd_wait(*cond, *mutex); }
    inline int cnd_timedwait(cnd_t* __restrict cond, mtx_t* __restrict mutex, const struct timespec* __restrict time_point) { return _Cnd_timedwait(*cond, *mutex, (const xtime*)time_point); }
    inline void cnd_destroy(cnd_t* cond) { _Cnd_destroy(*cond); }

#define thread_local _Thread_local
#define TSS_DTOR_ITERATIONS 256

    typedef void (*tss_dtor_t)(void*);

    typedef DWORD tss_t;

    int tss_create(tss_t* tss_key, tss_dtor_t destructor);
    void* tss_get(tss_t tss_key);
    int tss_set(tss_t tss_id, void* val);
    void tss_delete(tss_t tss_id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !__STDC_NO_THREADS__
