#ifdef SCHW_SCHED_PRIORITY

#include "unity.h"

#include "job.h"
#include "suite_job.h"

void test_job_queue_priority_init(void) {
  test_job_queue_common_init();
  TEST_ASSERT_NOT_NULL(queue.queue.keys);
  TEST_ASSERT_EQUAL_PTR(queue.queue.job_pool, queue.queue.first_free);
  // Ensure the linked list of the free job pool is complete and correct.
  struct job *next_job = queue.queue.first_free->next_free;
  for (uint32_t i = 1; i < CAP; ++i) {
    TEST_ASSERT_EQUAL_PTR(&queue.queue.job_pool[i], next_job);
    next_job = next_job->next_free;
  }
  // Ensure the last job in the pool points to NULL.
  TEST_ASSERT_NULL(next_job);
}

void test_job_queue_priority_push(void) {
  // Ensure the push function updates the queue correctly
  // by putting highest priority in front.
  for (uint32_t i = CAP; i > 0; --i) {
    struct job job = { 0 };
    job.id = CAP - i;
    job.job.priority = i;
    int push_res = job_push(&queue, &job);
    TEST_ASSERT_EQUAL(0, push_res);
    TEST_ASSERT_EQUAL(i, queue.queue.keys[0].priority);
    // Next line is hyper impmlementation specific. If implementation
    // of job_pool changes at all, this line will need to be updated.
    TEST_ASSERT_EQUAL_PTR(queue.queue.keys[0].job, queue.queue.job_pool + job.id);
    TEST_ASSERT_EQUAL(job.id, queue.queue.keys[0].job->id);
  }
  TEST_ASSERT_NULL(queue.queue.first_free);
}

void test_job_queue_priority_pop(void) {
  // Ensure highest priority job is popped first.
  // Use the same jobs as in the push test.
  test_job_queue_priority_push();
  int32_t last_priority = INT32_MIN;
  for (uint32_t i = 0; i < CAP; ++i) {
    struct job job = { 0 };
    int pop_res = job_pop(&queue, &job);
    TEST_ASSERT_EQUAL(0, pop_res);
    // Jobs with higher priority should be popped first.
    // These have higher ids. Ids should go from CAP - 1 to 0.
    TEST_ASSERT_EQUAL(CAP - i - 1, job.id);
    TEST_ASSERT_GREATER_THAN(last_priority, job.job.priority);
    last_priority = job.job.priority;
  }
  TEST_ASSERT_NOT_NULL(queue.queue.first_free);
  // Ensure linked list is rebuilt
  struct job *next_job = queue.queue.first_free->next_free;
  for (uint32_t i = 1; i < CAP; ++i) {
    TEST_ASSERT_NOT_NULL(next_job);
    next_job = next_job->next_free;
  }
  TEST_ASSERT_NULL(next_job);
}

int main(void) {
  UNITY_BEGIN();
  run_common_tests();
  RUN_TEST(test_job_queue_common_push);
  RUN_TEST(test_job_queue_common_pop);
  
  RUN_TEST(test_job_queue_priority_init);
  RUN_TEST(test_job_queue_priority_push);
  RUN_TEST(test_job_queue_priority_pop);

  return UNITY_END();
}

#endif // SCHW_SCHED_PRIORITY
