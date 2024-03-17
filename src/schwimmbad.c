#include "schwimmbad.h"

#include <pthread.h>

#include "job.h"
#include "thread.h"

static jid job_counter = 0;
static pthread_spinlock_t job_counter_lock;

static struct thread_pool pool = { 0 };

#ifdef SCHW_SCHED_FIFO
static struct job_fifo queue = { 0 };

jid schw_push(void *(*job_func)(void*), void *arg) {
  if (!job_func)
    return -1;

  struct job j = { 0 };
  pthread_spin_lock(&job_counter_lock);
  j.job_id = job_counter++;
  pthread_spin_unlock(&job_counter_lock);

  j.job_func = job_func;
  j.job_arg = arg;
  job_fifo_push(&queue, &j);
  return j.job_id;
}

#endif

#ifdef SCHW_SCHED_PRIORITY
static struct job_pqueue queue = { 0 };

jid schw_push(void *(*job_func)(void*), void *arg, int32_t priority);
#endif

int schw_init(uint32_t num_threads, uint32_t queue_len) {
  pthread_spin_init(&job_counter_lock, 0);

  int err = job_init(&queue, queue_len);
  if (err != 0)
    return err;

  err = thread_pool_init(&pool, (void *)&queue, num_threads);
  if (err != 0)
    return err;

  return 0;
}

void schw_free() {
  thread_pool_free(&pool);
  job_free(&queue);
}

// TODO: wrap in mutex to ensure integrity
// and correctness of value
uint32_t schw_threads() {
  return pool.num_threads;
}

// TODO: same as schw_threads
uint32_t schw_working_threads() {
  return pool.working_threads;
}

// TODO: same as schw_threads
uint32_t schw_queue_len() {
  return queue.len;
}

// TODO: same as schw_threads
uint32_t schw_queue_cap() {
  return queue.cap;
}

int schw_pool_resize(int32_t change) { return 1; }
int schw_queue_resize(int32_t change) { return 1; }

