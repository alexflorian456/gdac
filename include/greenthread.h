#include <ucontext.h>

#define STACK_SIZE 8192

typedef struct greenthread_t {
    ucontext_t context;
    char stack[STACK_SIZE];
    uint8_t done;
} greenthread_t;
