#define _GNU_SOURCE
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "scheduler.h"

static scheduler_t scheduler;

void scheduler_init() {
    scheduler.current_thread = 0;
    scheduler.thread_count = 1;
}

void scheduler_signal_handler(int sig, siginfo_t* si, void* ucontext) {
    (void)sig;
    (void)si;
    if (scheduler.thread_count == 0) {
        return;
    }
    scheduler.greenthreads[scheduler.current_thread].context = *(ucontext_t*)ucontext;
    scheduler.current_thread = (scheduler.current_thread + 1) % scheduler.thread_count;
    if (!scheduler.greenthreads[scheduler.current_thread].done){
        setcontext(&scheduler.greenthreads[scheduler.current_thread].context);
    }
}

void thread_wrapper_function(void (*function)(void), greenthread_t* thread) {
    function();
    thread->done = 1;
    pause();
}

void scheduler_create_thread(void (*function)(void)) {
    if (scheduler.thread_count >= MAX_THREAD_COUNT) {
        return;
    }

    greenthread_t* thread = &scheduler.greenthreads[scheduler.thread_count];
    getcontext(&thread->context);
    thread->done = 0;

    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = STACK_SIZE;
    thread->context.uc_link = 0;

    makecontext(&thread->context, (void(*)(void))thread_wrapper_function, 2, function, thread);

    scheduler.thread_count++;
}
