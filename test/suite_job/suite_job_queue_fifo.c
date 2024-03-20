#ifdef SCHW_SCHED_FIFO

#include "unity.h"

#include "job.h"
#include "suite_job.h"

void test_job_queue_fifo_init(void) {
  test_job_queue_common_init();
  TEST_ASSERT_EQUAL(0, queue.queue.head);
  TEST_ASSERT_EQUAL(0, queue.queue.tail);
}

void test_job_queue_fifo_push(void) {
  test_job_queue_common_push();
  // Above function makes queue full.
  TEST_ASSERT_EQUAL(0, queue.queue.head);
  TEST_ASSERT_EQUAL(0, queue.queue.tail);
}

void test_job_queue_fifo_pop(void) {
  test_job_queue_common_pop();
  // Above function makes queue empty.
  TEST_ASSERT_EQUAL(0, queue.queue.head);
  TEST_ASSERT_EQUAL(0, queue.queue.tail);
}

int main(void) {
  UNITY_BEGIN();
  run_common_tests();

  RUN_TEST(test_job_queue_fifo_init);
  RUN_TEST(test_job_queue_fifo_push);
  RUN_TEST(test_job_queue_fifo_pop);

  return UNITY_END();
}

#endif // SCHW_SCHED_FIFO
