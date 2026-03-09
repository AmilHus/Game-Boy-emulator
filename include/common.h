#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define BIT(a, n) ((a & (1 << n)) ? 1 : 0)

#define BIT_SET(a, n, on)                                                      \
  {                                                                            \
    if (on)                                                                    \
      a |= (1 << n);                                                           \
    else                                                                       \
      a &= ~(1 << n);                                                          \
  }

#define BETWEEN(a, b, c) ((a >= b) && (a <= c))

void delay(u32 ms);
u32 get_ticks();

#define NO_IMPL                                                                \
  {                                                                            \
    fprintf(stderr, "NOT IMPLEMENTED\n");                                      \
    exit(-5);                                                                  \
  }
