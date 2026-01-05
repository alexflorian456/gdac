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
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &block_set, &old_set);

    handle_t handle = scheduler_create_thread(function, args, old_set);
    
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return handle;
}

handle_t api_get_current_thread() {
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &block_set, &old_set);

    handle_t handle = scheduler_get_current_thread();

    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return handle;
}

void api_join_thread(handle_t handle) {
    // TODO: Implement thread joining
}
