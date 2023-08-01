#pragma once

#include <stddef.h>
#include <sys/types.h>

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__,   \
              message);                                                        \
      exit(EXIT_FAILURE);                                                      \
    } else {                                                                   \
      fprintf(stdout, "Assertion succeeded at %s:%d: %s\n", __FILE__,          \
              __LINE__, message);                                              \
    }                                                                          \
  } while (0)

void error(const char *format, ...) __attribute__((__noreturn__));
int char_to_int(char digit);
size_t read_into_buf(const char *path, char *buffer, size_t bufferSize);

uint hex_to_int(const char *hex_string, uint len);
uint binary_to_int(const char *binary_string, uint len);

void test_util();
