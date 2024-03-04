#ifndef SCHW_COMMON_H
#define SCHW_COMMON_H

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#endif // SCHW_COMMON_H
