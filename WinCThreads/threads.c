#include "threads.h"

static tss_dtor_t _Dtors[64 + 1024];

typedef struct _Tss_dtor_node
{
    tss_dtor_t _Dtor;
    void* _Data;
    struct _Tss_dtor_node* _Next;
} _Tss_dtor_node;

static _Thread_local _Tss_dtor_node* _Dtor_head = NULL;
static _Thread_local _Tss_dtor_node* _Dtor_tail = NULL;

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg)
{
    thr->_Hnd = CreateThread(NULL, 0, func, arg, 0, &(thr->_Id));
    if (!thr->_Hnd)
    {
        if (GetLastError() == ERROR_OUTOFMEMORY)
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
    DWORD r = !SleepEx((DWORD)duration->tv_sec * 1000 + duration->tv_nsec / 1000000, TRUE);
    if (!r) return 0;
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
    return r == WAIT_IO_COMPLETION ? -1 : -2;
}

__declspec(noreturn) void thrd_exit(int res)
{
    _Tss_dtor_node* current = _Dtor_head;
    while (current)
    {
        _Tss_dtor_node* next = current->_Next;
        current->_Dtor(current->_Data);
        free(current);
        current = next;
    }
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
    *tss_key = TlsAlloc();
    if (*tss_key == TLS_OUT_OF_INDEXES)
        return thrd_error;
    else
    {
        _Dtors[*tss_key] = destructor;
        return thrd_success;
    }
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
    _Tss_dtor_node* node = malloc(sizeof(_Tss_dtor_node));
    node->_Dtor = _Dtors[tss_id];
    node->_Data = TlsGetValue(tss_id);
    node->_Next = NULL;
    if (!_Dtor_head)
        _Dtor_head = node;
    else
        _Dtor_tail->_Next = node;
    _Dtor_tail = node;
    TlsFree(tss_id);
}
