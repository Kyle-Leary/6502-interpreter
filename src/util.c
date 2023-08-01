#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "util.h"

void error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}

int char_to_int(char digit) {
  printf("Converting %c to an int.\n", digit);
  if (digit >= '0' && digit <= '9') {
    return digit - '0';
  }
  error("Digit conversion: failed to convert char to digit.\n");
  return -1;
}

size_t read_into_buf(const char *path, char *buffer, size_t bufferSize) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    char error_buf[256];
    sprintf(error_buf, "Error opening file [%s]", path);
    perror(error_buf);
    exit(1);
    return 0;
  }

  size_t bytesRead = fread(buffer, sizeof(char), bufferSize, file);
  if (ferror(file)) {
    char error_buf[256];
    sprintf(error_buf, "Error reading from file [%s]", path);
    perror(error_buf);
    exit(1);
    bytesRead = 0;
  }

  fclose(file);
  return bytesRead;
}

uint powers_of_two[32] = {
    1,         2,         4,          8,         16,       32,       64,
    128,       256,       512,        1024,      2048,     4096,     8192,
    16384,     32768,     65536,      131072,    262144,   524288,   1048576,
    2097152,   4194304,   8388608,    16777216,  33554432, 67108864, 134217728,
    268435456, 536870912, 1073741824, 2147483648};
uint powers_of_sixteen[8] = {1,     16,      256,      4096,
                             65536, 1048576, 16777216, 268435456};

uint hex_to_int(const char *hex_string, uint len) {
  uint result = 0;
  uint ch_value;
  char ch;

  printf("hex string conversion: %s\n", hex_string);
  for (int i = 0; i < len; i++) {
    ch = hex_string[len - i -
                    1]; // We start from the least significant character.

    if (ch >= '0' && ch <= '9') { // Then it's the hex numbers 0 - 9
      ch_value = ch - '0';
    } else if (ch >= 'a' && ch <= 'f') { // Then it's a - f
      ch_value = ch - 'a' + 10;
    }

    result += powers_of_sixteen[i] * ch_value;
  }

  return result;
}

uint binary_to_int(const char *binary_string, uint len) {
  uint result = 0;
  uint ch_value;
  char ch;

  for (int i = 0; i < len; i++) {
    ch = binary_string[len - i -
                       1]; // We start from the least significant character.

    if (ch == '0' || ch == '1') { // Either a 0 or 1
      ch_value = ch - '0';
    }

    result += powers_of_two[i] * ch_value;
  }

  return result;
}

void test_util() {
  printf("\n\nTESTING UTIL FUNCTIONS\n\n\n");

  // Test cases for hex_to_int
  assert(hex_to_int("a", 1) == 10);
  assert(hex_to_int("f", 1) == 15);
  assert(hex_to_int("10", 2) == 16);
  assert(hex_to_int("1f", 2) == 31);
  assert(hex_to_int("ff", 2) == 255);

  // Test cases for binary_to_int
  assert(binary_to_int("1", 1) == 1);
  assert(binary_to_int("10", 2) == 2);
  assert(binary_to_int("11", 2) == 3);
  assert(binary_to_int("1001", 4) == 9);
  assert(binary_to_int("1111", 4) == 15);
  assert(binary_to_int("10000", 5) == 16);

  printf("\n\nDONE TESTING UTIL FUNCTIONS, SUCCESS!\n\n\n");
}
