#ifndef API_H
#define API_H

#include "scheduler.h"

void api_init(void);
handle_t api_create_thread(thread_function_t function, void* args);

#endif // API_H
