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
            printf("Thread %d %lu\n", api_get_current_thread_id(), x);
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
            printf("Thread %d %lu\n", api_get_current_thread_id(), x);
        }
    }
    printf("Thread %d Done\n", api_get_current_thread_id());
    return NULL;
}

uint64_t g_sum;
uint64_t *g_array;
mutex_handle_t g_mutex;

struct thread_sum_args {
    uint64_t start;
    uint64_t end;
};

void *thread_sum(void *args) {
    struct thread_sum_args *targs = (struct thread_sum_args*)args;
    uint64_t start = targs->start;
    uint64_t end = targs->end;
    uint64_t local_sum = 0;
    for(uint64_t i = start; i <= end; i++) {
        local_sum += g_array[i];
    }
    printf("Thread %d local sum: %ld\n", api_get_current_thread().id, local_sum);
    api_lock_mutex(g_mutex);
    for (uint64_t x = 0; x < 1000000000; x++) {
        if (x % print_step == 0) {
            printf("Thread %d %lu\n", api_get_current_thread_id(), x);
        }
    }
    g_sum += local_sum;
    api_unlock_mutex(g_mutex);
    printf("Thread %d Done\n", api_get_current_thread_id());
    return NULL;
}

void init_g_array(uint64_t n) {
    g_array = (uint64_t*)malloc(n * sizeof(uint64_t));
    for (uint64_t i = 0; i < n; i++) {
        g_array[i] = i + 1;
    }
}

int main(void) {
    uint64_t n = 1000000;
    api_init();
    init_g_array(n);

    g_mutex = api_create_mutex();

    struct thread_sum_args *a1 = malloc(sizeof(*a1));
    *a1 = (struct thread_sum_args){.start = 0, .end = n/2-1};
    greenthread_handle_t handle_1 = api_create_thread(thread_sum, a1);

    struct thread_sum_args *a2 = malloc(sizeof(*a2));
    *a2 = (struct thread_sum_args){.start = n/2, .end = n-1};
    greenthread_handle_t handle_2 = api_create_thread(thread_sum, a2);

    api_join_thread(handle_1);
    api_join_thread(handle_2);

    (void)handle_1;
    (void)handle_2;
    
    printf("Global sum: %ld\n", g_sum);
    
    printf("Main exit\n");
    return 0;
}
