#ifndef API_H
#define API_H

#include "scheduler.h"

void api_init(void);
greenthread_handle_t api_create_thread(thread_function_t function, void* args);
greenthread_handle_t api_get_current_thread(void);
void api_join_thread(greenthread_handle_t handle);
void api_exit_thread(void);

#endif // API_H
