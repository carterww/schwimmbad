#ifndef SCHW_JOB_H
#define SCHW_JOB_H

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

// Default scheduling policy will be FIFO
#ifndef SCHW_SCHED_PRIORITY
#define SCHW_SCHED_FIFO
#else
#define SCHW_SCHED_PRIORITY
#endif

#define SCHW_MAX_JOBS 256

// TMP
#define SCHW_SCHED_PRIORITY

struct job {
  union {
    struct {
      void *(*job_func)(void *arg);
      void *job_arg;
    };
    struct job *next_free;
  };
};

#ifdef SCHW_SCHED_FIFO

struct job_fifo {
  pthread_mutex_t rwlock;
  struct job *jobs;
  uint32_t head;
  uint32_t tail;
  uint32_t cap;
  sem_t free_slots;
  uint32_t len;
};

inline int job_fifo_init(struct job_fifo *fifo, uint32_t cap);
int job_fifo_push(struct job_fifo *fifo, struct job *job);
int job_fifo_pop(struct job_fifo *fifo, struct job *buf);
int job_fifo_free(struct job_fifo *fifo);

#endif // SCHW_SCHED_FIFO

#ifdef SCHW_SCHED_PRIORITY

/*
 * How often should the scheduler invoke the aging algorithm.
 * 128 signifies that the scheduler will invoke the aging algorithm
 * every 128th time it is called.
 */
#define SCHW_INVOKE_AGE 128

struct job_key {
  int32_t priority;
  uint32_t last_age;
  struct job *job;
};

struct job_pqueue {
  pthread_mutex_t rwlock;
  struct job_key *keys;
  struct job *job_pool;
  struct job *first_free;
  sem_t free_slots;
  uint32_t cap;
  uint32_t len;
};

inline int job_pqueue_init(struct job_pqueue *pqueue, uint32_t cap);
int job_pqueue_push(struct job_pqueue *pqueue, struct job *job, int32_t priority);
int job_pqueue_pop(struct job_pqueue *pqueue, struct job *buf);
int job_pqueue_free(struct job_pqueue *pqueue);

#endif // SCHW_SCHED_PRIORITY

#ifdef __cplusplus
}
#endif

#endif // SCHW_JOB_H
