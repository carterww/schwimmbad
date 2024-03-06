#include "thread.h"

#include <errno.h>
#include <stdlib.h>

#include "common.h"

int thread_pool_init(struct thread_pool *pool, uint32_t num_threads,
                     uint32_t max_threads) {
  struct thread *threads = malloc(num_threads * sizeof(struct thread));
  if (unlikely(threads == NULL)) {
    return errno;
  }
  for (uint32_t i = 0; i < num_threads - 1; ++i) {
    threads[i].next_free = &threads[i + 1];
  }
  threads[num_threads - 1].next_free = NULL;

  pthread_mutex_init(&pool->rwlock, NULL);
  pool->threads = threads;
  pool->num_threads = num_threads;
  pool->working_threads = 0;
  pool->max_threads = max_threads;

  return 0;
}

int thread_pool_free(struct thread_pool *pool) {
  pthread_mutex_lock(&pool->rwlock);
  if(pool->working_threads > 0) {
    pthread_mutex_unlock(&pool->rwlock);
    return EBUSY;
  }
  pthread_mutex_unlock(&pool->rwlock);
  pthread_mutex_destroy(&pool->rwlock);
  free(pool->threads);
  pool->threads = NULL;
  pool->num_threads = 0;
  pool->working_threads = 0;
  pool->max_threads = 0;
  return 0;
}

jid thread_run(struct thread_pool *pool, struct job *job) {}

#ifdef THREAD_POOL_DYNAMIC_SIZING

int thread_pool_increase(struct thread_pool *pool, uint32_t num_threads) {}

int thread_pool_decrease(struct thread_pool *pool, uint32_t num_threads) {}

#endif // THREAD_POOL_DYNAMIC_SIZING
