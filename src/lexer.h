#pragma once

#include "ast.h"
#include "defines.h"
#include <stdbool.h>

// as long as there's not more than 1024 of each type, we're fine and we can
// just check this bit on the instructions.
#define INSTRUCTION_MASK 0b100000000000
#define KEYWORD_MASK 0b1000000000000000

// Lexeme is the type, token is the instance of the specific lexeme produced by
// the lexer.
typedef enum Lexeme {
  LEXEME_NULL = 0, // why not

  // define literals, these will all be nicely handled directly by the lexer and
  // have their values returned in the token value. in the cases of the complex
  // values like strings, a pointer will be in the token value field.
  BINARY_LITERAL, // 0b00101010...
  HEX_LITERAL,    // 0xabcd or $abcd
  INT_LITERAL,    // literal type lexemes.
  FLOAT_LITERAL,  // literal type lexemes.
  CHAR_LITERAL,   // single quote char
  STRING_LITERAL, // double quote string

  ID, // an ID for a variable or function or whatnot, parsed as a
      // backup if the keyword parsing falls through.

  ADD,
  SUB,
  MUL,
  DIV,

  LPAREN,
  RPAREN,
  LCURLY,
  RCURLY,
  SEMI,
  DOT,

  DOLLAR,
  HASHTAG,

  COMMA,
  COLON,

  NEWLINE, // we actually care about NEWLINE now.

  EQUAL,
  DOUBLE_EQUAL,

  SINGLE_QUOTE,
  DOUBLE_QUOTE,

  WHITESPACE, // newline and spaces
  EMPTY,      // this would be EOF, but that appears to be reserved.

  // keyword defs
  KW_INT = KEYWORD_MASK,
  KW_CHAR,
  KW_VOID,
  KW_AUTO,
  KW_BREAK,
  KW_CASE,
  KW_CONST,
  KW_CONTINUE,
  KW_DEFAULT,
  KW_DO,
  KW_DOUBLE,
  KW_ELSE,
  KW_ENUM,
  KW_EXTERN,
  KW_FLOAT,
  KW_FOR,
  KW_GOTO,
  KW_IF,
  KW_LONG,
  KW_REGISTER,
  KW_RETURN,
  KW_SHORT,
  KW_SIGNED,
  KW_SIZEOF,
  KW_STATIC,
  KW_STRUCT,
  KW_SWITCH,
  KW_TYPEDEF,
  KW_UNION,
  KW_UNSIGNED,
  KW_VOLATILE,
  KW_WHILE,

  // put all enum variants in their own ranges, so we can easily tell whether an
  // enum is in a specific range.
  // gcc will compile this as a 32 bit int anyway, so it doesn't matter as long
  // as we're under 2 ** 32.
  ADC = INSTRUCTION_MASK, // Add with carry
  AND,                    // Logical AND
  ASL,                    // Arithmetic Shift Left
  BCC,                    // Branch if carry clear
  BCS,                    // Branch if carry set
  BEQ,                    // Branch if equal (zero set)
  BIT,                    // Bit test
  BMI,                    // Branch if minus (negative set)
  BNE,                    // Branch if not equal (zero clear)
  BPL,                    // Branch if plus (negative clear)
  BRK,                    // Break / interrupt
  BVC,                    // Branch if overflow clear
  BVS,                    // Branch if overflow set
  CLC,                    // Clear carry
  CLD,                    // Clear decimal
  CLI,                    // Clear interrupt disable
  CLV,                    // Clear overflow
  CMP,                    // Compare
  CPX,                    // Compare X register
  CPY,                    // Compare Y register
  DEC,                    // Decrement
  DEX,                    // Decrement X
  DEY,                    // Decrement Y
  EOR,                    // Exclusive OR
  INC,                    // Increment
  INX,                    // Increment X
  INY,                    // Increment Y
  JMP,                    // Jump
  JSR,                    // Jump to subroutine
  LDA,                    // Load accumulator
  LDX,                    // Load X
  LDY,                    // Load Y
  LSR,                    // Logical shift right
  NOP,                    // No operation
  ORA,                    // Logical OR
  PHA,                    // Push accumulator
  PHP,                    // Push processor status
  PLA,                    // Pull accumulator
  PLP,                    // Pull processor status
  ROL,                    // Rotate left
  ROR,                    // Rotate right
  RTI,                    // Return from interrupt
  RTS,                    // Return from subroutine
  SBC,                    // Subtract with carry
  SEC,                    // Set carry
  SED,                    // Set decimal
  SEI,                    // Set interrupt disable
  STA,                    // Store accumulator
  STX,                    // Store X
  STY,                    // Store Y
  TAX,                    // Transfer A to X
  TAY,                    // Transfer A to Y
  TSX,                    // Transfer stack pointer to X
  TXA,                    // Transfer X to A
  TXS,                    // Transfer X to stack pointer
  TYA,                    // Transfer Y to A
} Lexeme;

typedef uint64_t TokenValue;

// what the lexeme actually holds internally.
typedef struct Token {
  Lexeme type; // store the type of the token and some arbitrary data value,
               // either just a u64 or an actual pointer.
  TokenValue value;
} Token;

// ideally, the lexer is the only one with direct access to the text.
// it crunches that into Lexemes -> tokens, and it's all high-level from there
// on out.
typedef struct Lexer {
  char text[INPUT_LEN]; // the current text input it's crunching through.
  int text_len;
  int pos;          // index into the text input of the lexer.
  char curr_char;   // the raw character at the current position.
  Token curr_token; // the current token the parser is on.
} Lexer;

int get_int(Lexer *l);
void next(Lexer *l);
void eat(Lexer *lx, Lexeme l);

BinopType binop_from_lexeme(Lexeme l);
DataType datatype_from_lexeme(Lexeme l);
const char *lexeme_to_string(Lexeme lexeme);

bool is_instruction(Lexeme l);
bool is_keyword(Lexeme l);

void test_lexer();
