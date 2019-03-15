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
#include <Windows.h>
#include <assert.h>
#include <stdlib.h>

// TLS counts is limited to 1088 = 64 + 1024
#define _DTORS_COUNT (64 + 1024)

// An array of all the destructors
static tss_dtor_t _Dtors[_DTORS_COUNT];

// A list of destructors and data to delete.
typedef struct _Tss_dtor_node
{
    tss_dtor_t _Dtor;
    void* _Data;
    struct _Tss_dtor_node* _Next;
} _Tss_dtor_node;

static thread_local _Tss_dtor_node* _Dtor_head = NULL;

// Add a destructor to the list with its data
static void _Add_dtor(_In_ tss_dtor_t dtor, _In_opt_ void* data)
{
    if (data)
    {
        _Tss_dtor_node* node = malloc(sizeof(_Tss_dtor_node));
        if (node)
        {
            node->_Dtor = dtor;
            node->_Data = data;
            node->_Next = _Dtor_head;
            _Dtor_head = node;
        }
    }
}

// Call all destructors
static void _Tss_clear()
{
    _Tss_dtor_node* current = _Dtor_head;
    while (current)
    {
        _Tss_dtor_node* next = current->_Next;
        current->_Dtor(current->_Data);
        free(current);
        current = next;
    }
}

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

// Clear all remained data TSS_DTOR_ITERATIONS times.
static void _Tss_clear_all()
{
    for (int i = 0, again = TRUE; i < TSS_DTOR_ITERATIONS && again; i++)
    {
        again = FALSE;
        for (_Tss_dtor_id_node* current = _All_dtor_head; current; current = current->_Next)
        {
            if (_Dtors[current->_Key])
            {
                void* value = TlsGetValue(current->_Key);
                if (value)
                {
                    TlsSetValue(current->_Key, NULL);
                    again = TRUE;
                    _Dtors[current->_Key](TlsGetValue(current->_Key));
                }
            }
        }
    }
    _Tss_dtor_id_node* current = _All_dtor_head;
    while (current)
    {
        _Tss_dtor_id_node* next = current->_Next;
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
    // Clear all data before exit
    _Tss_clear();
    _Tss_clear_all();
    ExitThread((DWORD)res);
}

static BOOL _Init_once_callback(PINIT_ONCE initOnce, PVOID parameter, PVOID* context)
{
    // Ignore some params
    (void)initOnce;
    (void)context;
    // Call the function
    void (*func)(void) = (void (*)(void))parameter;
    func();
    return TRUE;
}

void call_once(_In_ once_flag* flag, _In_ void (*func)(void))
{
    BOOL r = InitOnceExecuteOnce((PINIT_ONCE)flag, _Init_once_callback, (void*)func, NULL);
    assert(r);
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
        if (destructor) _Add_dtor_id(*tss_key);
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
        _Dtors[tss_id] = NULL;
    }
    BOOL r = TlsFree(tss_id);
    assert(r);
}
