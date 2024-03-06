#ifndef SCHW_THREAD_H
#define SCHW_THREAD_H

#include <pthread.h>
#include <stdint.h>

#include "job.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DISABLE_DYNAMIC_SIZING
#define THREAD_POOL_DYNAMIC_SIZING
#endif

struct thread {
  pthread_t tid;
  union {
    struct {
      jid job_id;
    };
    struct thread *next_free;
  };
};

struct thread_pool {
  pthread_mutex_t rwlock;
  struct thread *threads;
  struct thread *first_free;
  uint32_t num_threads;
  uint32_t working_threads;
  uint32_t max_threads;
};

inline int thread_pool_init(struct thread_pool *pool, uint32_t num_threads,
                            uint32_t max_threads);
int thread_pool_free(struct thread_pool *pool);

jid thread_run(struct thread_pool *pool, struct job *job);


#ifdef THREAD_POOL_DYNAMIC_SIZING

int thread_pool_increase(struct thread_pool *pool, uint32_t num_threads);
int thread_pool_decrease(struct thread_pool *pool, uint32_t num_threads);

#endif // THREAD_POOL_DYNAMIC_SIZING

#ifdef __cplusplus
}
#endif

#endif // SCHW_THREAD_H
