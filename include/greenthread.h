#ifndef GREENTHREAD_H
#define GREENTHREAD_H

#include <ucontext.h>
#include <stdalign.h>

#include "mutex.h"

#define STACK_SIZE (128 * 1024)

typedef struct greenthread_handle_t {
    int32_t id;
} greenthread_handle_t;

typedef struct greenthread_t {
    ucontext_t context;
    alignas(16) uint8_t stack[STACK_SIZE];
    uint8_t done;
    greenthread_handle_t wait_for_join_handle;

    // used only for deadlock detection, not for proper scheduling
    mutex_handle_t acquired_mutex_handle;
    greenthread_handle_t wait_for_mutex_handle;
} greenthread_t;

#endif // GREENTHREAD_H
