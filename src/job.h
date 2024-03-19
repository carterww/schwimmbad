#ifndef SCHW_JOB_H
#define SCHW_JOB_H

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#include "schwimmbad.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A job that is stored in the job queue. The job contains a
 * unique identifier, a function pointer to the job function,
 * and a void pointer to the argument for the job function.
 * The jobs form a singly linked list of free slots when there
 * is no job in the slot. This implementation is only used when
 * the scheduling policy is PRIORITY.
 */
struct job {
  union {
    struct {
      jid id;
      struct schw_job job;
    };
    struct job *next_free;
  };
};

/*
 * @summary: Initialize the job queue with the given capacity.
 * This function has two different implementations based on the
 * scheduling policy chosen at compile time. If the policy is
 * FIFO, then the job queue will be implemented as a circular
 * buffer. If the policy is PRIORITY, then the job queue will
 * be implemented as a priority queue. To see the implementation
 * specific descriptions, see the comments for job_init in
 * job.c.
 * @param queue: The job queue to initialize.
 * @param cap: The capacity of the job queue.
 * @return: 0 on success, error code on failure.
 * @error: errno when malloc, sem_init, or pthread_mutex_init fails.
 */
int job_init(struct job_queue *queue, uint32_t cap);

/*
 * @summary: Push a job onto the queue. This function is
 * has two implementations, one for FIFO and one for priority.
 * The job will be copied into the queue. If there are no free slots
 * in the queue, the function will return an error.
 * @param queue: The job queue to push onto.
 * @param job: The job to push onto the queue.
 * @return: 0 on success, error code on failure.
 * @errror: EAGAIN if the queue is full.
 */
int job_push(struct job_queue *queue, struct job *job);

/*
 * @summary: Pop the next job off the queue. The job will be copied
 * into the buffer. This function has two different implementations
 * based on the scheduling policy chosen at compile time. If the
 * policy is FIFO, then the job will be popped off the front of the
 * queue. If the policy is PRIORITY, then the job will be popped off
 * the front of the binary heap. To see the implementation specific
 * descriptions, see the comments for job_pop in job.c.
 * @param queue: The job queue to pop from.
 * @param buf: The buffer to copy the job into.
 * @return: 0 on success, error code on failure.
 * @error: ESRCH if the queue is empty.
 */
int job_pop(struct job_queue *queue, struct job *buf);

/*
 * @summary: Free the resources used by the job queue. This function
 * has two different implementations based on the scheduling policy.
 * The queue can only be freed if there are no jobs currently in the
 * queue.
 * @param queue: The job queue to free.
 * @return: 0 on success, error code on failure.
 * @error: EBUSY if the queue is not empty.
 */
int job_free(struct job_queue *queue);

#ifdef SCHW_SCHED_FIFO

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

#endif // SCHW_SCHED_FIFO

#ifdef SCHW_SCHED_PRIORITY

/*
 * The key for the priority queue. The key contains a pointer
 * to the job and the priority of the job. The priority queue
 * is implemented as a binary heap. The keys are stored in an
 * array and the jobs are stored in a separate array. The priority
 * queue is implemented as a binary heap sorted by the priority.
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
 * wait on the jobs_in_queue semaphore until there is a job in the
 * queue.
 * The priority queue is implemented in two parts: a binary heap
 * of keys and a pool of jobs.
 * The keys are stored separately from the jobs to minimize the
 * work required when shifting the keys around in the heap.
 * The jobs are stored as a contiguous singly linked list of free
 * slots. The first_free pointer points to the first free slot in
 * the job pool. This implementation allows for minimal overhead
 * when adding and removing jobs from the queue.
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

#endif // SCHW_SCHED_PRIORITY

/*
 * A wrapper for the implementation specific job queue. This
 * allows functions to be written that can be used with either
 * implementation without knowing which implementation is being
 * used.
 */
struct job_queue {
#ifdef SCHW_SCHED_FIFO
  struct job_fifo queue;
#endif
#ifdef SCHW_SCHED_PRIORITY
  struct job_pqueue queue;
#endif
};

#ifdef __cplusplus
}
#endif

#endif // SCHW_JOB_H
