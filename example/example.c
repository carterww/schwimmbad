#include "schwimmbad.h"
#include <stdio.h>
#include <unistd.h>

#define NUM_JOBS 500
#define NUM_THREADS 10
#define QUEUE_LEN 200
#define POLICY FIFO

struct schw_pool pool = { 0 };

static void *hello_world(void *arg) {
  long i = (long)arg;
  // NOT THREAD SAFE!!!! JUST EXAMPLE
  printf("Hello, world, from job %ld\n", i);
  return "I am done!";
}

static void handle_complete_job(jid id, void *arg, void *j_res) {
  struct schw_pool *pool = (struct schw_pool *)arg;
  char *job_msg = (char *)j_res;
  // NOT THREAD SAFE!!!! JUST EXAMPLE
  printf("CB -> [%ld]: %s\n", id, job_msg);
}

int main(void) {
  int init_res = schw_init(&pool, NUM_THREADS, QUEUE_LEN, POLICY);
  schw_set_callback(&pool, handle_complete_job, &pool);

  for (int i = 0; i < NUM_JOBS; i++) {
    struct schw_job job = {
      .job_arg = (void *)(long)i,
      .job_func = hello_world,
      .priority = NUM_JOBS - i
    };
    schw_push(&pool, &job, SCHW_PUSH_BLOCK);
  }

  schw_wait_all(&pool);

  int free_res = schw_free(&pool);
  return 0;
}
