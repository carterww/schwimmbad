#ifndef SCHW_THREAD_H
#define SCHW_THREAD_H

#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>

#include "job.h"

#ifdef __cplusplus
extern "C" {
#endif

// SHOULD BE IFNDEF WHEN IMPLEMENTED
#ifdef DISABLE_DYNAMIC_SIZING
#define THREAD_POOL_DYNAMIC_SIZING
#endif

struct thread {
  pthread_t tid;
  jid job_id;
};

struct thread_pool {
  pthread_mutex_t rwlock;
  struct thread *threads;
  uint32_t num_threads;
  uint32_t working_threads;
};

int thread_pool_init(struct thread_pool *pool, void *job_queue,
                     uint32_t num_threads);
int thread_pool_free(struct thread_pool *pool);

#ifdef THREAD_POOL_DYNAMIC_SIZING

int thread_pool_resize(struct thread_pool *pool, int32_t change);

#endif // THREAD_POOL_DYNAMIC_SIZING

#ifdef __cplusplus
}
#endif

#endif // SCHW_THREAD_H
