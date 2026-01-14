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
    scheduler.greenthreads[0].acquired_mutex_handle.id = -1;
    scheduler.greenthreads[0].wait_for_mutex_handle.id = -1;

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
    greenthread_t *current = &scheduler.greenthreads[scheduler.current_thread];

    /* Ensure greenthread's stack metadata is correct so other code can
     * inspect it if needed. Overwriting a finished thread's context is
     * harmless here and simplifies logic. */
    current->context.uc_stack.ss_sp = current->stack;
    current->context.uc_stack.ss_size = STACK_SIZE;
    current->context.uc_link = NULL;

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

    /* Save interrupted context into current->context and jump to trampoline */
    swapcontext(&current->context, &scheduler_trampoline_ctx);
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
        fprintf(stderr, "Max thread count reached\n");
        exit(1);
    }

    greenthread_t *thread = &scheduler.greenthreads[scheduler.thread_count];
    getcontext(&thread->context);
    thread->done = 0;
    thread->wait_for_join_handle.id = -1;
    thread->acquired_mutex_handle.id = -1;
    thread->wait_for_mutex_handle.id = -1;

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
        fprintf(stderr, "Max mutex count reached\n");
        exit(1);
    }
    ret.id = scheduler.mutex_count;
    scheduler.mutexes[ret.id].is_locked = 0;
    scheduler.mutex_count++;
    return ret;
}

uint8_t scheduler_lock_mutex(mutex_handle_t mutex_handle, greenthread_handle_t current_handle) {
    int32_t mutex_idx = mutex_handle.id;
    if (scheduler.mutexes[mutex_idx].is_locked) {
        for (uint32_t i = 0; i < scheduler.thread_count; i++) {
            if (scheduler.greenthreads[i].acquired_mutex_handle.id == mutex_handle.id) {
                scheduler.greenthreads[current_handle.id].wait_for_mutex_handle.id = i;
            }
        }
        return 0;
    }
    scheduler.mutexes[mutex_idx].is_locked = 1;
    scheduler.greenthreads[current_handle.id].acquired_mutex_handle.id = mutex_handle.id;
    return 1;
}

void scheduler_unlock_mutex(mutex_handle_t mutex_handle, greenthread_handle_t current_handle) {
    int32_t mutex_idx = mutex_handle.id;
    if (!scheduler.mutexes[mutex_idx].is_locked) {
        fprintf(stderr, "Double unlock on mutex called\n");
        exit(1);
    }
    scheduler.mutexes[mutex_idx].is_locked = 0;
    scheduler.greenthreads[current_handle.id].acquired_mutex_handle.id = -1;
}

#define WHITE 0
#define GRAY  1
#define BLACK 2

/* get thread index from handle */
static inline int thread_index_from_handle(greenthread_handle_t h) {
    return (int)h.id;
}

/* DFS with cycle path tracking */
static int dfs_detect_cycle(int t, uint8_t *color, int *stack, int depth) {
    color[t] = GRAY;
    stack[depth] = t;

    greenthread_t *gt = &scheduler.greenthreads[t];

    /* Skip completed threads */
    if (!gt->done) {
        /* 1. Join dependency */
        if (gt->wait_for_join_handle.id != -1) {
            int next = thread_index_from_handle(gt->wait_for_join_handle);

            if (color[next] == GRAY) {
                /* cycle detected: print the cycle */
                fprintf(stderr, "Deadlock cycle detected: ");
                int i;
                /* print from first occurrence of `next` in stack */
                for (i = 0; i <= depth; i++) {
                    if (stack[i] == next) break;
                }
                for (; i <= depth; i++) {
                    fprintf(stderr, "%d -> ", stack[i]);
                }
                fprintf(stderr, "%d\n", next);
                return 1;
            }

            if (color[next] == WHITE &&
                dfs_detect_cycle(next, color, stack, depth + 1))
                return 1;
        }

        /* 2. Mutex dependency (already mapped to thread handle) */
        if (gt->wait_for_mutex_handle.id != -1) {
            int next = thread_index_from_handle(gt->wait_for_mutex_handle);

            if (color[next] == GRAY) {
                /* cycle detected: print the cycle */
                fprintf(stderr, "Deadlock cycle detected: ");
                int i;
                for (i = 0; i <= depth; i++) {
                    if (stack[i] == next) break;
                }
                for (; i <= depth; i++) {
                    fprintf(stderr, "%d -> ", stack[i]);
                }
                fprintf(stderr, "%d\n", next);
                return 1;
            }

            if (color[next] == WHITE &&
                dfs_detect_cycle(next, color, stack, depth + 1))
                return 1;
        }
    }

    color[t] = BLACK;
    return 0;
}

void scheduler_deadlock_report(void) {
    uint8_t color[MAX_THREAD_COUNT] = {0};
    int stack[MAX_THREAD_COUNT];

    for (uint32_t i = 0; i < scheduler.thread_count; i++) {
        if (color[i] == WHITE) {
            if (dfs_detect_cycle((int)i, color, stack, 0)) {
                exit(1);
            }
        }
    }

    fprintf(stderr, "No deadlock detected\n");
    exit(0);
}
