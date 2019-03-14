#include "threads.h"

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg)
{
    thr->_Hnd = CreateThread(NULL, 0, func, arg, 0, &(thr->_Id));
    if (!thr->_Hnd)
    {
        if (GetLastError() == thrd_nomem)
            return thrd_nomem;
        else
            return thrd_error;
    }
    return thrd_success;
}

int thrd_equal(thrd_t lhs, thrd_t rhs)
{
    return lhs._Id == rhs._Id;
}

thrd_t thrd_current(void)
{
    return (thrd_t){ ._Hnd = GetCurrentThread(), ._Id = GetCurrentThreadId() };
}

int thrd_sleep(const struct timespec* duration, struct timespec* remaining)
{
    struct timespec t1;
    timespec_get(&t1, TIME_UTC);
    _Thrd_sleep((const xtime*)duration);
    if (remaining)
    {
        struct timespec t2;
        timespec_get(&t2, TIME_UTC);
        remaining->tv_sec = t2.tv_sec - t1.tv_sec - duration->tv_sec;
        remaining->tv_nsec = t2.tv_nsec - t1.tv_nsec - duration->tv_nsec;
        while (remaining->tv_nsec < 0)
        {
            remaining->tv_sec--;
            remaining->tv_nsec += 1000000000;
        }
    }
    return 0;
}

__declspec(noreturn) void thrd_exit(int res)
{
    ExitThread((DWORD)res);
}

BOOL _Init_once_callback(PINIT_ONCE initOnce, PVOID parameter, PVOID* context)
{
    void (*func)(void) = parameter;
    func();
    return TRUE;
}

void call_once(once_flag* flag, void (*func)(void))
{
    InitOnceExecuteOnce(flag, _Init_once_callback, func, NULL);
}

int tss_create(tss_t* tss_key, tss_dtor_t destructor)
{
	// TODO: Use destructor.
    *tss_key = TlsAlloc();
    if (*tss_key == TLS_OUT_OF_INDEXES)
        return thrd_error;
    else
        return thrd_success;
}

void* tss_get(tss_t tss_key)
{
    return TlsGetValue(tss_key);
}

int tss_set(tss_t tss_id, void* val)
{
    if (TlsSetValue(tss_id, val))
        return thrd_success;
    else
        return thrd_error;
}

void tss_delete(tss_t tss_id)
{
    TlsFree(tss_id);
}
