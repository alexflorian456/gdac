#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

typedef struct sem_handle_t {
    int32_t id;
} sem_handle_t;

typedef struct sem_t {
    uint8_t value;
    uint8_t valid;
} sem_t;

#endif // SEMAPHORE_H