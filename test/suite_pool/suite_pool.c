#include "job.h"
#include "unity.h"

#include <pthread.h>

#include "schwimmbad.h"
#include "thread.h"

#define NUM_THREADS 2

static struct job_fifo queue = { 0 };
static struct schw_pool pool = { 0 };

void setUp(void) {
  // Without job queue ptr, thread pool
  // init will dereference a null pointer.
  pool.fqueue = &queue;
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
