#ifndef SCHWIMMBAD_H
#define SCHWIMMBAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t jid;

// Default scheduling policy will be FIFO
#ifndef SCHW_SCHED_PRIORITY
#define SCHW_SCHED_FIFO
#else
#define SCHW_SCHED_PRIORITY
#endif


int schw_init(uint32_t num_threads, uint32_t queue_len);
void schw_free();

uint32_t schw_threads();
uint32_t schw_working_threads();

uint32_t schw_queue_len();
uint32_t schw_queue_cap();

int schw_pool_resize(int32_t change);
int schw_queue_resize(int32_t change);

#ifdef SCHW_SCHED_FIFO
jid schw_push(void *(*job_func)(void*), void *arg);
#endif

#ifdef SCHW_SCHED_PRIORITY
jid schw_push(void *(*job_func)(void*), void *arg, int32_t priority);
#endif

#ifdef __cplusplus
}
#endif

#endif // SCHWIMMBAD_H
