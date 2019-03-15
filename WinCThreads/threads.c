#include "threads.h"
#include <Windows.h>
#include <assert.h>
#include <stdlib.h>

#define _DTORS_COUNT (64 + 1024)

static tss_dtor_t _Dtors[_DTORS_COUNT];

typedef struct _Tss_dtor_node
{
    tss_dtor_t _Dtor;
    void* _Data;
    struct _Tss_dtor_node* _Next;
} _Tss_dtor_node;

static _Thread_local _Tss_dtor_node* _Dtor_head = NULL;
static _Thread_local _Tss_dtor_node* _Dtor_tail = NULL;

static void _Add_dtor(_In_ tss_dtor_t dtor, _In_opt_ void* data)
{
    _Tss_dtor_node* node = malloc(sizeof(_Tss_dtor_node));
    if (node)
    {
        node->_Dtor = dtor;
        node->_Data = data;
        node->_Next = NULL;
        if (!_Dtor_head)
            _Dtor_head = node;
        else
            _Dtor_tail->_Next = node;
        _Dtor_tail = node;
    }
}

typedef struct
{
    thrd_start_t _Func;
    void* _Arg;
} _Thrd_start_arg;

static DWORD WINAPI _Thrd_start(void* arg)
{
    _Add_dtor(free, arg);
    _Thrd_start_arg* start_arg = arg;
    int res = start_arg->_Func(start_arg->_Arg);
    thrd_exit(res);
}

int thrd_create(_Out_ thrd_t* thr, _In_ thrd_start_t func, _In_opt_ void* arg)
{
    thr->_Hnd = NULL;
    _Thrd_start_arg* start_arg = malloc(sizeof(_Thrd_start_arg));
    if (start_arg)
    {
        start_arg->_Func = func;
        start_arg->_Arg = arg;
        DWORD id;
        thr->_Hnd = CreateThread(NULL, 0, _Thrd_start, start_arg, 0, &id);
        thr->_Id = id;
    }
    if (!thr->_Hnd)
    {
        if (start_arg)
            free(start_arg);
        if (GetLastError() == ERROR_OUTOFMEMORY)
            return thrd_nomem;
        else
            return thrd_error;
    }
    return thrd_success;
}

thrd_t thrd_current(void)
{
    thrd_t t;
    t._Hnd = GetCurrentThread();
    t._Id = GetCurrentThreadId();
    return t;
}

int thrd_sleep(_In_ const struct timespec* duration, struct timespec* remaining)
{
    struct timespec t1;
    if (!timespec_get(&t1, TIME_UTC)) remaining = NULL;
    DWORD r = SleepEx((DWORD)duration->tv_sec * 1000 + duration->tv_nsec / 1000000, TRUE);
    if (!r) return 0;
    if (remaining)
    {
        struct timespec t2;
        if (timespec_get(&t2, TIME_UTC))
        {
            remaining->tv_sec = t2.tv_sec - t1.tv_sec - duration->tv_sec;
            remaining->tv_nsec = t2.tv_nsec - t1.tv_nsec - duration->tv_nsec;
            while (remaining->tv_nsec < 0)
            {
                remaining->tv_sec--;
                remaining->tv_nsec += 1000000000;
            }
        }
    }
    return r == WAIT_IO_COMPLETION ? -1 : -2;
}

__declspec(noreturn) void thrd_exit(_In_ int res)
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

static BOOL _Init_once_callback(PINIT_ONCE initOnce, PVOID parameter, PVOID* context)
{
    (void)initOnce;
    (void)context;
    void (*func)(void) = (void (*)(void))parameter;
    func();
    return TRUE;
}

void call_once(_In_ once_flag* flag, _In_ void (*func)(void))
{
    InitOnceExecuteOnce((PINIT_ONCE)flag, _Init_once_callback, (void*)func, NULL);
}

int tss_create(_Out_ tss_t* tss_key, _In_opt_ tss_dtor_t destructor)
{
    *tss_key = TlsAlloc();
    if (*tss_key == TLS_OUT_OF_INDEXES)
        return thrd_error;
    else
    {
        assert(*tss_key < _DTORS_COUNT);
        _Dtors[*tss_key] = destructor;
        return thrd_success;
    }
}

void* tss_get(_In_ tss_t tss_key)
{
    return TlsGetValue(tss_key);
}

int tss_set(_In_ tss_t tss_id, _In_opt_ void* val)
{
    if (TlsSetValue(tss_id, val))
        return thrd_success;
    else
        return thrd_error;
}

void tss_delete(_In_ tss_t tss_id)
{
    if (_Dtors[tss_id])
    {
        _Add_dtor(_Dtors[tss_id], TlsGetValue(tss_id));
    }
    TlsFree(tss_id);
}
