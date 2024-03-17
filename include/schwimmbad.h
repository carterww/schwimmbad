#ifndef SCHWIMMBAD_H
#define SCHWIMMBAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int schw_init();
void schw_free();

uint32_t schw_threads();
uint32_t schw_working_threads();

uint32_t schw_queue_len();
uint32_t schw_queue_cap();
uint32_t schw_queue_empty();

int schw_pool_resize(int32_t change);
int schw_queue_resize(int32_t change);

#ifdef SCHW_SCHED_FIFO
int schw_push(void *(*job_func)(void*), void *arg);
#endif

#ifdef SCHW_SCHED_PRIORITY
int schw_push(void *(*job_func)(void*), void *arg, int32_t priority);
#endif


#ifdef __cplusplus
}
#endif

#endif // SCHWIMMBAD_H
