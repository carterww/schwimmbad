/*
 * This file tests the common attributes shared by all job queue
 * implementations. More specific test cases that are not agnostic
 * to the implementation are found in suite_job_queue_fifo.c and
 * suite_job_queue_priority.c. Those files build off these tests cases
 * and run them for their respective implementations.
 */

#include "unity.h"

#include <errno.h>

#include "job.h"
#include "suite_job.h"

/*
 * @summary: Ensure the push function does not accept NULL pointers.
 */
void test_job_queue_common_push_NULL(void) {
  struct schw_job job = { 0 };
  int push_res = pool.push_job(NULL, &job);
  TEST_ASSERT_EQUAL(EINVAL, push_res);
  int push_res2 = pool.push_job(&pool, NULL);
  TEST_ASSERT_EQUAL(EINVAL, push_res2);
}

/*
 * @summary: Ensure the push function updates the queue correctly.
 * The queue should be full after this test. Ensure the push function
 * returns EAGAIN when the queue is full.
 */
void test_job_queue_common_push(void) {
  uint32_t *len;
  sem_t *free_slots_sem;
  sem_t *jobs_in_q_sem;
  if (pool.policy == FIFO) {
    len = &pool.fqueue->len;
    free_slots_sem = &pool.fqueue->free_slots;
    jobs_in_q_sem = &pool.fqueue->jobs_in_q;
  } else if (pool.policy == PRIORITY) {
    len = &pool.pqueue->len;
    free_slots_sem = &pool.pqueue->free_slots;
    jobs_in_q_sem = &pool.pqueue->jobs_in_q;
  }

  for (jid i = 0; i < CAP; ++i) {
    struct schw_job job = { 0 };
    job.id = i;
    int push_res = pool.push_job(&pool, &job);
    TEST_ASSERT_EQUAL(0, push_res);
    TEST_ASSERT_EQUAL(i + 1, *len);
    int free_slots;
    int jobs_in_q;
    sem_getvalue(free_slots_sem, &free_slots);
    sem_getvalue(jobs_in_q_sem, &jobs_in_q);
    TEST_ASSERT_EQUAL(CAP - i - 1, free_slots);
    TEST_ASSERT_EQUAL(i + 1, jobs_in_q);
  }
  struct schw_job job = { 0 };
  job.id = CAP;
  int push_res = pool.push_job(&pool, &job);
  TEST_ASSERT_EQUAL(EAGAIN, push_res);
}

/*
 * @summary: Ensure the pop function does not accept NULL pointers.
 */
void test_job_queue_common_pop_NULL(void) {
  struct schw_job job = { 0 };
  int pop_res = pool.pop_job(NULL, &job);
  TEST_ASSERT_EQUAL(EINVAL, pop_res);
  int pop_res2 = pool.pop_job(&pool, NULL);
  TEST_ASSERT_EQUAL(EINVAL, pop_res2);
}

/*
 * @summary: Ensure the pop function updates the queue correctly.
 */
void test_job_queue_common_pop(void) {
  struct schw_job buf = { 0 };
  int no_job_res = pool.pop_job(&pool, &buf);
  TEST_ASSERT_EQUAL(EAGAIN, no_job_res);
  for (jid i = 0; i < CAP; ++i) {
    struct schw_job job = { 0 };
    job.id = i;
    int push_res = pool.push_job(&pool, &job);
  }

  uint32_t *len;
  sem_t *free_slots_sem;
  sem_t *jobs_in_q_sem;
  if (pool.policy == FIFO) {
    len = &pool.fqueue->len;
    free_slots_sem = &pool.fqueue->free_slots;
    jobs_in_q_sem = &pool.fqueue->jobs_in_q;
  } else {
    len = &pool.pqueue->len;
    free_slots_sem = &pool.pqueue->free_slots;
    jobs_in_q_sem = &pool.pqueue->jobs_in_q;
  }

  for (jid i = 0; i < CAP; ++i) {
    int pop_res = pool.pop_job(&pool, &buf);
    TEST_ASSERT_EQUAL(0, pop_res);
    // Don't know the order they will be popped. Just
    // ensure they are within the range of the job id's.
    TEST_ASSERT_LESS_THAN(CAP, buf.id);
    TEST_ASSERT_GREATER_THAN(-1, buf.id);
    TEST_ASSERT_EQUAL(CAP - i - 1, *len);
    // Ensure semaphores are updated correctly.
    int free_slots;
    int jobs_in_q;
    sem_getvalue(free_slots_sem, &free_slots);
    sem_getvalue(jobs_in_q_sem, &jobs_in_q);
    TEST_ASSERT_EQUAL(i + 1, free_slots);
    TEST_ASSERT_EQUAL(CAP - i - 1, jobs_in_q);
  }
}

void run_common_tests(void) {
  RUN_TEST(test_job_queue_common_push_NULL);
  RUN_TEST(test_job_queue_common_pop_NULL);
}
