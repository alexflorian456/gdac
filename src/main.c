#include <stdio.h>
#include <stdint.h>

#include "api.h"

struct thread_1_args {
    int a;
    int b;
};

void* thread_1(void* args) {
    struct thread_1_args* targs = (struct thread_1_args*)args;
    uint64_t x = 0;
    while (x < 10000000) {
        x+=targs->a*targs->b;
    }
    printf("Thread 1 Done\n");
    return NULL;
}

struct thread_2_args {
    int a;
    int b;
};

void* thread_2(void* args) {
    struct thread_2_args* targs = (struct thread_2_args*)args;
    uint64_t x = 0;
    while (x < 10000000) {
        x+=targs->a+targs->b;
    }
    printf("Thread 2 Done\n");
    return NULL;
}

int main() {
    api_init();

    api_create_thread(thread_1, &(struct thread_1_args){.a=2, .b=3});
    api_create_thread(thread_2, &(struct thread_2_args){.a=5, .b=7});

    uint64_t x = 0;
    while (x < 1000000000) {
        x++;
    }
    
    printf("Main exit\n");
    return 0;
}
