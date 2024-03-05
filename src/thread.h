#ifndef SCHW_THREAD_H
#define SCHW_THREAD_H

#include <pthread.h>
#include <stdint.h>

#include "job.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THREAD_POOL_DYNAMIC_SIZING

struct thread {
  union {
    struct {
      pthread_t tid;
    };
    struct thread *next_free;
  };
};

struct thread_pool {
  struct thread *threads;
  struct thread *first_free;
  pthread_mutex_t rwlock;
  uint32_t num_threads;
  uint32_t working_threads;
  uint32_t max_threads;
};

inline int thread_pool_init(struct thread_pool *pool, uint32_t num_threads,
                            uint32_t max_threads);
void thread_pool_free(struct thread_pool *pool);

int thread_pool_increase(struct thread_pool *pool, uint32_t num_threads);
int thread_pool_decrease(struct thread_pool *pool, uint32_t num_threads);

pthread_t thread_run(struct thread_pool *pool, struct job *job);
int thread_cancel(struct thread_pool *pool, pthread_t tid);

#ifdef __cplusplus
}
#endif

#endif // SCHW_THREAD_H
