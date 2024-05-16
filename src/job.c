#include "job.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "schwimmbad.h"

static inline int __job_fifo_init(struct job_fifo *queue, uint32_t cap) {
  /* Allocations */
  struct job *jobs = malloc(cap * sizeof(struct job));
  if (unlikely(jobs == NULL)) {
    goto unwind_0;
  }
  /* Initialize semaphore and mutexes. */
  if (pthread_mutex_init(&queue->rwlock, NULL)) {
    goto unwind_1;
  }
  if (sem_init(&queue->free_slots, 0, cap)) {
    goto unwind_2;
  }
  if (sem_init(&queue->jobs_in_q, 0, 0)) {
    goto unwind_3;
  }

  queue->job_pool = jobs;
  queue->head = 0;
  queue->tail = 0;
  queue->cap = cap;
  queue->len = 0;
  return 0;
// Fall throughs to free resources if an error occurs.
unwind_3:
  sem_destroy(&queue->free_slots);
unwind_2:
  pthread_mutex_destroy(&queue->rwlock);
unwind_1:
  free(jobs);
unwind_0:
  free(queue);
  return errno;
}

static inline void __job_fifo_free(struct job_fifo *queue) {
  /* Free resources */
  pthread_mutex_destroy(&queue->rwlock);
  free(queue->job_pool);
  sem_destroy(&queue->free_slots);
  sem_destroy(&queue->jobs_in_q);
  free(queue);
}

int job_fifo_init(struct schw_pool *pool, uint32_t cap) {
  pool->fqueue = malloc(sizeof(struct job_fifo));
  if (unlikely(pool->fqueue == NULL)) {
    return errno;
  }

  pool->push_job = job_fifo_push;
  pool->pop_job = job_fifo_pop;

  return __job_fifo_init(pool->fqueue, cap);
}

int job_fifo_push(struct schw_pool *pool, struct schw_job *job, uint32_t flags) {
  if (!(job && pool)) {
    return EINVAL;
  }
  struct job_fifo *queue = pool->fqueue;
  // If queue is full and flag is set to block, then wait for a slot.
  if (flags & SCHW_PUSH_BLOCK) {
    while (sem_wait(&queue->free_slots) == -1 && errno == EINTR)
      ;
  } else {
    if (sem_trywait(&queue->free_slots) == -1) {
      return errno;
    }
  }
  pthread_mutex_lock(&queue->rwlock);

  queue->job_pool[queue->tail].job = *job;
  queue->tail = (queue->tail + 1) % queue->cap;
  ++queue->len;

  pthread_mutex_unlock(&queue->rwlock);
  sem_post(&queue->jobs_in_q);
  return 0;
}

int job_fifo_pop(struct schw_pool *pool, struct schw_job *buf) {
  if (!(pool && buf)) {
    return EINVAL;
  }
  struct job_fifo *queue = pool->fqueue;
  pthread_mutex_lock(&queue->rwlock);

  *buf = queue->job_pool[queue->head].job;
  queue->head = (queue->head + 1) % queue->cap;
  --queue->len;

  pthread_mutex_unlock(&queue->rwlock);
  sem_post(&queue->free_slots);
  return 0;
}

int job_fifo_free(struct schw_pool *pool) {
  struct job_fifo *queue = pool->fqueue;
  pthread_mutex_lock(&queue->rwlock);

  // Ensure queue is empty before freeing
  int sval;
  sem_getvalue(&queue->jobs_in_q, &sval);
  if (sval > 0) {
    pthread_mutex_unlock(&queue->rwlock);
    return EBUSY;
  }

  pthread_mutex_unlock(&queue->rwlock);
  __job_fifo_free(queue);
  return 0;
}

// Binary heap functions for operating on the priority queue.
static void bheap_bup(struct job_key *keys, uint32_t idx);
static void bheap_bdown(struct job_key *keys, uint32_t idx, uint32_t len);
static void bheap_push(struct job_key *keys, uint32_t len, struct job_key *key);
static struct job_key bheap_pop(struct job_key *keys, uint32_t len);
static void bheap_heapify(struct job_key *keys, uint32_t len);

