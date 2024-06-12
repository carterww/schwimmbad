#ifndef SCHW_COMMON_H
#define SCHW_COMMON_H

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define MUTEX_LOCK_AND_OP(lock, code_block) \
  pthread_mutex_lock(&lock);                \
  code_block;                               \
  pthread_mutex_unlock(&lock);

#define SPIN_LOCK_AND_OP(lock, code_block) \
  pthread_spin_lock(&lock);                \
  code_block;                             \
  pthread_spin_unlock(&lock);
#endif // SCHW_COMMON_H
