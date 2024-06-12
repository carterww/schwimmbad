#include "schwimmbad.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "job.h"
#include "thread.h"

jid schw_push(struct schw_pool *pool, struct schw_job *job, uint32_t flags) {
  if (!job || !job->job_func)
    return -1;

  // If a job is created every ns, then it will take 292.3 years
  // to overflow the jid (64 byte signed integer).
  SPIN_LOCK_AND_OP(pool->jid_helper.lock, {
    job->id = pool->jid_helper.current++;
  });

  if (pool->push_job(pool, job, flags) != 0)
    return -1;

  return job->id;
}

int schw_init(struct schw_pool *pool, uint32_t num_threads,
              uint32_t queue_len, enum schw_job_queue_policy policy) {
  if (pool == NULL)
    return EINVAL;
  pool->policy = policy;

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
  return err; // 0 if no error, no need to check
}

void schw_set_callback(struct schw_pool *pool, job_cb cb, void *arg) {
  if (pool == NULL)
    return;
  
  MUTEX_LOCK_AND_OP(pool->rwlock, {
      pool->cb = cb;
      pool->cb_arg = arg;
  });
}

void schw_wait_all(struct schw_pool *pool) {
  if (pool == NULL)
    return;

  MUTEX_LOCK_AND_OP(pool->rwlock, {
      uint32_t queue_len = 0;
      QUEUE_GET_MEMBER(pool, queue_len, len);
      if (pool->working_threads == 0 &&
          queue_len == 0) {
        goto locked_exit;
      }
      pool->flags |= SCHW_POOL_FLAG_WAIT_ALL;
  });
  sem_wait(&pool->sync);
  MUTEX_LOCK_AND_OP(pool->rwlock, {
      pool->flags &= ~SCHW_POOL_FLAG_WAIT_ALL;
  });
locked_exit:
  pthread_mutex_unlock(&pool->rwlock);
}

// TODO: Move checks to this function from the two
// frees to make sure they are always called
// together.
int schw_free(struct schw_pool *pool) {
  if (pool == NULL)
    return EINVAL;
  if (pool->policy == FIFO)
    job_fifo_free(pool);
  else if (pool->policy == PRIORITY)
    job_pqueue_free(pool);
  else
    return EINVAL;
  thread_pool_free(pool);
  return 0;
}

uint32_t schw_threads(struct schw_pool *pool) {
  uint32_t num_threads = 0;
  MUTEX_LOCK_AND_OP(pool->rwlock, {
      num_threads = pool->num_threads;
  });
  return num_threads;
}

uint32_t schw_working_threads(struct schw_pool *pool) {
  uint32_t working_threads = 0;
  MUTEX_LOCK_AND_OP(pool->rwlock, {
      working_threads = pool->working_threads;
  });
  return working_threads;
}

uint32_t schw_queue_len(struct schw_pool *pool) {
  uint32_t len = 0;
  MUTEX_LOCK_AND_OP(pool->rwlock, {
      QUEUE_GET_MEMBER(pool, len, len);
  });
  return len;
}

uint32_t schw_queue_cap(struct schw_pool *pool) {
  uint32_t cap = 0;
  MUTEX_LOCK_AND_OP(pool->rwlock, {
      QUEUE_GET_MEMBER(pool, cap, cap);
  });
  return cap;
}
