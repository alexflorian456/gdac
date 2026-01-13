#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "scheduler.h"

static scheduler_t scheduler;
void scheduler_signal_trampoline(void);

/* Dedicated context and stack for the scheduler trampoline. */
static ucontext_t scheduler_trampoline_ctx;
alignas(16) static uint8_t scheduler_trampoline_stack[STACK_SIZE];

void scheduler_init()
{
    scheduler.current_thread = 0;
    scheduler.thread_count = 1;
    scheduler.greenthreads[0].done = 0;
    scheduler.greenthreads[0].wait_for_join_handle = -1;

    /* Initialize trampoline context with its own stack so signal handler
     * can switch to a clean user context rather than relying on the
     * kernel signal frame. */
    getcontext(&scheduler_trampoline_ctx);
    scheduler_trampoline_ctx.uc_stack.ss_sp = scheduler_trampoline_stack;
    scheduler_trampoline_ctx.uc_stack.ss_size = STACK_SIZE;
    scheduler_trampoline_ctx.uc_link = NULL;
    makecontext(&scheduler_trampoline_ctx, (void (*)(void))scheduler_signal_trampoline, 0);
}

void scheduler_signal_handler(int sig)
{
    (void)sig;
    if (scheduler.thread_count == 0)
    {
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
    do
    {
        scheduler.current_thread = (scheduler.current_thread + 1) % scheduler.thread_count;
        handle_t current_wait_for_join = scheduler.greenthreads[scheduler.current_thread].wait_for_join_handle;
        current_thread_waits = 0;
        if (current_wait_for_join != -1)
        {
            if (!scheduler.greenthreads[current_wait_for_join].done)
            {
                current_thread_waits = 1;
            }
            else
            {
                scheduler.greenthreads[scheduler.current_thread].wait_for_join_handle = -1;
            }
        }
    } while (scheduler.greenthreads[scheduler.current_thread].done || current_thread_waits);

    /* Save interrupted context into cur->context and jump to trampoline */
    swapcontext(&cur->context, &scheduler_trampoline_ctx);
    /* When we resume here, another context has been restored; handler exits. */
}

void scheduler_signal_trampoline()
{
    BLOCK_SCHEDULER(
        setcontext(&scheduler.greenthreads[scheduler.current_thread].context);
        __builtin_unreachable(););
}

void thread_wrapper_function(thread_function_t function, void *args, greenthread_t *thread)
{
    function(args);

    BLOCK_SCHEDULER(
        thread->done = 1;);

    pause();
}

handle_t scheduler_create_thread(thread_function_t function, void *args, sigset_t old_set)
{
    if (scheduler.thread_count >= MAX_THREAD_COUNT)
    {
        return -1;
    }

    greenthread_t *thread = &scheduler.greenthreads[scheduler.thread_count];
    getcontext(&thread->context);
    thread->done = 0;
    thread->wait_for_join_handle = -1;

    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = STACK_SIZE;
    thread->context.uc_link = 0;
    thread->context.uc_sigmask = old_set;

    makecontext(&thread->context, (void (*)(void))thread_wrapper_function, 3, function, args, thread);

    scheduler.thread_count++;

    return scheduler.thread_count - 1;
}

handle_t scheduler_get_current_thread()
{
    return scheduler.current_thread;
}

void scheduler_join_thread(handle_t handle_current, handle_t handle_to_join)
{
    printf("Thread %d called join on thread %d\n", handle_current, handle_to_join);
    scheduler.greenthreads[handle_current].wait_for_join_handle = handle_to_join;
}
