#ifndef __TEST_H__
#define __TEST_H__

#include <cstdio>

#define OUTPUT(format, ...) printf(format, ##__VA_ARGS__)
#define ASSERT(condition, format, ...)                                         \
  if (!(condition)) {                                                          \
    OUTPUT("\033[;31mAssertion ' %s ' Failed!\n%s:%d: " format "\n\033[0m",    \
           #condition, __FILE__, __LINE__, ##__VA_ARGS__);                     \
    exit(1);                                                                   \
  }

#define EXPECT(condition, format, ...)                                         \
  if (!(condition)) {                                                          \
    OUTPUT("\033[;33mExpect ' %s ' \n%s:%d: " format "\n\033[0m", #condition,  \
           __FILE__, __LINE__, ##__VA_ARGS__);                                 \
  }

#endif