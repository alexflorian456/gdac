#include <stdint.h>

#include "greenthread.h"

#define MAX_THREAD_COUNT 128

typedef struct scheduler_t {
    greenthread_t greenthreads[MAX_THREAD_COUNT];
    uint32_t current_thread;
    uint32_t thread_count;
} scheduler_t;

void scheduler_init();

void scheduler_signal_handler(int sig, siginfo_t* si, void* ucontext);

void scheduler_create_thread(void (*function)(void));
