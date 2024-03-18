#ifndef SCHWIMMBAD_H
#define SCHWIMMBAD_H

#include <pthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t jid;

// Default scheduling policy will be FIFO
#ifndef SCHW_SCHED_PRIORITY
#define SCHW_SCHED_FIFO
#else
#define SCHW_SCHED_PRIORITY
#endif

struct schw_jid_helper {
  jid current;
  pthread_spinlock_t lock;
};

struct schw_job {
  void *(*job_func)(void *arg);
  void *job_arg;
#ifdef SCHW_SCHED_PRIORITY
  int32_t priority;
#endif
};

struct schw_pool {
  pthread_mutex_t rwlock;
  struct schw_jid_helper jid_helper;
  struct thread *threads;
  struct job_queue *queue;
  uint32_t num_threads;
  uint32_t working_threads;
};

/*
 * @summary: Initialize the thread pool and job queue with the given
 * number of threads and queue length. Allocates all the memory needed
 * for the entire pool and starts each thread.
 * @param pool: The thread pool to initialize.
 * @param num_threads: The number of threads to start.
 * @param queue_len: The length of the job queue.
 * @return: 0 on success, error code on failure.
 * @error: errno if malloc, pthread_create, pthread_mutex_init, or
 * sem_init fails.
 */
int schw_init(struct schw_pool *pool, uint32_t num_threads, uint32_t queue_len);

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
jid schw_push(struct schw_pool *pool, struct schw_job *job);

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
