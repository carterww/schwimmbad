#ifndef SUITE_JOB_H
#define SUITE_JOB_H

#define CAP 10

extern struct job_queue queue;

void setUp(void);
void tearDown(void);

void run_common_tests(void);

void test_job_queue_common_init(void);
void test_job_queue_common_push(void);
void test_job_queue_common_pop(void);

#endif // SUITE_JOB_H
