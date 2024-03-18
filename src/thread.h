#ifndef SCHW_THREAD_H
#define SCHW_THREAD_H

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include "job.h"
#include "schwimmbad.h"

#ifdef __cplusplus
extern "C" {
#endif

// SHOULD BE IFNDEF WHEN IMPLEMENTED
#ifdef DISABLE_DYNAMIC_SIZING
#define THREAD_POOL_DYNAMIC_SIZING
#endif

struct thread {
  pthread_t tid;
  jid job_id;
};

/*
 * @summary: Initialize the thread pool with the given number of threads.
 * Allocates memory for the thread pool and starts each thread.
 * @param pool: The thread pool to initialize.
 * @param num_threads: The number of threads to start.
 * @return: 0 on success, error code on failure.
 * @error: errno when malloc, pthread_create, or pthread_detach fails.
 */
int thread_pool_init(struct schw_pool *pool, uint32_t num_threads);

/*
 * @summary: Free the thread pool and all of its resources.
 * If there are any threads working, then this function will
 * return an error.
 * @param pool: The thread pool to free.
 * @return: 0 on success, error code on failure.
 * @error: EBUSY if there are still threads working.
 */
int thread_pool_free(struct schw_pool *pool);

#ifdef THREAD_POOL_DYNAMIC_SIZING

int thread_pool_resize(struct schw_pool *pool, int32_t change);

#endif // THREAD_POOL_DYNAMIC_SIZING

#ifdef __cplusplus
}
#endif

#endif // SCHW_THREAD_H
