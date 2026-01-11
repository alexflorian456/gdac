#ifndef GREENTHREAD_H
#define GREENTHREAD_H

#include <ucontext.h>
#define STACK_SIZE (128 * 1024)

typedef int32_t handle_t;

typedef struct greenthread_t {
    ucontext_t context;
    char stack[STACK_SIZE];
    uint8_t done;
    handle_t wait_for_join_handle;
} greenthread_t;

#endif // GREENTHREAD_H
