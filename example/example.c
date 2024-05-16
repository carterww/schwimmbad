#include "schwimmbad.h"
#include <stdio.h>
#include <unistd.h>

struct schw_pool pool = { 0 };

static void *hello_world(void *arg) {
  long i = (long)arg;
  printf("Hello, world, from job %ld\n", i);
  return NULL;
}

int main(void) {
  int init_res = schw_init(&pool, 5, 10, PRIORITY);

  for (int i = 0; i < 500; i++) {
    struct schw_job job = {
      .job_arg = (void *)(long)i,
      .job_func = hello_world,
      .priority = 500 - i
    };
    schw_push(&pool, &job, SCHW_PUSH_BLOCK);
  }

  sleep(15);

  int free_res = schw_free(&pool);
  return 0;
}
