#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "scheduler.h"

static scheduler_t scheduler;
void scheduler_signal_trampoline(void);

/* Dedicated context and stack for the scheduler trampoline. */
static ucontext_t scheduler_trampoline_ctx;
alignas(16) static uint8_t scheduler_trampoline_stack[STACK_SIZE];

void scheduler_init() {
    // Initialize thread data
    scheduler.current_thread = 0;
    scheduler.thread_count = 1;
    scheduler.greenthreads[0].done = 0;
    scheduler.greenthreads[0].wait_for_join_handle.id = -1;
    scheduler.greenthreads[0].wait_for_mutex_lock_handle.id = -1;

    // Initialize mutex data
    scheduler.mutex_count = 0;

    /* Initialize trampoline context with its own stack so signal handler
     * can switch to a clean user context rather than relying on the
     * kernel signal frame. */
    getcontext(&scheduler_trampoline_ctx);
    scheduler_trampoline_ctx.uc_stack.ss_sp = scheduler_trampoline_stack;
    scheduler_trampoline_ctx.uc_stack.ss_size = STACK_SIZE;
    scheduler_trampoline_ctx.uc_link = NULL;
    makecontext(&scheduler_trampoline_ctx, (void (*)(void))scheduler_signal_trampoline, 0);
}

void scheduler_signal_handler(int sig) {
    (void)sig;
    if (scheduler.thread_count == 0) {
        return;
    }
    /* Save interrupted context (into the greenthread slot) and switch to
     * the trampoline which will perform the setcontext to the selected
     * next greenthread. We must determine the next thread to run before
     * switching, because the trampoline will use scheduler.current_thread. */
    greenthread_t *cur = &scheduler.greenthreads[scheduler.current_thread];

    /* Ensure greenthread's stack metadata is correct so other code can
     * inspect it if needed. Overwriting a finished thread's context is
     * harmless here and simplifies logic. */
    cur->context.uc_stack.ss_sp = cur->stack;
    cur->context.uc_stack.ss_size = STACK_SIZE;
    cur->context.uc_link = NULL;

    /* Increment context to pick the next runnable thread */
    uint8_t current_thread_waits = 0;
    do {
        scheduler.current_thread = (scheduler.current_thread + 1) % scheduler.thread_count;
        greenthread_handle_t current_wait_for_join = scheduler.greenthreads[scheduler.current_thread].wait_for_join_handle;
        current_thread_waits = 0;
        if (current_wait_for_join.id != -1) {
            if (!scheduler.greenthreads[current_wait_for_join.id].done) {
                current_thread_waits = 1;
            }
            else {
                scheduler.greenthreads[scheduler.current_thread].wait_for_join_handle.id = -1;
            }
        }
    } while (scheduler.greenthreads[scheduler.current_thread].done || current_thread_waits);

    /* Save interrupted context into cur->context and jump to trampoline */
    swapcontext(&cur->context, &scheduler_trampoline_ctx);
    /* When we resume here, another context has been restored; handler exits. */
}

void scheduler_signal_trampoline() {
    BLOCK_SCHEDULER(
        setcontext(&scheduler.greenthreads[scheduler.current_thread].context);
        __builtin_unreachable(););
}

void thread_wrapper_function(thread_function_t function, void *args, greenthread_t *thread) {
    function(args);

    BLOCK_SCHEDULER(
        thread->done = 1;);

    pause();
}

greenthread_handle_t scheduler_create_thread(thread_function_t function, void *args, sigset_t old_set) {
    greenthread_handle_t ret;
    if (scheduler.thread_count >= MAX_THREAD_COUNT) {
        perror("Max thread count reached");
        exit(1);
    }

    greenthread_t *thread = &scheduler.greenthreads[scheduler.thread_count];
    getcontext(&thread->context);
    thread->done = 0;
    thread->wait_for_join_handle.id = -1;

    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = STACK_SIZE;
    thread->context.uc_link = 0;
    thread->context.uc_sigmask = old_set;

    makecontext(&thread->context, (void (*)(void))thread_wrapper_function, 3, function, args, thread);

    scheduler.thread_count++;

    ret.id = scheduler.thread_count - 1;
    return ret;
}

greenthread_handle_t scheduler_get_current_thread() {
    greenthread_handle_t ret;
    ret.id = scheduler.current_thread;
    return ret;
}

void scheduler_join_thread(greenthread_handle_t handle_current, greenthread_handle_t greenthread_handle_to_join) {
    scheduler.greenthreads[handle_current.id].wait_for_join_handle = greenthread_handle_to_join;
}

void scheduler_exit_thread(greenthread_handle_t handle_current) {
    scheduler.greenthreads[handle_current.id].done = 1;
}

mutex_handle_t scheduler_create_mutex() {
    mutex_handle_t ret;
    if (scheduler.mutex_count >= MAX_MUTEX_COUNT) {
        perror("Max mutex count reached");
        exit(1);
    }
    ret.id = scheduler.mutex_count;
    scheduler.mutexes[ret.id].is_locked = 0;
    scheduler.mutex_count++;
    return ret;
}

uint8_t scheduler_lock_mutex(mutex_handle_t mutex_handle) {
    int32_t mutex_idx = mutex_handle.id;
    if (scheduler.mutexes[mutex_idx].is_locked) {
        return 0;
    }
    scheduler.mutexes[mutex_idx].is_locked = 1;
    return 1;
}

void scheduler_unlock_mutex(mutex_handle_t mutex_handle) {
    int32_t mutex_idx = mutex_handle.id;
    if (!scheduler.mutexes[mutex_idx].is_locked) {
        perror("Double unlock on mutex called");
        exit(1);
    }
    scheduler.mutexes[mutex_idx].is_locked = 0;
}
