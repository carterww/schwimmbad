#include "thread.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "job.h"

/*
 * Main thread loop for worker threads.
 * @summary: This function is called when the thread pool is initialized
 * for each thread. It will perform some initializations and then enter
 * a loop where it waits for jobs to be available in the queue. When a job
 * is available, the thread will pop the job and execute it.
 * @param arg: Pointer to a start_thread_arg struct.
 * @return: Never unless an error occurs.
 * @error errno: When the pthread_setcancelstate or pthread_setcanceltype
 * functions fail, errno is returned.
 */
static void *start_thread(void *arg);

/*
 * Argument for start_thread function. This struct is used to pass arguments to
 * the start_thread function. It contains a pointer to the job queue, the
 * thread, and the thread pool.
 */
struct start_thread_arg {
  struct thread *thread;
  struct schw_pool *pool;
};

int thread_pool_init(struct schw_pool *pool, uint32_t num_threads) {
  if (pool == NULL) {
    return EINVAL;
  }
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

int thread_pool_free(struct schw_pool *pool) {
  if (pool == NULL) {
    return EINVAL;
  }
  pthread_mutex_lock(&pool->rwlock);
  if (pool->working_threads > 0) {
    pthread_mutex_unlock(&pool->rwlock);
    return EBUSY;
  }

  for (uint32_t i = 0; i < pool->num_threads; ++i) {
    pthread_cancel(pool->threads[i].tid);
  }

  pthread_mutex_unlock(&pool->rwlock);
  pthread_mutex_destroy(&pool->rwlock);
  free(pool->threads);
  return 0;
}

static void *start_thread(void *arg) {
  struct start_thread_arg *t_arg = (struct start_thread_arg *)arg;
  struct schw_pool *pool = t_arg->pool;
  int error = 0;
  struct job j = {0};
  // Allow thread to be cancelled at cleanup points
  error = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (error != 0)
    goto error;
  error |= pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
  if (error != 0)
    goto error;

  t_arg->thread->job_id = -1;
  // Main thread loop
  while (1) {
    // Wait for a job to be available. This is a cancellation point.
    while (sem_wait(&queue->queue.jobs_in_q) == -1 && errno == EINTR)
      ;
    pthread_mutex_lock(&t_arg->pool->rwlock);
    ++t_arg->pool->working_threads;
    pthread_mutex_unlock(&t_arg->pool->rwlock);

    // Pop jobs until the queue is empty
    while (job_pop(queue, &j) == 0) {
      t_arg->thread->job_id = j.id;
      void *res = j.job.job_func(j.job.job_arg);
    }

    pthread_mutex_lock(&t_arg->pool->rwlock);
    --t_arg->pool->working_threads;
    pthread_mutex_unlock(&t_arg->pool->rwlock);
  }

error:
  free(arg);
  return (void *)error; // Leave this cast for now, but change it later
}

#ifdef THREAD_POOL_DYNAMIC_SIZING

int thread_pool_resize(struct schw_pool *pool, int32_t change) {
  return -1;
}

#endif // THREAD_POOL_DYNAMIC_SIZING
