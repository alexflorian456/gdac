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
    printf("Thread %d Done\n", api_get_current_thread_id());
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
    printf("Thread %d Done\n", api_get_current_thread_id());
    return NULL;
}

int32_t g_sum;
int32_t g_array[1000];
mutex_handle_t g_mutex;

struct thread_sum_args {
    uint32_t start;
    uint32_t end;
};

void *thread_sum(void *args) {
    struct thread_sum_args *targs = (struct thread_sum_args*)args;
    uint32_t start = targs->start;
    uint32_t end = targs->end;
    int32_t local_sum = 0;
    for(uint32_t i = start; i <= end; i++) {
        local_sum += g_array[i];
    }
    printf("Thread %d local sum: %d", api_get_current_thread().id, local_sum);
    api_lock_mutex(g_mutex);
    g_sum += local_sum;
    api_unlock_mutex(g_mutex);
    return NULL;
}

void init_g_array(void) {
    for (int i = 0; i < 1000; i++) {
        g_array[i] = i + 1;
    }
}

int main(void) {
    api_init();
    init_g_array();

    g_mutex = api_create_mutex();

    struct thread_sum_args *a1 = malloc(sizeof(*a1));
    *a1 = (struct thread_sum_args){.start = 0, .end = 499};
    greenthread_handle_t handle_1 = api_create_thread(thread_sum, a1);

    struct thread_sum_args *a2 = malloc(sizeof(*a2));
    *a2 = (struct thread_sum_args){.start = 500, .end = 999};
    greenthread_handle_t handle_2 = api_create_thread(thread_sum, a2);

    api_join_thread(handle_1);
    printf("joined handle_1\n");
    api_join_thread(handle_2);
    printf("joined handle_2\n");

    printf("Main exit\n");
    return 0;
}
