#include "unity.h"

#include "job.h"
#include "suite_job.h"

#include "schwimmbad.h"

struct schw_pool pool = { 0 };

void setUp(void) {
  pool.policy = PRIORITY;
  int init_res = job_pqueue_init(&pool, CAP);
}

void tearDown(void) {
  job_pqueue_free(&pool);
  pool = (struct schw_pool){ 0 };
}

void test_job_queue_priority_init(void) {
  TEST_ASSERT_EQUAL(CAP, pool.pqueue->cap);
  TEST_ASSERT_EQUAL(0, pool.pqueue->len);
  int free_slots;
  int jobs_in_q;
  sem_getvalue(&pool.pqueue->free_slots, &free_slots);
  sem_getvalue(&pool.pqueue->jobs_in_q, &jobs_in_q);
  TEST_ASSERT_EQUAL(CAP, free_slots);
  TEST_ASSERT_EQUAL(0, jobs_in_q);
  TEST_ASSERT_NOT_NULL(pool.pqueue->job_pool);
  TEST_ASSERT_EQUAL(0, pthread_mutex_trylock(&pool.pqueue->rwlock));
  TEST_ASSERT_EQUAL(0, pthread_mutex_unlock(&pool.pqueue->rwlock));

  TEST_ASSERT_NOT_NULL(pool.pqueue->keys);
  TEST_ASSERT_EQUAL_PTR(pool.pqueue->job_pool, pool.pqueue->first_free);
  // Ensure the linked list of the free job pool is complete and correct.
  struct job *next_job = pool.pqueue->first_free->next_free;
  for (uint32_t i = 1; i < CAP; ++i) {
    TEST_ASSERT_EQUAL_PTR(&pool.pqueue->job_pool[i], next_job);
    next_job = next_job->next_free;
  }
  // Ensure the last job in the pool points to NULL.
  TEST_ASSERT_NULL(next_job);
}

void test_job_queue_priority_push(void) {
  // Ensure the push function updates the queue correctly
  // by putting highest priority in front.
  for (uint32_t i = CAP; i > 0; --i) {
    struct schw_job job = { 0 };
    job.id = CAP - i;
    job.priority = i;
    int push_res = pool.push_job(&pool, &job, 0);
    TEST_ASSERT_EQUAL(0, push_res);
    TEST_ASSERT_EQUAL(i, pool.pqueue->keys[0].priority);
    // Next line is hyper impmlementation specific. If implementation
    // of job_pool changes at all, this line will need to be updated.
    TEST_ASSERT_EQUAL_PTR(pool.pqueue->keys[0].job, pool.pqueue->job_pool + job.id);
    TEST_ASSERT_EQUAL(job.id, pool.pqueue->keys[0].job->job.id);
  }
  TEST_ASSERT_NULL(pool.pqueue->first_free);
}

void test_job_queue_priority_pop(void) {
  // Ensure highest priority job is popped first.
  // Use the same jobs as in the push test.
  test_job_queue_priority_push();
  int32_t last_priority = INT32_MIN;
  for (uint32_t i = 0; i < CAP; ++i) {
    struct schw_job job = { 0 };
    int pop_res = pool.pop_job(&pool, &job);
    TEST_ASSERT_EQUAL(0, pop_res);
    // Jobs with higher priority should be popped first.
    // These have higher ids. Ids should go from CAP - 1 to 0.
    TEST_ASSERT_EQUAL(CAP - i - 1, job.id);
    TEST_ASSERT_GREATER_THAN(last_priority, job.priority);
    last_priority = job.priority;
  }
  TEST_ASSERT_NOT_NULL(pool.pqueue->first_free);
  // Ensure linked list is rebuilt
  struct job *next_job = pool.pqueue->first_free->next_free;
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
