#ifndef SCHWIMMBAD_H
#define SCHWIMMBAD_H

#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHW_PUSH_BLOCK 0x1

typedef int64_t jid;

struct schw_jid_helper {
  jid current;
  pthread_spinlock_t lock;
};

enum schw_job_queue_policy {
  FIFO,
  PRIORITY
};

struct schw_job {
  jid id;
  void *(*job_func)(void *arg);
  void *job_arg;
  int32_t priority;
};

typedef void (*job_cb)(jid id, void *arg);

struct schw_pool {
  pthread_mutex_t rwlock;
  struct schw_jid_helper jid_helper;
  /* Pool can be either a priority queue or a FIFO queue. */
  union {
    struct job_pqueue *pqueue;
    struct job_fifo *fqueue;
  };
  /* Job queue functions. */
  int (*push_job)(struct schw_pool *pool, struct schw_job *job, uint32_t flags);
  int (*pop_job)(struct schw_pool *pool, struct schw_job *buf);

  struct thread *threads;
  uint32_t num_threads;
  _Atomic(uint32_t) working_threads;

  enum schw_job_queue_policy policy;

  // User defined callback for when a job is done.
  job_cb cb;
  void *cb_arg;
};

/*
 * @summary: Initialize the thread pool and job queue with the given
 * number of threads and queue length. Allocates all the memory needed
 * for the entire pool and starts each thread.
 * @param pool: The thread pool to initialize.
 * @param num_threads: The number of threads to start.
 * @param queue_len: The length of the job queue.
 * @return: 0 on success, error code on failure.
 * @error EINVAL: num_threads is 0, queue_len is 0, pool is NULL,
 * policy is invalid.
 * @error: errno if malloc, pthread_create, pthread_mutex_init, or
 * sem_init fails.
 */
int schw_init(struct schw_pool *pool, uint32_t num_threads, uint32_t queue_len,
    enum schw_job_queue_policy policy);

/*
 * @summary: Free the thread pool and all of its resources. If there are
 * any threads working or the job queue is not empty, then this function
 * will return an error.
 * @param pool: The thread pool to free.
 * @return: 0 on success, error code on failure.
 * @error: EBUSY if there are still threads working or the job queue is
 * not empty.
 */
int schw_free(struct schw_pool *pool);

/*
 * @summary: Set a pool's callback function for when a job is completed.
 * Allows the user to be notified when a job is done by passing the
 * job id and the user defined argument to the callback function.
 * @param pool: The thread pool to set the callback for.
 * @param job_done_cb: The callback function to call when a job is done.
 * Pass NULL to remove the callback.
 * @param arg: The argument to pass to the callback function.
 * @return: void
 * @note: This is global for the entire pool. Each time a job completes,
 * it will be called.
 */
void schw_set_callback(struct schw_pool *pool, job_cb cb, void *arg);

/*
 * @summary: Push a job onto the queue. The job will be copied
 * into the lib's buffers. This function will return an error if there
 * is no room in the job queue.
 * If the priority scheduling policy is chosen, then the job will be inserted
 * into the queue with the priority given by the priority field in the job
 * struct. A lower priority value means the job will be executed sooner.
 * @param pool: The thread pool to push the job onto.
 * @param job: The job to push onto the queue. Will be copied. Contains
 * the function to execute and the argument to pass to the function.
 * @return: The job id of the pushed job on success, -1 on failure.
 * @error: -1 if the job queue is full.
 */
jid schw_push(struct schw_pool *pool, struct schw_job *job, uint32_t flags);

/*
 * @summary: Cancel a job in the queue. If the job has not been started,
 * then it will be removed from the queue. If the job has already started,
 * an attempt to cancel it will be made. If the job has already finished
 * (or does not exist), then the function will return an error.
 * @param pool: The thread pool to cancel the job from.
 * @param id: The job id of the job to cancel.
 * @return: 0 on success, error code on failure.
 * @error: EINVAL if the job id is invalid (does not exist/has already
 * finished).
 * @error: EBUSY if the job has already started and cannot be cancelled.
 * @error: errno if pthread_cancel fails.
 */
int schw_cancel(struct schw_pool *pool, jid id);

/*
 * @param pool: The thread pool to query.
 * @return: The number of threads in the pool.
 */
uint32_t schw_threads(struct schw_pool *pool);

/*
 * @param pool: The thread pool to query.
 * @return: The number of threads currently working in the pool.
 */
uint32_t schw_working_threads(struct schw_pool *pool);

/*
 * @param pool: The thread pool to query.
 * @return: The number of jobs in the queue.
 */
uint32_t schw_queue_len(struct schw_pool *pool);

/*
 * @param pool: The thread pool to query.
 * @return: The capacity of the job queue.
 */
uint32_t schw_queue_cap(struct schw_pool *pool);

int schw_pool_resize(struct schw_pool *pool, int32_t change);
int schw_queue_resize(struct schw_pool *pool, int32_t change);

#ifdef __cplusplus
}
#endif

#endif // SCHWIMMBAD_H
