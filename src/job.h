#ifndef SCHW_JOB_H
#define SCHW_JOB_H

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#include "schwimmbad.h"

#ifdef __cplusplus
extern "C" {
#endif

// Macro to make getting a member from the queue easier.
#define QUEUE_GET_MEMBER(pool, var, member) \
  if (pool->policy == FIFO) { \
    var = pool->fqueue->member; \
  } else if (pool->policy == PRIORITY) { \
    var = pool->pqueue->member; \
  }

// Macro to make getting a member ptr from the queue easier.
#define QUEUE_GET_MEMBER_PTR(pool, var, member) \
  if (pool->policy == FIFO) { \
    var = &pool->fqueue->member; \
  } else if (pool->policy == PRIORITY) { \
    var = &pool->pqueue->member; \
  }

/*
 * Wrapper for schw_job which allows for a singly linked list
 * of free job slots when no schw_job is stored in the slot.
 */
struct job {
  union {
    struct schw_job job;
    struct job *next_free;
  };
};

/*
 * A job queue that is implemented as a circular buffer. The
 * queue can be accessed by multiple threads at the same time.
 * The queue has a read-write lock that must be acquired before
 * reading or writing to the queue. The queue has two semaphores
 * that are used to signal when there are free slots in the queue
 * and when there are jobs in the queue. Each worker thread will
 * wait on the jobs_in_q semaphore until there is a job in the
 * queue.
 */
struct job_fifo {
  pthread_mutex_t rwlock;
  struct job *job_pool;
  uint32_t head;
  uint32_t tail;
  uint32_t cap;
  uint32_t len;
  sem_t free_slots;
  sem_t jobs_in_q;
};

/*
 * The key for the priority queue. The key contains a pointer
 * to the job and the priority of the job. The priority queue
 * is implemented as a binary heap. The priority is stored as
 * a member of the key struct in order to maximize cache locality.
 * Without it, the *job pointer would have to be dereferenced to get
 * the priority.
 */
struct job_key {
  struct job *job;
  int32_t priority;
};

/*
 * A job queue that is implemented as a priority queue. The
 * queue can be accessed by multiple threads at the same time,
 * so it has a read-write lock that must be acquired before
 * reading or writing to the queue. The queue has two semaphores
 * that are used to signal when there are free slots in the queue
 * and when there are jobs in the queue. Each worker thread will
 * wait on the jobs_in_q semaphore until there is a job in the
 * queue.
 * The priority queue is implemented in two parts: a binary heap
 * of keys and a pool of jobs.
 * The keys are stored separately from the jobs to minimize the
 * work required when shifting the keys around in the heap.
 * The jobs are stored as a contiguous singly linked list of free
 * slots. The first_free pointer points to the head of the free list.
 */
struct job_pqueue {
  pthread_mutex_t rwlock;
  struct job_key *keys;
  struct job *job_pool;
  struct job *first_free;
  sem_t free_slots;
  sem_t jobs_in_q;
  uint32_t cap;
  uint32_t len;
};

/*
 * Initialize FIFO job queue
 * @summary: Allocate memory for the job queue and initialize the fields.
 * @param pool: Pointer to the thread pool.
 * @param cap: The maximum number of jobs that can be stored in the queue.
 * @return: 0 on success, or an error.
 * @error EINVAL: The queue pointer is NULL.
 * @error errno: If malloc, pthread_mutex_init, or sem_init fails, errno is
 * returned.
 */
int job_fifo_init(struct schw_pool *pool, uint32_t cap);

/*
 * Push a job onto the FIFO queue.
 * @summary: Add a job at to the end of the queue if there is space.
 * If there is no space, then the function will return an error.
 * @param pool: Pointer to the thread pool.
 * @param job: Pointer to the job.
 * @param flags: Flags that change the behavior of the function.
 *               SCHW_PUSH_BLOCK: Block until there is space in the queue.
 * @return: 0 on success. An error otherwise.
 * @error EAGAIN: There is no space in the queue.
 * @error EINVAL: The queue or job pointer is NULL.
 */
int job_fifo_push(struct schw_pool *pool, struct schw_job *job, uint32_t flags);

/*
 * Pop a job from the FIFO queue.
 * @summary: Remove a job from the front of the queue. This function
 * assumes the jobs_in_q semaphore has been waited on before.
 * @param pool: Pointer to the thread pool.
 * @param buf: Pointer to the buffer to copy the job into.
 * @return: 0 on success, an error otherwise.
 * @error EINVAL: The queue or buf pointer is NULL.
 */
int job_fifo_pop(struct schw_pool *pool, struct schw_job *buf);

/*
 * Free the FIFO job queue.
 * @summary: Free the memory allocated for the job queue and reset the fields.
 * @param pool: Pointer to the thread pool.
 * @return: 0 on success, or an error.
 * @error EBUSY: The job queue is not empty and connot be freed.
 */
int job_fifo_free(struct schw_pool *pool);

/*
 * Initialize priority job queue.
 * @summary: Allocate memory for the job queue and initialize the fields.
 * @param pool: Pointer to the thread pool.
 * @param cap: The maximum number of jobs that can be stored in the queue.
 * For now, this cannot be changed after initialization.
 * @return: 0 on success, or an error.
 * @error EINVAL: The queue pointer is NULL.
 * @error errno: If malloc failed, the error code errno is returned.
 */
int job_pqueue_init(struct schw_pool *pool, uint32_t cap);

/*
 * Push a job onto the priority queue.
 * @summary: Push a job on the priority queue. The job's data is copied into
 * the queue. A lower priority number correlates with a higher priority. If
 * there is no space in the queue, the function will return an error.
 * @param pool: Pointer to the thread pool.
 * @param job: Job to push to the queue. The job is copied, so the passed
 * in job can be discarded after the call.
 * @param flags: Flags that change the behavior of the function.
 *               SCHW_PUSH_BLOCK: Block until there is space in the queue.
 * @return: 0 on success, or an error.
 * @error EAGAIN: There is no space in the queue.
 * @error EINVAL: The queue or job pointer is NULL.
 * @error errno: If sem_trywait fails, errno is returned.
 */
int job_pqueue_push(struct schw_pool *pool, struct schw_job *job, uint32_t flags);

/*
 * Pop a job off the queue.
 * @summary: Pops the highest priority job off the queue. Copies the
 * job into buf. Assumes that the jobs_in_q semaphore has been waited
 * on before.
 * @param pool: Pointer to the thread pool.
 * @param buf: Buffer to copy the job that was popped off the queue.
 * @return: 0 on success, or an error.
 * @return EINVAL: The queue or buf pointer is NULL.
 */
int job_pqueue_pop(struct schw_pool *pool, struct schw_job *buf);

/*
 * Frees the priority job queue.
 * @summary: Frees the priority job queue. If the queue is not empty,
 * EBUSY is returned.
 * @param pool: Pointer to the thread pool.
 * @return: 0 on success, or an error.
 * @error EBUSY: The queue is not empty.
 */
int job_pqueue_free(struct schw_pool *pool);

#ifdef __cplusplus
}
#endif

#endif // SCHW_JOB_H
