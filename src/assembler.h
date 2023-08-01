#pragma once

#include "6502_defines.h"
#include "arguments.h"
#include "cpu.h"
#include "lexer.h"

// each opcode id is one byte, with 0, 1 or 2 arg value bytes.
// the addressing mode is encoded in the opcode id.
#define MAX_OPCODE_LEN 3
#define NUM_6502_OPCODES 80

uint make_opcode(Arg argument, Lexeme instruction_id, u8 dest[MAX_OPCODE_LEN]);
