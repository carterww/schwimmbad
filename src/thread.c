#include "thread.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "job.h"
#include "schwimmbad.h"

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
  struct thread *thread = t_arg->thread;
  sem_t *jobs_in_q;

  QUEUE_GET_MEMBER_PTR(pool, jobs_in_q, jobs_in_q);

  int error = 0;
  struct schw_job j = {0};
  // Allow thread to be cancelled at cleanup points
  error = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  error |= pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
  if (error != 0)
    goto error;

  thread->job_id = -1;
  // Main thread loop
  while (1) {
    // Wait for a job to be available. This is a cancellation point.
    while (sem_wait(jobs_in_q) == -1 && errno == EINTR)
      ;
    ++pool->working_threads;

    do {
      int pop_res = pool->pop_job(pool, &j);
      if (pop_res != 0)
        break;

      thread->job_id = j.id;
      void *res = j.job_func(j.job_arg);

      // Call the job done callback if it exists
      if (pool->cb != NULL)
        pool->cb(j.id, pool->cb_arg);

    // If the job queue is not empty, continue to pop jobs
    } while (sem_trywait(jobs_in_q) == 0);

    --pool->working_threads;
    thread->job_id = -1;
  }

error:
  free(arg);
  return (void *)(long)error;
}

#ifdef THREAD_POOL_DYNAMIC_SIZING

int thread_pool_resize(struct schw_pool *pool, int32_t change) {
  return -1;
}

#endif // THREAD_POOL_DYNAMIC_SIZING
