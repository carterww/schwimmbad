#include "unity.h"

#include <pthread.h>

#include "schwimmbad.h"
#include "thread.h"
#include "job.h"

#define NUM_THREADS 2

static struct schw_pool pool = { 0 };
static struct job_queue queue = { 0 };

void setUp(void) {
  // If no queue, thread will dereference a NULL pointer.
  pool.queue = &queue;
  int init_res = thread_pool_init(&pool, NUM_THREADS);
}

void tearDown(void) {
  int free_res = thread_pool_free(&pool);
}

void test_pool_init(void) {
  TEST_ASSERT_NOT_NULL(pool.threads);
  TEST_ASSERT_EQUAL(NUM_THREADS, pool.num_threads);
  TEST_ASSERT_EQUAL(0, pool.working_threads);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pool_init);

  return UNITY_END();
}
