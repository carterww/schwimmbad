#include "unity.h"

#include "job.h"
#include "suite_job.h"

struct schw_pool pool = { 0 };

void setUp(void) {
  pool.policy = FIFO;
  int init_res = job_fifo_init(&pool, CAP);
  TEST_ASSERT_EQUAL(0, init_res);
}

void tearDown(void) {
  job_fifo_free(&pool);
  pool = (struct schw_pool){ 0 };
}

void test_job_queue_fifo_init(void) {
  TEST_ASSERT_EQUAL(CAP, pool.fqueue->cap);
  TEST_ASSERT_EQUAL(0, pool.fqueue->len);
  int free_slots;
  int jobs_in_q;
  sem_getvalue(&pool.fqueue->free_slots, &free_slots);
  sem_getvalue(&pool.fqueue->jobs_in_q, &jobs_in_q);
  TEST_ASSERT_EQUAL(CAP, free_slots);
  TEST_ASSERT_EQUAL(0, jobs_in_q);
  TEST_ASSERT_NOT_NULL(pool.fqueue->job_pool);
  TEST_ASSERT_EQUAL(0, pthread_mutex_trylock(&pool.fqueue->rwlock));
  TEST_ASSERT_EQUAL(0, pthread_mutex_unlock(&pool.fqueue->rwlock));
  TEST_ASSERT_NOT_NULL(pool.fqueue->job_pool);

  TEST_ASSERT_EQUAL(0, pool.fqueue->head);
  TEST_ASSERT_EQUAL(0, pool.fqueue->tail);
}

void test_job_queue_fifo_push(void) {
  test_job_queue_common_push();
  // Above function makes queue full.
  TEST_ASSERT_EQUAL(0, pool.fqueue->head);
  TEST_ASSERT_EQUAL(0, pool.fqueue->tail);
}

void test_job_queue_fifo_pop(void) {
  test_job_queue_common_pop();
  // Above function makes queue empty.
  TEST_ASSERT_EQUAL(0, pool.fqueue->head);
  TEST_ASSERT_EQUAL(0, pool.fqueue->tail);
}

int main(void) {
  UNITY_BEGIN();
  run_common_tests();

  RUN_TEST(test_job_queue_fifo_init);
  RUN_TEST(test_job_queue_fifo_push);
  RUN_TEST(test_job_queue_fifo_pop);

  return UNITY_END();
}
