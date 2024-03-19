#ifdef SCHW_SCHED_FIFO

#include "unity.h"


#include "job.h"
#include "suite_job.h"

void test_job_queue_fifo_init(void) {
  TEST_ASSERT_EQUAL(0, queue.queue.head);
  TEST_ASSERT_EQUAL(0, queue.queue.tail);
}

int main(void) {
  UNITY_BEGIN();

  run_common_tests();
  RUN_TEST(test_job_queue_fifo_init);

  return UNITY_END();
}

#endif // SCHW_SCHED_FIFO
