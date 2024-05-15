#include "schwimmbad.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "job.h"
#include "thread.h"

jid schw_push(struct schw_pool *pool, struct schw_job *job) {
  if (!job->job_func)
    return -1;

  pthread_spin_lock(&pool->jid_helper.lock);
  job->id = pool->jid_helper.current;
  if (unlikely(pool->jid_helper.current == INT64_MAX))
    pool->jid_helper.current = -1;
  ++pool->jid_helper.current;
  pthread_spin_unlock(&pool->jid_helper.lock);


  if (pool->push_job(pool, job) != 0)
    return -1;

  return job->id;
}

int schw_init(struct schw_pool *pool, uint32_t num_threads,
              uint32_t queue_len, enum schw_job_queue_policy policy) {

  if (pool == NULL)
    return EINVAL;

  int err = 0;
  if (policy == FIFO)
    err = job_fifo_init(pool, queue_len);
  else if (policy == PRIORITY)
    err = job_pqueue_init(pool, queue_len);
  else
    return EINVAL;

  if (err != 0)
    return err;

  err = thread_pool_init(pool, num_threads);
  if (err != 0)
    return err;

  pool->jid_helper.current = 0;
  pthread_spin_init(&pool->jid_helper.lock, 0);

  pool->policy = policy;

  return 0;
}

// TODO: Move checks to this function from the two
// frees to make sure they are always called
// together.
int schw_free(struct schw_pool *pool) {
  if (pool == NULL)
    return EINVAL;
  job_free(pool);
  thread_pool_free(pool);
  return 0;
}

// TODO: wrap in mutex to ensure integrity
// and correctness of value
uint32_t schw_threads(struct schw_pool *pool) { return pool->num_threads; }

// TODO: same as schw_threads
uint32_t schw_working_threads(struct schw_pool *pool) {
  return pool->working_threads;
}

// TODO: same as schw_threads
uint32_t schw_queue_len(struct schw_pool *pool) {
  if (pool->policy == FIFO) {
    return pool->fqueue->len;
  } else if (pool->policy == PRIORITY) {
    return pool->pqueue->len;
  } else {
    return -1;
  }
}

// TODO: same as schw_threads
uint32_t schw_queue_cap(struct schw_pool *pool) {
  if (pool->policy == FIFO) {
    return pool->fqueue->cap;
  } else if (pool->policy == PRIORITY) {
    return pool->pqueue->cap;
  } else {
    return -1;
  }
}

int schw_pool_resize(struct schw_pool *pool, int32_t change) { return 1; }
int schw_queue_resize(struct schw_pool *pool, int32_t change) { return 1; }
