#include "thread.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "common.h"

static void *start_thread(void * arg);

static void job_ready_handler(int sig);

struct start_thread_arg {
  struct thread *thread;
  struct thread_pool *pool;
};

int thread_pool_init(struct thread_pool *pool, uint32_t num_threads,
                     uint32_t max_threads) {
  struct thread *threads = malloc(num_threads * sizeof(struct thread));
  if (unlikely(threads == NULL)) {
    return errno;
  }
  for (uint32_t i = 0; i < num_threads - 1; ++i) {
    threads[i].next_free = &threads[i + 1];
  }
  threads[num_threads - 1].next_free = NULL;

  pthread_mutex_init(&pool->rwlock, NULL);
  pool->threads = threads;
  pool->num_threads = num_threads;
  pool->working_threads = 0;
  pool->max_threads = max_threads;


  int err = 0;
  for (; threads != pool->threads + max_threads; threads++) {
    struct start_thread_arg *arg = malloc(sizeof(struct start_thread_arg));
    arg->pool = pool;
    arg->thread = threads;
    if (arg == NULL) {
      err = errno;
      break;
    }

    err = pthread_create(&threads->tid, NULL, start_thread, (void *) &arg);
    if (err != 0)
      break;

    err = pthread_detach(threads->tid);
    if (err != 0)
      break;
  }

  if (err != 0) {
    thread_pool_free(pool);
    return err;
  }

  return 0;
}

int thread_pool_free(struct thread_pool *pool) {
  pthread_mutex_lock(&pool->rwlock);
  if(pool->working_threads > 0) {
    pthread_mutex_unlock(&pool->rwlock);
    return EBUSY;
  }
  pthread_mutex_unlock(&pool->rwlock);
  pthread_mutex_destroy(&pool->rwlock);
  free(pool->threads);
  pool->threads = NULL;
  pool->num_threads = 0;
  pool->working_threads = 0;
  pool->max_threads = 0;
  return 0;
}

static void *start_thread(void *arg) {
  struct start_thread_arg *t_arg = (struct start_thread_arg *)arg;
  struct sigaction act = { 0 };
  act.sa_handler = job_ready_handler;
  act.sa_flags = SA_NODEFER;
  int sig_res = sigaction(SIGUSR1, &act, NULL);
  if (sig_res != 0)
    pthread_exit((void *)1);


  // Wait on signal to start job
  while (pause()) {
    
  }

}

#ifdef THREAD_POOL_DYNAMIC_SIZING

int thread_pool_resize(struct thread_pool *pool, int32_t change) {}

#endif // THREAD_POOL_DYNAMIC_SIZING

static void job_ready_handler(int sig) {

}
