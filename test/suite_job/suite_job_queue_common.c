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

struct job_queue queue = { 0 };

void setUp(void) {
  int init_res = job_init(&queue, CAP);
}

void tearDown(void) {
  job_free(&queue);
  queue = (struct job_queue){ 0 };
}

/*
 * @summary: Ensure the init function does not accept NULL pointers.
 */
void test_job_queue_common_init_NULL(void) {
  int init_res = job_init(NULL, CAP);
  TEST_ASSERT_EQUAL(EINVAL, init_res);
}

/*
 * @summary: Ensure the job queue is initialized correctly.
 */
void test_job_queue_common_init(void) {
  // Common attributes shared by FIFO and priority queues.
  TEST_ASSERT_EQUAL(CAP, queue.queue.cap);
  TEST_ASSERT_EQUAL(0, queue.queue.len);
  int free_slots;
  int jobs_in_q;
  sem_getvalue(&queue.queue.free_slots, &free_slots);
  sem_getvalue(&queue.queue.jobs_in_q, &jobs_in_q);
  TEST_ASSERT_EQUAL(CAP, free_slots);
  TEST_ASSERT_EQUAL(0, jobs_in_q);
  TEST_ASSERT_NOT_NULL(queue.queue.job_pool);
  TEST_ASSERT_EQUAL(0, pthread_mutex_trylock(&queue.queue.rwlock));
  TEST_ASSERT_EQUAL(0, pthread_mutex_unlock(&queue.queue.rwlock));
  TEST_ASSERT_NOT_NULL(queue.queue.job_pool);
}

/*
 * @summary: Ensure the push function does not accept NULL pointers.
 */
void test_job_queue_common_push_NULL(void) {
  struct job job = { 0 };
  int push_res = job_push(NULL, &job);
  TEST_ASSERT_EQUAL(EINVAL, push_res);
  int push_res2 = job_push(&queue, NULL);
  TEST_ASSERT_EQUAL(EINVAL, push_res2);
}

/*
 * @summary: Ensure the push function updates the queue correctly.
 * The queue should be full after this test. Ensure the push function
 * returns EAGAIN when the queue is full.
 */
void test_job_queue_common_push(void) {
  for (jid i = 0; i < CAP; ++i) {
    struct job job = { 0 };
    job.id = i;
    int push_res = job_push(&queue, &job);
    TEST_ASSERT_EQUAL(0, push_res);
    TEST_ASSERT_EQUAL(i + 1, queue.queue.len);
    int free_slots;
    int jobs_in_q;
    sem_getvalue(&queue.queue.free_slots, &free_slots);
    sem_getvalue(&queue.queue.jobs_in_q, &jobs_in_q);
    TEST_ASSERT_EQUAL(CAP - i - 1, free_slots);
    TEST_ASSERT_EQUAL(i + 1, jobs_in_q);
  }
  struct job job = { 0 };
  job.id = CAP;
  int push_res = job_push(&queue, &job);
  TEST_ASSERT_EQUAL(EAGAIN, push_res);
}

/*
 * @summary: Ensure the pop function does not accept NULL pointers.
 */
void test_job_queue_common_pop_NULL(void) {
  struct job job = { 0 };
  int pop_res = job_pop(NULL, &job);
  TEST_ASSERT_EQUAL(EINVAL, pop_res);
  int pop_res2 = job_pop(&queue, NULL);
  TEST_ASSERT_EQUAL(EINVAL, pop_res2);
}

/*
 * @summary: Ensure the pop function updates the queue correctly.
 */
void test_job_queue_common_pop(void) {
  struct job buf = { 0 };
  int no_job_res = job_pop(&queue, &buf);
  TEST_ASSERT_EQUAL(EAGAIN, no_job_res);
  for (jid i = 0; i < CAP; ++i) {
    struct job job = { 0 };
    job.id = i;
    int push_res = job_push(&queue, &job);
  }
  for (jid i = 0; i < CAP; ++i) {
    int pop_res = job_pop(&queue, &buf);
    TEST_ASSERT_EQUAL(0, pop_res);
    // Don't know the order they will be popped. Just
    // ensure they are within the range of the job id's.
    TEST_ASSERT_LESS_THAN(CAP, buf.id);
    TEST_ASSERT_GREATER_THAN(-1, buf.id);
    TEST_ASSERT_EQUAL(CAP - i - 1, queue.queue.len);
    // Ensure semaphores are updated correctly.
    int free_slots;
    int jobs_in_q;
    sem_getvalue(&queue.queue.free_slots, &free_slots);
    sem_getvalue(&queue.queue.jobs_in_q, &jobs_in_q);
    TEST_ASSERT_EQUAL(i + 1, free_slots);
    TEST_ASSERT_EQUAL(CAP - i - 1, jobs_in_q);
  }
}

void run_common_tests(void) {
  RUN_TEST(test_job_queue_common_init_NULL);
  RUN_TEST(test_job_queue_common_push_NULL);
  RUN_TEST(test_job_queue_common_pop_NULL);
}
