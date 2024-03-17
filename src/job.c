#include "job.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#ifdef SCHW_SCHED_FIFO

/*
 * Initialize FIFO job queue
 * @summary: Allocate memory for the job queue and initialize the fields.
 * @param fifo: Pointer to the job_fifo struct to initialize.
 * @param cap: The maximum number of jobs that can be stored in the queue.
 * For now, this cannot be changed after initialization.
 * @return: 0 on success, or an error.
 * @error errno: If malloc fails, errno is returnd.
 */
int job_init(struct job_fifo *fifo, uint32_t cap) {
  struct job *jobs = malloc(cap * sizeof(struct job));
  if (unlikely(jobs == NULL)) {
    return errno;
  }
  pthread_mutex_init(&fifo->rwlock, NULL);
  fifo->job_pool = jobs;
  fifo->head = 0;
  fifo->tail = 0;
  fifo->cap = cap;
  fifo->len = 0;
  sem_init(&fifo->free_slots, 0, cap);
  sem_init(&fifo->jobs_in_q, 0, 0);
  return 0;
}

/*
 * Push a job onto the FIFO queue.
 * @summary: Add a job at to the end of the queue if there is space.
 * If there is no space, then the thread will block until there is space.
 * @param fifo: Pointer to the job_fifo struct to add the job to.
 * @param job: Pointer to the job to add to the queue.
 * @return: 0 on success.
 */
int job_fifo_push(struct job_fifo *fifo, struct job *job) {
  // Prevent cases where signal interrupts this call
  while (sem_wait(&fifo->free_slots))
    ;
  pthread_mutex_lock(&fifo->rwlock);

  fifo->job_pool[fifo->tail] = *job;
  fifo->tail = (fifo->tail + 1) % fifo->cap;
  ++fifo->len;

  pthread_mutex_unlock(&fifo->rwlock);
  sem_post(&fifo->jobs_in_q);
  return 0;
}

/*
 * Pop a job from the FIFO queue.
 * @summary: Remove a job from the front of the queue if there is one.
 * If there is no job, then the function will return a non-zero errno value.
 * @param fifo: Pointer to the job_fifo struct to remove the job from.
 * @param buf: Pointer to a buffer to hold popped job.
 * @return: 0 on success, an error otherwise.
 * @error ESRCH: There are no jobs on the queue.
 */
int job_pop(struct job_fifo *fifo, struct job *buf) {
  // Prevent cases where signal interrupts this call
  while (sem_wait(&fifo->jobs_in_q))
    ;
  pthread_mutex_lock(&fifo->rwlock);

  if (fifo->len == 0) {
    pthread_mutex_unlock(&fifo->rwlock);
    return ESRCH;
  }

  *buf = fifo->job_pool[fifo->head];
  fifo->head = (fifo->head + 1) % fifo->cap;
  --fifo->len;

  pthread_mutex_unlock(&fifo->rwlock);
  sem_post(&fifo->free_slots);
  return 0;
}

/*
 * Free the FIFO job queue.
 * @summary: Free the memory allocated for the job queue and reset the fields.
 * @param fifo: Pointer to the job_fifo struct to free.
 * @return: 0 on success, or an error.
 * @error EBUSY: The job queue is not empty.
 */
int job_free(struct job_fifo *fifo) {
  pthread_mutex_lock(&fifo->rwlock);
  if (fifo->len > 0) {
    pthread_mutex_unlock(&fifo->rwlock);
    return EBUSY;
  }
  pthread_mutex_unlock(&fifo->rwlock);
  pthread_mutex_destroy(&fifo->rwlock);
  free(fifo->job_pool);
  fifo->job_pool = NULL;
  fifo->head = fifo->tail = fifo->cap = fifo->len = 0;
  sem_destroy(&fifo->free_slots);
  sem_destroy(&fifo->jobs_in_q);
  return 0;
}

#endif // SCHW_SCHED_FIFO

#ifdef SCHW_SCHED_PRIORITY

// Binary heap functions for operating on the priority queue.
static void bheap_bup(struct job_key *keys, uint32_t idx);
static void bheap_bdown(struct job_key *keys, uint32_t idx, uint32_t len);
static void bheap_push(struct job_key *keys, uint32_t len, struct job_key *key);
static struct job_key bheap_pop(struct job_key *keys, uint32_t len);
static void bheap_heapify(struct job_key *keys, uint32_t len);

/*
 * Initialize priority job queue.
 * @summary: Allocate memory for the job queue and initialize the fields.
 * @param pqueue: Pointer to the job_pqueue struct to initialize.
 * @param cap: The maximum number of jobs that can be stored in the queue.
 * For now, this cannot be changed after initialization.
 * @return: 0 on success, or an error.
 * @error errno: If malloc failed, the error code errno is returned.
 */
int job_init(struct job_pqueue *pqueue, uint32_t cap) {
  struct job_key *keys = malloc(cap * sizeof(struct job_key));
  if (unlikely(keys == NULL)) {
    return errno;
  }

  struct job *job_pool = malloc(cap * sizeof(struct job));
  if (unlikely(job_pool == NULL)) {
    free(keys);
    return errno;
  }
  // Initialize the job pool as linked list of
  // free blocks.
  for (uint32_t i = 0; i < cap - 1; ++i) {
    job_pool[i].next_free = &job_pool[i + 1];
  }
  job_pool[cap - 1].next_free = NULL;

  pthread_mutex_init(&pqueue->rwlock, NULL);
  pqueue->keys = keys;
  pqueue->job_pool = job_pool;
  pqueue->first_free = job_pool;
  sem_init(&pqueue->free_slots, 0, cap);
  pqueue->cap = cap;
  pqueue->len = 0;

  return 0;
}

