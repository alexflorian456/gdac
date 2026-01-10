#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "api.h"

void api_init() {
   scheduler_init();
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = scheduler_signal_handler;
    sa.sa_flags = SA_NODEFER | SA_SIGINFO | SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 10000; // 10 ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 10000; // 10 ms

    setitimer(ITIMER_REAL, &timer, NULL);
}

handle_t api_create_thread(thread_function_t function, void* args) {
    handle_t handle;
    BLOCK_SCHEDULER(
        handle = scheduler_create_thread(function, args, old_set);
    );
    return handle;
}

handle_t api_get_current_thread() {
    handle_t handle;
    BLOCK_SCHEDULER(
        handle = scheduler_get_current_thread();
    );
    return handle;
}

void api_join_thread(handle_t handle_to_join) {
    BLOCK_SCHEDULER(
        handle_t handle_current = scheduler_get_current_thread();
        scheduler_join_thread(handle_current, handle_to_join);
    );
    raise(SIGALRM);
}