static inline void __job_pqueue_init_jobll(struct job *job_pool, uint32_t cap) {
  for (uint32_t i = 0; i < cap - 1; ++i) {
    job_pool[i].next_free = &job_pool[i + 1];
  }
  job_pool[cap - 1].next_free = NULL;
}

static inline int __job_pqueue_init(struct job_pqueue *queue, uint32_t cap) {
  /* Allocations */
  struct job_key *keys = malloc(cap * sizeof(struct job_key));
  if (unlikely(keys == NULL)) {
    goto unwind_0;
  }

  struct job *job_pool = malloc(cap * sizeof(struct job));
  if (unlikely(job_pool == NULL)) {
    goto unwind_1;
  }
  __job_pqueue_init_jobll(job_pool, cap);

  /* Initialize semaphore and mutexes. */
  if (pthread_mutex_init(&queue->rwlock, NULL)) {
    goto unwind_2;
  }
  if (sem_init(&queue->free_slots, 0, cap)) {
    goto unwind_3;
  }
  if (sem_init(&queue->jobs_in_q, 0, 0)) {
    goto unwind_4;
  }

  queue->keys = keys;
  queue->job_pool = job_pool;
  queue->first_free = job_pool;
  queue->cap = cap;
  queue->len = 0;
  return 0;

// Fall throughs to free resources if an error occurs.
unwind_4:
  sem_destroy(&queue->free_slots);
unwind_3:
  pthread_mutex_destroy(&queue->rwlock);
unwind_2:
  free(job_pool);
unwind_1:
  free(keys);
unwind_0:
  free(queue);
  return errno;
}

int job_pqueue_init(struct schw_pool *pool, uint32_t cap) {
  pool->pqueue = malloc(sizeof(struct job_pqueue));
  if (unlikely(pool->pqueue == NULL)) {
    return errno;
  }

  pool->push_job = job_pqueue_push;
  pool->pop_job = job_pqueue_pop;

  return __job_pqueue_init(pool->pqueue, cap);
}

int job_pqueue_push(struct schw_pool *pool, struct schw_job *job, uint32_t flags) {
  if (!(job && pool)) {
    return EINVAL;
  }
  struct job_pqueue *queue = pool->pqueue;
  if (flags & SCHW_PUSH_BLOCK) {
    while (sem_wait(&queue->free_slots) == -1 && errno == EINTR)
      ;
  } else {
    if (sem_trywait(&queue->free_slots) == -1) {
      return errno;
    }
  }
  pthread_mutex_lock(&queue->rwlock);

  // If this occurs, there is a bug.
  if (unlikely(queue->first_free == NULL)) {
    return EAGAIN;
  }

  struct job_key new_key = {
      .priority = job->priority,
      .job = queue->first_free,
  };
  bheap_push(queue->keys, queue->len, &new_key);

  struct job *next_free = queue->first_free->next_free;
  queue->first_free->job = *job;
  queue->first_free = next_free;
  ++queue->len;

  pthread_mutex_unlock(&queue->rwlock);
  sem_post(&queue->jobs_in_q);
  return 0;
}

int job_pqueue_pop(struct schw_pool *pool, struct schw_job *buf) {
  if (!(pool && buf)) {
    return EINVAL;
  }
  struct job_pqueue *queue = pool->pqueue;
  pthread_mutex_lock(&queue->rwlock);

  struct job_key key = bheap_pop(queue->keys, queue->len);
  *buf = key.job->job;

  key.job->next_free = queue->first_free;
  queue->first_free = key.job;
  --queue->len;

  pthread_mutex_unlock(&queue->rwlock);
  sem_post(&queue->free_slots);
  return 0;
}

int job_pqueue_free(struct schw_pool *pool) {
  struct job_pqueue *queue = pool->pqueue;
  pthread_mutex_lock(&queue->rwlock);
  if (queue->len > 0) {
    pthread_mutex_unlock(&queue->rwlock);
    return EBUSY;
  }
  pthread_mutex_unlock(&queue->rwlock);
  pthread_mutex_destroy(&queue->rwlock);
  free(queue->keys);
  free(queue->job_pool);
  sem_destroy(&queue->free_slots);
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
static void bheap_push(struct job_key *keys, uint32_t len,
                       struct job_key *key) {
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