/*
 * Push a job onto the priority queue.
 * @summary: Push a job on the priority queue. The job's data is copied into
 * the queue. A lower priority number correlates with a higher priority.
 * Blocks if there is no room in the queue. Latency spikes can occur when
 * the jobs are being aged. This only happens periodically.
 * @param pqueue: Priority Job Queue
 * @param job: Job to push to the queue. The job is copied, so the passed
 * in job can be discarded after the call.
 * @param priority: The job's priority. Lower number = Higher priority.
 * @return: 0 on success, or an error.
 * @error ENOBUFS: There is no free space in the queue. The thread should
 * block if there is no space, so if this is returned, there is a bug.
 */
int job_pqueue_push(struct job_pqueue *pqueue, struct job *job, int32_t priority) {
  // I don't know if I like the blocking policy. May change later
  sem_wait(&pqueue->free_slots);
  pthread_mutex_lock(&pqueue->rwlock);

  // If this occurs, there is a bug.
  if (unlikely(pqueue->first_free == NULL)) {
    return ENOBUFS;
  }

  struct job_key new_key = {
    .priority = priority,
    .job = pqueue->first_free,
    .last_age = 0
  };
  bheap_push(pqueue->keys, pqueue->len, &new_key);

  struct job *next_free = pqueue->first_free->next_free;
  *pqueue->first_free = *job;
  pqueue->first_free = next_free;
  ++pqueue->len;

  pthread_mutex_unlock(&pqueue->rwlock);
  return 0;
}

/*
 * Pop a job off the queue.
 * @summary: Pops the highest priority job off the queue. Copies the
 * job into buf.
 * @param pqueue: Job priority queue.
 * @param buf: Buffer to copy the job that was popped off the queue.
 * @return: 0 on success, or an error.
 * @error ESRCH: There are no jobs in the queue.
 */
int job_pop(struct job_pqueue *pqueue, struct job *buf) {
  pthread_mutex_lock(&pqueue->rwlock);

  if (pqueue->len == 0) {
    pthread_mutex_unlock(&pqueue->rwlock);
    return ESRCH;
  }

  struct job_key key = bheap_pop(pqueue->keys, pqueue->len);
  *buf = *key.job;

  key.job->next_free = pqueue->first_free;
  pqueue->first_free = key.job;

  pthread_mutex_unlock(&pqueue->rwlock);
  sem_post(&pqueue->free_slots);
  return 0;

}

/*
 * Frees the priority job queue.
 * @summary: Frees the priority job queue. If the queue is not empty,
 * EBUSY is returned.
 * @param pqueue: Job priority queue.
 * @return: 0 on success, or an error.
 * @error EBUSY: The queue is not empty.
 */
int job_free(struct job_pqueue *pqueue) {
  pthread_mutex_lock(&pqueue->rwlock);
  if (pqueue->len > 0) {
    pthread_mutex_unlock(&pqueue->rwlock);
    return EBUSY;
  }
  pthread_mutex_unlock(&pqueue->rwlock);
  pthread_mutex_destroy(&pqueue->rwlock);
  free(pqueue->keys);
  pqueue->keys = NULL;
  free(pqueue->job_pool);
  pqueue->job_pool = NULL;
  pqueue->first_free = NULL;
  sem_destroy(&pqueue->free_slots);
  pqueue->cap = 0;
  pqueue->len = 0;
  return 0;
}

/* Bubble up an item in the binary heap */
static void bheap_bup(struct job_key *keys, uint32_t idx) {
  if (idx == 0)
    return;

  uint32_t parent = (idx - 1) / 2;

  while (keys[parent].priority > keys[idx].priority) {
    struct job_key tmp = keys[parent];
    keys[parent] = keys[idx];
    keys[idx] = tmp;

    if (parent == 0)
      break;

    idx = parent;
    parent = (idx - 1) / 2;
  }
}

/* Bubble down an item in the binary heap */
static void bheap_bdown(struct job_key *keys, uint32_t idx, uint32_t len) {
  while (idx <= (UINT32_MAX / 2) + 2) {
    uint32_t l = 2 * idx + 1, r = 2 * idx + 2;
    uint32_t min_idx = idx;

    if (l < len && keys[l].priority < keys[min_idx].priority)
      min_idx = l;
    if (r < len && keys[r].priority < keys[min_idx].priority)
      min_idx = r;

    if (min_idx == idx)
      break;

    struct job_key tmp = keys[min_idx];
    keys[min_idx] = keys[idx];
    keys[idx] = tmp;
    idx = min_idx;
  }
}

/* Push an item to the binary heap */
static void bheap_push(struct job_key *keys, uint32_t len, struct job_key *key) {
  keys[len++] = *key;
  bheap_bup(keys, len - 1);
}

/* Pop the item with the lowest key off the binary heap */
static struct job_key bheap_pop(struct job_key *keys, uint32_t len) {
  struct job_key job = keys[0];
  keys[0] = keys[--len];
  bheap_bdown(keys, 0, len);
  return job;
}

/* 
 * Construct a heap from an array which satisfies the shape property of a
 * binary heap.
 */
static void bheap_heapify(struct job_key *keys, uint32_t len) {
  for (uint32_t i = len / 2; i > 0; --i) {
    bheap_bdown(keys, i, len);
  }
}

#endif // SCHW_SCHED_PRIORITY
