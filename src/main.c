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
    printf("Thread %d Done\n", api_get_current_thread());
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
    printf("Thread %d Done\n", api_get_current_thread());
    return NULL;
}

int main(void) {
    api_init();

    handle_t handle_1 = api_create_thread(thread_1, &(struct thread_1_args){.a=2, .b=3});
    handle_t handle_2 = api_create_thread(thread_2, &(struct thread_2_args){.a=5, .b=7});

    uint64_t x = 0;
    while (x < 1000000000) {
        x++;
    }

    
    
    printf("Main exit\n");
    return 0;
}
