#ifdef SCHW_SCHED_PRIORITY

#include "unity.h"

#include "job.h"
#include "suite_job.h"

void test_job_queue_priority_init(void) {
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

int main(void) {
  UNITY_BEGIN();

  run_common_tests();
  RUN_TEST(test_job_queue_priority_init);

  return UNITY_END();
}

#endif // SCHW_SCHED_PRIORITY
