#include "assembler.h"
#include "6502_defines.h"
#include "cpu.h"
#include "util.h"
#include <sys/types.h>

u8 opcode_id_table[NUM_6502_OPCODES][ADDRMODE_COUNT] = {
    //   Imp,  Imm,   Abs,  AbsX, AbsY,   ZP,  ZPX,  ZPY, Rel, Ind, IdxInd,
    //   IndIdx
    {0x00, 0x69, 0x6D, 0x7D, 0x79, 0x65, 0x75, 0x00, 0x00, 0x00, 0x61,
     0x71}, // ADC
    {0x00, 0x29, 0x2D, 0x3D, 0x39, 0x25, 0x35, 0x00, 0x00, 0x00, 0x21,
     0x31}, // AND
    {0x0A, 0x00, 0x0E, 0x1E, 0x00, 0x06, 0x16, 0x00, 0x00, 0x00, 0x00,
     0x00}, // ASL
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00,
     0x00}, // BCC
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x00,
     0x00}, // BCS
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00,
     0x00}, // BEQ
    {0x00, 0x00, 0x2C, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // BIT
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00,
     0x00}, // BMI
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00,
     0x00}, // BNE
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
     0x00}, // BPL
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // BRK
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00,
     0x00}, // BVC
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00,
     0x00}, // BVS
    {0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // CLC
    {0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // CLD
    {0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // CLI
    {0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // CLV
    {0x00, 0xC9, 0xCD, 0xDD, 0xD9, 0xC5, 0xD5, 0x00, 0x00, 0x00, 0xC1,
     0xD1}, // CMP
    {0x00, 0xE0, 0xEC, 0x00, 0x00, 0xE4, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // CPX
    {0x00, 0xC0, 0xCC, 0x00, 0x00, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // CPY
    {0xC6, 0x00, 0xCE, 0xDE, 0x00, 0xC6, 0xD6, 0x00, 0x00, 0x00, 0x00,
     0x00}, // DEC
    {0xCA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // DEX
    {0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // DEY
    {0x00, 0xC9, 0xCD, 0xDD, 0xD9, 0xC5, 0xD5, 0x00, 0x00, 0x00, 0xC1,
     0xD1}, // EOR
    {0xE6, 0x00, 0xEE, 0xFE, 0x00, 0xE6, 0xF6, 0x00, 0x00, 0x00, 0x00,
     0x00}, // INC
    {0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // INX
    {0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // INY
    {0x00, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6C, 0x00,
     0x00}, // JMP
    {0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // JSR
    {0x00, 0xA9, 0xAD, 0xBD, 0xB9, 0xA5, 0xB5, 0x00, 0x00, 0x00, 0xA1,
     0xB1}, // LDA
    {0x00, 0xA2, 0xAE, 0xBE, 0x00, 0xA6, 0xB6, 0x00, 0x00, 0x00, 0x00,
     0x00}, // LDX
    {0x00, 0xA0, 0xAC, 0xBC, 0x00, 0xA4, 0xB4, 0x00, 0x00, 0x00, 0x00,
     0x00}, // LDY
    {0x4A, 0x00, 0x4E, 0x5E, 0x00, 0x46, 0x56, 0x00, 0x00, 0x00, 0x00,
     0x00}, // LSR
    {0xEA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // NOP
    {0x00, 0x09, 0x0D, 0x1D, 0x19, 0x05, 0x15, 0x00, 0x00, 0x00, 0x01,
     0x11}, // ORA
    {0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // PHA
    {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // PHP
    {0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // PLA
    {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // PLP
    {0x2A, 0x00, 0x2E, 0x3E, 0x00, 0x26, 0x36, 0x00, 0x00, 0x00, 0x00,
     0x00}, // ROL
    {0x6A, 0x00, 0x6E, 0x7E, 0x00, 0x66, 0x76, 0x00, 0x00, 0x00, 0x00,
     0x00}, // ROR
    {0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // RTI
    {0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // RTS
    {0x00, 0xE9, 0xED, 0xFD, 0xF9, 0xE5, 0xF5, 0x00, 0x00, 0x00, 0xE1,
     0xF1}, // SBC
    {0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // SEC
    {0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // SED
    {0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // SEI
    {0x00, 0x00, 0x8D, 0x9D, 0x99, 0x85, 0x95, 0x00, 0x00, 0x00, 0x81,
     0x91}, // STA
    {0x00, 0x00, 0x8E, 0x00, 0x00, 0x86, 0x96, 0x00, 0x00, 0x00, 0x00,
     0x00}, // STX
    {0x00, 0x00, 0x8C, 0x00, 0x00, 0x84, 0x94, 0x00, 0x00, 0x00, 0x00,
     0x00}, // STY
    {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // TAX
    {0xA8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // TAY
    {0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // TSX
    {0x8A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // TXA
    {0x9A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00}, // TXS
    {0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00} // TYA
};

// arg contains the addressing type and value, lexeme contains the instruction
// id offset by INSTRUCTION_MASK.
//
// return the size of the opcode that was filled.
uint make_opcode(Arg argument, Lexeme instruction_id, u8 dest[MAX_OPCODE_LEN]) {
  //// PARSE THE OPCODE ID OUT OF THE LUT, THEN THE ARGUMENT BYTES.
  // in theory, the lookup table instructions should be ordered the same way
  // they are in the Lexeme table. (just offset by the instruction mask.)
  int opcode_offset = instruction_id - INSTRUCTION_MASK;
  if (opcode_offset < 0 || opcode_offset > NUM_6502_OPCODES) {
    error("INVALID INSTRUCTION LEXEME PASSED TO make_opcode(). [lexeme: %d]\n",
          instruction_id);
  }

  // find the first byte, just the opcode offset in the table.
  u8 opcode_id = opcode_id_table[opcode_offset][argument.mode];
  dest[0] = opcode_id;

  if (opcode_id == 0x00) {
    error("INVALID OPCODE ID SELECTED FROM THE TABLE!\nYou tried to "
          "select: %d with addrmode %d. Exiting...\n",
          instruction_id, argument.mode);
  }

  // TODO: oh god will this fuck up on systems with different endianness
  // oh god do i actually need to do an endianness check?
  // i don't think that's really that bad, actually. just check endianness, and
  // make sure that this value is in little-endian format. then, everything
  // should work fine.
  u8 *value_list = (u8 *)&argument.value;

  // ok, so we're just going to assume this code runs on a little endian system
  // for now. this is actually really nice, since it means we're using the same
  // endianness as the 6502.

  // now, handle the argument differently depending on the addressing mode.
  switch (argument.mode) {
  case Implicit: {
    // do nothing, the opcode is empty.
    return 1; // only one byte was filled, none of the arg bytes were used.
  } break;

  case Immediate: {
    dest[1] = value_list[0]; // copy the lsb of the value over to the only
                             // immediate byte of the opcode.
    return 2;
  } break;

  case Abs: {
    dest[1] = value_list[0];
    dest[2] = value_list[1];
    return 3;
  } break;

  case AbsX: {
    dest[1] = value_list[0];
    dest[2] = value_list[1];
    return 3;
  } break;

  case AbsY: {
    dest[1] = value_list[0];
    dest[2] = value_list[1];
    return 3;
  } break;

  case ZP: {
    dest[1] = value_list[0];
    return 2;
  } break;

  case ZPX: {
    dest[1] = value_list[0];
    return 2;
  } break;

  case ZPY: {
    dest[1] = value_list[0];
    return 2;
  } break;

  case Relative: { // relative to the current position.
    dest[1] = value_list[0];
    return 2;
  } break;

  case Indirect: {
    dest[1] = value_list[0];
    return 3;
  } break;

  case IndexedIndirect: {
    dest[1] = value_list[0];
    return 2;
  } break;

  case IndirectIndexed: {
    dest[1] = value_list[0];
    return 2;
  } break;

  default: {
  } break;
  }
}
