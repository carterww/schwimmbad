#include "thread.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "job.h"

static void *start_thread(void *arg);

struct start_thread_arg {
  void *job_queue;
  struct thread *thread;
  struct thread_pool *pool;
};

int thread_pool_init(struct thread_pool *pool, void *job_queue,
                     uint32_t num_threads) {

  struct thread *threads = malloc(num_threads * sizeof(struct thread));
  if (unlikely(threads == NULL)) {
    return errno;
  }

  pthread_mutex_init(&pool->rwlock, NULL);
  pool->threads = threads;
  pool->num_threads = num_threads;
  pool->working_threads = 0;

  int err = 0;
  for (; threads != pool->threads + num_threads; threads++) {
    struct start_thread_arg *arg = malloc(sizeof(struct start_thread_arg));
    arg->job_queue = job_queue;
    arg->pool = pool;
    arg->thread = threads;
    if (arg == NULL) {
      err = errno;
      break;
    }
    err = pthread_create(&threads->tid, NULL, start_thread, (void *)arg);
    if (err != 0)
      break;
    err = pthread_detach(threads->tid);
    if (err != 0)
      break;
  }
  if (err != 0) {
    thread_pool_free(pool);
    return err;
  }
  return 0;
}

int thread_pool_free(struct thread_pool *pool) {
  pthread_mutex_lock(&pool->rwlock);
  if (pool->working_threads > 0) {
    pthread_mutex_unlock(&pool->rwlock);
    return EBUSY;
  }

  for (;pool->threads < pool->threads + pool->num_threads; ++pool->threads) {
    pthread_cancel(pool->threads->tid);
  }

  pthread_mutex_unlock(&pool->rwlock);
  pthread_mutex_destroy(&pool->rwlock);
  free(pool->threads);
  pool->threads = NULL;
  pool->num_threads = 0;
  pool->working_threads = 0;
  return 0;
}

static void *start_thread(void *arg) {
  struct start_thread_arg *t_arg = (struct start_thread_arg *)arg;
#ifdef SCHW_SCHED_FIFO
  struct job_fifo *queue = (struct job_fifo *)t_arg->job_queue;
#endif
#ifdef SCHW_SCHED_PRIORITY
  struct job_pqueue *queue = (struct job_pqueue *)t_arg->job_queue;
#endif

  // Main thread loop
  while (1) {
    struct job j = {0};

    // Wait for a job to be available
    while (sem_wait(&queue->jobs_in_q) == -1 && errno == EINTR)
      ;
    pthread_mutex_lock(&t_arg->pool->rwlock);
    ++t_arg->pool->working_threads;
    pthread_mutex_unlock(&t_arg->pool->rwlock);

    // Pop jobs until the queue is empty
    while (job_pop(queue, &j) == 0) {
      t_arg->thread->job_id = j.job_id;
      void *res = j.job_func(j.job_arg);
    }

    pthread_mutex_lock(&t_arg->pool->rwlock);
    --t_arg->pool->working_threads;
    pthread_mutex_unlock(&t_arg->pool->rwlock);
  }

  free(arg);
  return (void *) 1;
}

#ifdef THREAD_POOL_DYNAMIC_SIZING

int thread_pool_resize(struct thread_pool *pool, int32_t change) {}

#endif // THREAD_POOL_DYNAMIC_SIZING
