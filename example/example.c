#include "schwimmbad.h"

struct schw_pool pool = { 0 };

int main(void) {
  int init_res = schw_init(&pool, 2, 5);

  int free_res = schw_free(&pool);
  return 0;
}
