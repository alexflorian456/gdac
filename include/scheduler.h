#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <signal.h>

#include "greenthread.h"
#include "utils.h"

#define MAX_THREAD_COUNT 128

typedef struct scheduler_t {
    greenthread_t greenthreads[MAX_THREAD_COUNT];
    uint32_t current_thread;
    uint32_t thread_count;
} scheduler_t;

typedef void*(*thread_function_t)(void*);

void scheduler_init(void);

void scheduler_signal_handler(int sig);

handle_t scheduler_create_thread(thread_function_t function, void* args, sigset_t old_set);

handle_t scheduler_get_current_thread(void);

void scheduler_join_thread(handle_t handle_current, handle_t handle_to_join);

#endif // SCHEDULER_H
