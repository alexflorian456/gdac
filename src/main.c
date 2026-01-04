#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "scheduler.h"

void thread_1() {
    uint64_t x = 0;
    while (x < 10000000) {
        if (x % 1000000 == 0) {
            printf("Thread 1 %ld\n", x);
        }
        x++;
    }
}

void thread_2() {
    uint64_t x = 0;
    while (x < 10000000) {
        if (x % 1000000 == 0) {
            printf("Thread 2 %ld\n", x);
        }
        x++;
    }
}

int main() {
    scheduler_init();

    scheduler_create_thread(thread_1);
    scheduler_create_thread(thread_2);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = scheduler_signal_handler;
    sa.sa_flags = SA_NODEFER & SA_SIGINFO;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 10000; // 10 ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 10000; // 10 ms

    setitimer(ITIMER_REAL, &timer, NULL);
    pause();
    printf("Main exit\n");
    return 0;
}
