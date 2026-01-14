#ifndef API_H
#define API_H

#include "scheduler.h"

void api_init(void);

greenthread_handle_t api_create_thread(thread_function_t function, void* args);
greenthread_handle_t api_get_current_thread(void);
int32_t api_get_current_thread_id(void);
void api_join_thread(greenthread_handle_t thread_handle);
void api_exit_thread(void);

mutex_handle_t api_create_mutex(void);
void api_lock_mutex(mutex_handle_t mutex_handle);
void api_unlock_mutex(mutex_handle_t mutex_handle);

sem_handle_t api_create_sem(uint8_t value);
void api_wait_sem(sem_handle_t sem_handle);
void api_post_sem(sem_handle_t sem_handle);
void api_destroy_sem(sem_handle_t sem_handle);

void api_deadlock_report(int sig);

#endif // API_H
