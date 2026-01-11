#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>

#include "scheduler.h"

static scheduler_t scheduler;

void scheduler_init() {
    scheduler.current_thread = 0;
    scheduler.thread_count = 1;
    scheduler.greenthreads[0].done = 0;
    scheduler.greenthreads[0].wait_for_join_handle = -1;
}

void scheduler_signal_handler(int sig, siginfo_t* si, void* ucontext) {
    (void)sig;
    (void)si;
    if (scheduler.thread_count == 0) {
        return;
    }

    uint32_t old_thread_idx = scheduler.current_thread;
    if (!scheduler.greenthreads[scheduler.current_thread].done) {
        // Save interrupted context
        scheduler.greenthreads[scheduler.current_thread].context = *(ucontext_t*)ucontext;
    }

    // Increment context
    uint8_t current_thread_waits = 0;
    do {
        scheduler.current_thread = (scheduler.current_thread + 1) % scheduler.thread_count;
        // // Handle wait for join
        // handle_t current_wait_for_join = scheduler.greenthreads[scheduler.current_thread].wait_for_join_handle;
        // current_thread_waits = 0;
        // if (current_wait_for_join != -1) {
        //     if (!scheduler.greenthreads[current_wait_for_join].done) {
        //         current_thread_waits = 1;
        //     } else {
        //         scheduler.greenthreads[scheduler.current_thread].wait_for_join_handle = -1;
        //     }
        // }
    } while (scheduler.greenthreads[scheduler.current_thread].done ||
             current_thread_waits);

    printf("Thread picked: %d\n", scheduler.current_thread);

    // Only switch if interrupted context differs from next context
    // Otherwise: SEGFAULT (for some reason)
    if (old_thread_idx != scheduler.current_thread) {
        setcontext(&scheduler.greenthreads[scheduler.current_thread].context);
    } else {
        return;
    }
}

void thread_wrapper_function(thread_function_t function, void* args, greenthread_t* thread) {
    function(args);

    BLOCK_SCHEDULER(
        thread->done = 1;
    );

    pause();
}

handle_t scheduler_create_thread(thread_function_t function, void* args, sigset_t old_set) {
    if (scheduler.thread_count >= MAX_THREAD_COUNT) {
        return -1;
    }

    greenthread_t* thread = &scheduler.greenthreads[scheduler.thread_count];
    getcontext(&thread->context);
    thread->done = 0;
    thread->wait_for_join_handle = -1;

    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = STACK_SIZE;
    thread->context.uc_link = 0;
    thread->context.uc_sigmask = old_set;

    makecontext(&thread->context, (void(*)(void))thread_wrapper_function, 3, function, args, thread);

    scheduler.thread_count++;

    return scheduler.thread_count - 1;
}

handle_t scheduler_get_current_thread() {
    return scheduler.current_thread;
}

void scheduler_join_thread(handle_t handle_current, handle_t handle_to_join) {
    printf("Thread %d called join on thread %d\n", handle_current, handle_to_join);
    scheduler.greenthreads[handle_current].wait_for_join_handle = handle_to_join;
}
