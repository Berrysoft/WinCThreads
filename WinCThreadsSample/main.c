#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

void check_return(int res)
{
    assert(res == thrd_success);
}

#define THREADS_COUNT 4

once_flag flag = ONCE_FLAG_INIT;
void print_string(void) { printf("Hello once!\n"); }

/*thread_local*/ int globalInt;
mtx_t mutex;

cnd_t cond;
mtx_t cond_mutex;

void free_print(void* p)
{
    free(p);
    printf("Freed: %p\n", p);
}

int thread_func(void* arg)
{
    printf(arg, thrd_current()._Id);

    int m = 5;
    while (m--)
    {
        check_return(mtx_lock(&mutex));

        int t = globalInt;
        check_return(thrd_sleep(&(struct timespec){ .tv_nsec = 10000000 }, NULL));
        globalInt = ++t;

        check_return(mtx_unlock(&mutex));
    }
    printf("globalInt == %d\n", globalInt);

    tss_t key;
    check_return(tss_create(&key, free_print));
    check_return(tss_set(key, malloc(sizeof(unsigned int))));
    *(unsigned int*)tss_get(key) = thrd_current()._Id;
    printf("TSS address: %p\tvalue: %u\n", tss_get(key), *(unsigned int*)tss_get(key));
    tss_delete(key);

    call_once(&flag, print_string);
    call_once(&flag, print_string);
    call_once(&flag, print_string);

    check_return(mtx_lock(&cond_mutex));
    check_return(cnd_wait(&cond, &cond_mutex));
    check_return(mtx_unlock(&cond_mutex));
    printf("Thread %u is going to exit.\n", thrd_current()._Id);

    thrd_exit(0);
}

int main()
{
    globalInt = 0;
    mtx_init(&mutex, mtx_plain);
    cnd_init(&cond);
    mtx_init(&cond_mutex, mtx_plain);

    thrd_t threads[THREADS_COUNT];
    for (int i = 0; i < THREADS_COUNT; i++)
    {
        check_return(thrd_create(&threads[i], thread_func, "Hello from thread %u!\n"));
    }
    printf("Hello from main!\n");

    check_return(cnd_signal(&cond));
    check_return(thrd_sleep(&(struct timespec){ .tv_sec = 1 }, NULL));
    check_return(cnd_signal(&cond));
    check_return(thrd_sleep(&(struct timespec){ .tv_sec = 1 }, NULL));
    check_return(cnd_broadcast(&cond));

    for (int i = 0; i < THREADS_COUNT; i++)
    {
        int res;
        check_return(thrd_join(threads[i], &res));
        printf("The thread %u exit with code %d.\n", threads[i]._Id, res);
    }

    mtx_destroy(&cond_mutex);
    cnd_destroy(&cond);
    mtx_destroy(&mutex);
    return 0;
}
