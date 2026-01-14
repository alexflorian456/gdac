#define _XOPEN_SOURCE 500
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "api.h"

alignas(16) static uint8_t sig_stack[STACK_SIZE];

void api_init() {
    scheduler_init();

    stack_t ss;
    ss.ss_sp = sig_stack;
    ss.ss_size = sizeof(sig_stack);
    ss.ss_flags = 0;

    if (sigaltstack(&ss, NULL) == -1) {
        perror("sigaltstack");
        exit(1);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = scheduler_signal_handler;
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 10000; // 10 ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 10000; // 10 ms

    setitimer(ITIMER_REAL, &timer, NULL);
}

greenthread_handle_t api_create_thread(thread_function_t function, void *args) {
    greenthread_handle_t handle;
    BLOCK_SCHEDULER(
        handle = scheduler_create_thread(function, args, old_set););

    return handle;
}

greenthread_handle_t api_get_current_thread() {
    greenthread_handle_t handle;
    BLOCK_SCHEDULER(
        handle = scheduler_get_current_thread(););

    return handle;
}

void api_join_thread(greenthread_handle_t greenthread_handle_to_join) {
    BLOCK_SCHEDULER(
        greenthread_handle_t handle_current = scheduler_get_current_thread();
        scheduler_join_thread(handle_current, greenthread_handle_to_join););

    pause();
}

void api_exit_thread() {
    BLOCK_SCHEDULER(
        greenthread_handle_t handle_current = scheduler_get_current_thread();
        scheduler_exit_thread(handle_current););

    pause();
}
