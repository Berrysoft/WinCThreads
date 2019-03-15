#include <stdio.h>
#include <threads.h>

static once_flag flag = ONCE_FLAG_INIT;

void print_string(void) { printf("Hello once!\n"); }

int thread_func(void* arg)
{
    printf(arg);
    call_once(&flag, print_string);
    call_once(&flag, print_string);
    call_once(&flag, print_string);
    return 0;
}

int main()
{
    thrd_t t;
    if (thrd_create(&t, thread_func, "Hello from thread!\n") == thrd_success)
    {
        printf("Hello from main!\n");
        int res;
        thrd_join(t, &res);
        printf("The thread exit with code %d.\n", res);
    }
    return 0;
}
