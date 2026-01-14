#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <signal.h>

#include "greenthread.h"
#include "utils.h"

#define MAX_THREAD_COUNT 128
#define MAX_MUTEX_COUNT 128

typedef struct scheduler_t {
    greenthread_t greenthreads[MAX_THREAD_COUNT];
    mutex_t mutexes[MAX_MUTEX_COUNT];
    uint32_t current_thread;
    uint32_t thread_count;
    uint32_t mutex_count;
} scheduler_t;

typedef void*(*thread_function_t)(void*);

void scheduler_init(void);

void scheduler_signal_handler(int sig);

greenthread_handle_t scheduler_create_thread(thread_function_t function, void* args, sigset_t old_set);

greenthread_handle_t scheduler_get_current_thread(void);

void scheduler_join_thread(greenthread_handle_t handle_current, greenthread_handle_t greenthread_handle_to_join);

void scheduler_exit_thread(greenthread_handle_t handle_current);

mutex_handle_t scheduler_create_mutex(void);

uint8_t scheduler_lock_mutex(mutex_handle_t mutex_handle);

void scheduler_unlock_mutex(mutex_handle_t mutex_handle);

#endif // SCHEDULER_H
