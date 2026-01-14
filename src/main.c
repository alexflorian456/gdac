#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "api.h"

static const uint64_t print_step = 10000000;

struct thread_1_args {
    int a;
    int b;
};

void *thread_1(void *args) {
    struct thread_1_args *targs = (struct thread_1_args *)args;
    uint64_t x = 0;
    uint64_t step = 0;
    while (x < 1000000000) {
        x += targs->a * targs->b;

        step++;
        if (step % print_step == 0) {
            printf("Thread 1 %lu\n", x);
        }
    }
    api_exit_thread();
    printf("Thread %d Done\n", api_get_current_thread().id);
    return NULL;
}

struct thread_2_args {
    int a;
    int b;
};

void *thread_2(void *args) {
    struct thread_2_args *targs = (struct thread_2_args *)args;
    uint64_t x = 0;
    uint64_t step = 0;
    while (x < 1000000000) {
        x += targs->a + targs->b;

        step++;
        if (step % print_step == 0) {
            printf("Thread 2 %lu\n", x);
        }
    }
    printf("Thread %d Done\n", api_get_current_thread().id);
    return NULL;
}

int main(void) {
    api_init();

    struct thread_1_args *a1 = malloc(sizeof(*a1));
    *a1 = (struct thread_1_args){.a = 2, .b = 3};
    greenthread_handle_t handle_1 = api_create_thread(thread_1, a1);

    struct thread_2_args *a2 = malloc(sizeof(*a2));
    *a2 = (struct thread_2_args){.a = 4, .b = 5};
    greenthread_handle_t handle_2 = api_create_thread(thread_2, a2);

    uint64_t x = 0;
    while (x < 1000000000) {
        x++;
        if (x % print_step == 0) {
            printf("Thread 0 %lu\n", x);
        }
    }

    api_join_thread(handle_1);
    printf("joined handle_1\n");
    api_join_thread(handle_2);
    printf("joined handle_2\n");

    printf("Main exit\n");
    return 0;
}
