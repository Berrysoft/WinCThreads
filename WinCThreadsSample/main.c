/**WinCThreadsSample main.c
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

// Used for debug
void check_return(int res)
{
    assert(res == thrd_success);
}

#define THREADS_COUNT 4

once_flag flag = ONCE_FLAG_INIT;
// This function will be called only once
void print_string(void) { printf("Hello once!\n"); }

int globalInt;
smph_t sem;

cnd_t cond;
mtx_t cond_mutex;

// Two different free functions
void free_print_1(void* p)
{
    printf("Free_1: %p\n", p);
    free(p);
}
void free_print_2(void* p)
{
    printf("Free_2: %p\n", p);
    free(p);
}

int thread_func(void* arg)
{
    int thrd_id = (int)(long long)arg;
    printf("Hello from thread %d!\n", thrd_id);

    int m = 5;
    while (m--)
    {
        // If not locked, you may get 5 in the end.
        // But actually we want 5 * THREADS_COUNT == 20 here.
        check_return(smph_wait(&sem));

        int t = globalInt;
        check_return(thrd_sleep(&(struct timespec){ .tv_nsec = 10000000 }, NULL));
        globalInt = ++t;

        check_return(smph_post(&sem));
    }
    printf("globalInt == %d\n", globalInt);

    tss_t key;
    check_return(tss_create(&key, globalInt % 2 != 0 ? free_print_1 : free_print_2));
    // The standard says that the data should be initialized to NULL.
    assert(tss_get(key) == NULL);
    void* data = malloc(sizeof(int));
    if (data)
    {
        check_return(tss_set(key, data));
        *(int*)tss_get(key) = thrd_id;
        printf("TSS address: %p\tvalue: %d\n", tss_get(key), *(int*)tss_get(key));
    }
    else
    {
        tss_delete(key);
    }

    call_once(&flag, print_string);

    // A RIGHT sample for condition variables
    check_return(mtx_slock(&cond_mutex));
    check_return(cnd_swait(&cond, &cond_mutex));
    check_return(mtx_sunlock(&cond_mutex));
    printf("Thread %d is going to exit.\n", thrd_id);

    // Return a specified code.
    return thrd_id * thrd_id;
}

int main()
{
    globalInt = 0;
    check_return(smph_init(&sem, 1, 0));
    check_return(cnd_init(&cond));
    check_return(mtx_init(&cond_mutex, mtx_shared | mtx_recursive));

    thrd_t threads[THREADS_COUNT] = { 0 };
    for (int i = 0; i < THREADS_COUNT; i++)
    {
        // Pass the threads their own id
        check_return(thrd_create(&threads[i], thread_func, (void*)(long long)i));
    }
    printf("Hello from main!\n");

    check_return(thrd_sleep(&(struct timespec){ .tv_sec = 1 }, NULL));
    check_return(smph_post(&sem));

    // Sleep and signal the threads
    check_return(thrd_sleep(&(struct timespec){ .tv_sec = 1 }, NULL));
    check_return(cnd_signal(&cond));
    check_return(thrd_sleep(&(struct timespec){ .tv_sec = 1 }, NULL));
    check_return(cnd_signal(&cond));
    check_return(thrd_sleep(&(struct timespec){ .tv_sec = 1 }, NULL));
    check_return(cnd_broadcast(&cond));

    // Wait for the threads
    for (int i = 0; i < THREADS_COUNT; i++)
    {
        int res;
        _Analysis_assume_(threads[i] != NULL);
        check_return(thrd_join(threads[i], &res));
        printf("The thread %d exit with code %d.\n", i, res);
    }

    mtx_destroy(&cond_mutex);
    cnd_destroy(&cond);
    smph_destroy(&sem);
    return 0;
}
