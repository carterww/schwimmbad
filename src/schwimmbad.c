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

  struct job j = {0};
  pthread_spin_lock(&pool->jid_helper.lock);
  j.id = pool->jid_helper.current;
  if (unlikely(pool->jid_helper.current == INT64_MAX))
    pool->jid_helper.current = -1;
  ++pool->jid_helper.current;
  pthread_spin_unlock(&pool->jid_helper.lock);
  j.job = *job;

  if (job_push(pool->queue, &j) != 0)
    return -1;
  return j.id;
}

int schw_init(struct schw_pool *pool, uint32_t num_threads,
              uint32_t queue_len) {

  if (pool == NULL)
    return EINVAL;
  pool->queue = malloc(sizeof(struct job_queue));
  if (pool->queue == NULL)
    return errno;
  int err = job_init(pool->queue, queue_len);
  if (err != 0)
    return err;

  err = thread_pool_init(pool, num_threads);
  if (err != 0)
    return err;

  pool->jid_helper.current = 0;
  pthread_spin_init(&pool->jid_helper.lock, 0);

  return 0;
}

// TODO: Move checks to this function from the two
// frees to make sure they are always called
// together.
int schw_free(struct schw_pool *pool) {
  job_free(pool->queue);
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
  return pool->queue->queue.len;
}

// TODO: same as schw_threads
uint32_t schw_queue_cap(struct schw_pool *pool) {
  return pool->queue->queue.len;
}

int schw_pool_resize(struct schw_pool *pool, int32_t change) { return 1; }
int schw_queue_resize(struct schw_pool *pool, int32_t change) { return 1; }
