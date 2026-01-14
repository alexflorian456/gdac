#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>

typedef struct mutex_handle_t {
    int32_t id;
} mutex_handle_t;

typedef struct mutex_t {
    uint8_t is_locked;
} mutex_t;

#endif // MUTEX_H