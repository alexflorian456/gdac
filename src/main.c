#include <stdio.h>
#include <stdint.h>

#include "api.h"

void thread_1() {
    uint64_t x = 0;
    while (x < 10000000) {
        if (x % 1000000 == 0) {
            printf("Thread 1 %ld\n", x);
        }
        x++;
    }
    printf("Thread 1 Done\n");
}

void thread_2() {
    uint64_t x = 0;
    while (x < 10000000) {
        if (x % 1000000 == 0) {
            printf("Thread 2 %ld\n", x);
        }
        x++;
    }
    printf("Thread 2 Done\n");
}

int main() {
    api_init();

    api_create_thread(thread_1);
    api_create_thread(thread_2);

    uint64_t x = 0;
    while (x < 1000000000) {
        if (x % 1000000 == 0) {
            printf("Main thread %ld\n", x);
        }
        x++;
    }
    
    printf("Main exit\n");
    return 0;
}
