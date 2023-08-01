#pragma once

// an argument with a specific addressing type passed to the instruction.
#include "cpu.h"
#include <stdint.h>

// we can parse these directly into the AST and store these structures directly
// in the ast without pointers, since it's a 64-bit struct exactly.
typedef struct Arg { // 64 bit struct.
  // use the AddrMode enum from the 6502 library.
  AddrMode mode; // 32 bit enum (most likely)

  // pad the struct out to 64 bits with a 32_t as the data field.
  uint32_t value; // the argument data fields are going to be a max of 2 bytes,
                  // (either 0, 1 or 2) so this is more than enough and we can
                  // just cast it down if necessary.
} Arg;
