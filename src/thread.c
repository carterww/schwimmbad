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
 * @summary: Dispatch a job to a thread. This function will call the job
 * function with the job argument and then call the job done callback if
 * it exists.
 */
static inline void __thread_dispatch(struct schw_pool *pool, struct thread *thread, struct schw_job job) {
  thread->job_id = job.id;
  void *res = job.job_func(job.job_arg);

  // Call the job done callback if it exists
  if (pool->cb != NULL)
    pool->cb(job.id, pool->cb_arg, res);
}

/*
 * @summary: Check and handle any flags before the thread goes to sleep.
 * SCHW_POOL_FLAG_WAIT_ALL: Thread checks if it is last one going to sleep.
 * If so, it will post to the sync semaphore to wake up the main thread.
 */
static inline void __handle_flags_before_sleep(struct schw_pool *pool) {
  if (pool->working_threads == 0 &&
      pool->flags & SCHW_POOL_FLAG_WAIT_ALL) {
    sem_post(&pool->sync);
  }
}

/*
 * Argument for start_thread function. This struct is used to pass arguments to
 * the start_thread function. It contains a pointer to the job queue, the
 * thread, and the thread pool.
 */
struct start_thread_arg {
  struct thread *thread;
  struct schw_pool *pool;
};

/*
 * @summary: Initialize a thread with the given thread pool and argument by starting
 * the pthread.
 */
static inline int __init_pthread(struct thread *thread, void *arg) {
  int err = pthread_create(&thread->tid, NULL, start_thread, arg);
  if (unlikely(err != 0))
    return err;
  return pthread_detach(thread->tid);
}

int thread_pool_init(struct schw_pool *pool, uint32_t num_threads) {
  struct thread *threads = malloc(num_threads * sizeof(struct thread));
  if (unlikely(threads == NULL)) {
    return errno;
  }

  pthread_mutex_init(&pool->rwlock, NULL);
  pool->jid_helper.current = 0;
  pthread_spin_init(&pool->jid_helper.lock, 0);
  pool->threads = threads;
  pool->num_threads = num_threads;
  pool->working_threads = 0;
  pool->cb = NULL;
  pool->cb_arg = NULL;
  pool->flags = 0;
  sem_init(&pool->sync, 0, 0);

  int err = 0;
  for (; threads != pool->threads + num_threads; threads++) {
    struct start_thread_arg *arg = malloc(sizeof(struct start_thread_arg));
    if (unlikely(arg == NULL)) {
      err = errno;
      break;
    }
    arg->pool = pool;
    arg->thread = threads;
    if ((err = __init_pthread(threads, arg) != 0))
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

static inline void __thread_work(struct schw_pool *pool, struct thread *thread) {
  sem_t *jobs_in_q = NULL;
  QUEUE_GET_MEMBER_PTR(pool, jobs_in_q, jobs_in_q);
  if (!jobs_in_q) {
    return;
  }
  struct schw_job job = {0};
  thread->job_id = -1;

  while (1) {
    // Wait for a job to be available. This is a cancellation point.
    while (sem_wait(jobs_in_q) == -1 && errno == EINTR)
      ;
    ++pool->working_threads;

    do {
      int pop_res = pool->pop_job(pool, &job);
      if (pop_res != 0)
        break;

      __thread_dispatch(pool, thread, job);
    // If the job queue is not empty, continue to pop jobs
    } while (sem_trywait(jobs_in_q) == 0);
    thread->job_id = -1;

    /* working_threads is an atomic uint but we wrap it in a lock/unlock
     * here to ensure that the value does not change between the decrement
     * and check.
     * The reason for keeping it an atomic uint is because the increment does
     * not need to check anything. My testing shows atomic can be 4-15x faster
     * than a lock/unlock pair.
     */
    pthread_mutex_lock(&pool->rwlock);
    --pool->working_threads;
    __handle_flags_before_sleep(pool);
    pthread_mutex_unlock(&pool->rwlock);
  }
}

static void *start_thread(void *arg) {
  struct start_thread_arg *t_arg = (struct start_thread_arg *)arg;
  struct schw_pool *pool = t_arg->pool;
  struct thread *thread = t_arg->thread;

  int error = 0;
  // Allow thread to be cancelled at cleanup points
  error = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  error |= pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
  if (error != 0)
    goto error;

  // Main thread loop
  __thread_work(pool, thread);

error:
  free(arg);
  return (void *)(long)error;
}
