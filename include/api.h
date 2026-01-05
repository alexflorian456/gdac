#ifndef API_H
#define API_H

#include "scheduler.h"

void api_init(void);
handle_t api_create_thread(thread_function_t function, void* args);
handle_t api_get_current_thread(void);
void api_join_thread(handle_t handle);

#endif // API_H
