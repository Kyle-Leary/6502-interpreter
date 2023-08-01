#pragma once

#include <stdint.h>

// stdint typenames suck
// make them better
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// consider the first index of the array NULL, therefore off-limits.
#define NULL_INDEX 0

#define INPUT_LEN (1024 * 100)
#define MAX_STR_LITERAL_SIZE 256
#define U16_MAX 65535
#define AST_LEN ((U16_MAX)-1)
#define SYMTAB_LEN 256

// how large can the keyword (and identifier) strings be? used for allocing the
// buffer in the Lexer next() function. "register", "continue", "unsigned" and
// "volatile" are the longest keywords in C.
#define MAX_KW_LEN 64
