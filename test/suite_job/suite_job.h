#ifndef SUITE_JOB_H
#define SUITE_JOB_H

#define CAP 10

extern struct schw_pool pool;

void run_common_tests(void);

void test_job_queue_common_push_NULL(void);
void test_job_queue_common_pop_NULL(void);

void test_job_queue_common_push(void);
void test_job_queue_common_pop(void);

#endif // SUITE_JOB_H
