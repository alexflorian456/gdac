#include <signal.h>
#include <stddef.h>

#include "api.h"
#include "scheduler.h"

static scheduler_t scheduler;

void api_init() {
    scheduler_init(&scheduler);
    
}