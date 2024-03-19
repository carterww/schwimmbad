#ifndef SUITE_JOB_H
#define SUITE_JOB_H

#define CAP 10

extern struct job_queue queue;

void setUp(void);
void tearDown(void);

void run_common_tests(void);

#endif // SUITE_JOB_H
