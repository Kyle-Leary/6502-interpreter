#include "interpret.h"
#include "assembler.h"
#include "ast.h"
#include "cpu.h"
#include "cpu_mapper.h"
#include "defines.h"
#include "lexer.h"
#include "parse.h"
#include "symtab.h"
#include "visit.h"
#include <ncurses.h>
#include <stdio.h>
#include <sys/types.h>

#include "emu.h"

static EmuState *emu_state;

// this interpreter source file also holds all the private data for the EmuState
// and whatever. this is a global state machine, and interpret() is the function
// that controls and ticks the interpreter state.

static void get_instruction_bytes(u8 *dest, Lexeme instruction) {}

// interpreter that uses the CPU emulation to execute certain commands and print
// out the state.
static NodeData visit_interpret(NodeIndex n_idx) {
  Node n = ast[n_idx];

  printf("  ");

  switch (n.type) {

  case NT_NULL: {
    printf("(INVALID NODE)\n");
  } break;
  case NT_EMPTY: {
    printf("(EMPTY STATEMENT)\n");
  } break;

  // value nodes can just store their values directly in the data field.
  case NT_NUMBER: {
    return n.data;
  } break;
  case NT_CHAR: {
    return n.data;
  } break;

  case NT_BINOP: {
    // the same as normal visitation. just do the operation and return it.
    return visit(n_idx);
  } break;

  case NT_ID: {
    printf("(ID: %s)\n", (char *)n.data.as_ptr);
  } break;

  case NT_STATEMENT_LIST: {
    // just print instead of normally visiting this time.
    visit_interpret(n.left);
    if (n.right != NULL_INDEX) {
      visit_interpret(n.right);
    }
  } break;

  case NT_INSTRUCTION: {
    Lexeme instruction =
        (Lexeme)n.data.as_raw_data; // pass in the instruction variant through
                                    // the data field
    Arg arg = visit_interpret(n.left).as_arg;
    printf("Executing instruction with addressing mode %d and value %d, with "
           "the instruction variant %d\n",
           arg.mode, arg.value, instruction);

    u8 opcode[MAX_OPCODE_LEN] = {0};
    uint opcode_len = make_opcode(arg, instruction, opcode);

    printf("opcode len: %d; opcode: [%02x %02x %02x]\n", opcode_len, opcode[0],
           opcode[1], opcode[2]);

    execute_instruction(emu_state, opcode, opcode_len);
  } break;

  case NT_ARGUMENT: {
    // just return the argument for the instruction execution rule to use.
    return n.data;
  } break;

  case NT_LABEL: {
    printf("(LABEL: %s)\n", (char *)visit(n.left).as_ptr);
  } break;

  default: {
    printf("(UNKNOWN NODE)\n");
  } break;
  }

  // return 0 in case we didn't return on one of the arms.
  return NO_NODE_DATA;
}

// Function to refresh the debug state on screen.
static void display_interpreter_state(WINDOW *interpreter_win) {
  wclear(interpreter_win);

  int i = 1;

  box(interpreter_win, 0, 0); // Add a border to the window

  wattron(interpreter_win, COLOR_PAIR(1)); // White color
  mvwprintw(interpreter_win, i, 1, "CPU State:");
  i++;

  wattron(interpreter_win, COLOR_PAIR(2)); // Green color
  mvwprintw(interpreter_win, i, 1, "PC: 0x%04X", emu_state->cpu_state->pc);
  i++;
  mvwprintw(interpreter_win, i, 1, "A: 0x%02X", emu_state->cpu_state->a);
  i++;
  mvwprintw(interpreter_win, i, 1, "X: 0x%02X", emu_state->cpu_state->x);
  i++;
  mvwprintw(interpreter_win, i, 1, "Y: 0x%02X", emu_state->cpu_state->y);
  i++;

  mvwprintw(interpreter_win, i, 1, "  MEMORY: ");
  i++;

  // how many slots of wram should be printed?
#define NUM_MEMORY_PRINTS 10
  // how much the wram printout is offset backwards.
#define INDEXING_OFFSET 5

  // print out a memory preview from wram.
  int limit = i + NUM_MEMORY_PRINTS;
  u8 pc = emu_state->cpu_state->pc;
  int mem_index = 0;
  for (; i < limit; i++) {
    int address_to_print = mem_index + pc - INDEXING_OFFSET;
    // extend into the mirror, i guess?
    if (address_to_print < 0x00 || address_to_print > WRAM_MIRROR_SIZE) {
      mvwprintw(interpreter_win, i, 1, "    - OUT OF BOUNDS -");
      // continue while still incrementing the mem index ptr
      goto mem_print_loop_end;
    }

    // the %s is the pointer at the program counter. if we're printing the
    // program counter wram address, then render the arrow pointer.
    mvwprintw(interpreter_win, i, 1, "    [0x%02X]: 0x%02X %s",
              address_to_print, emu_state->map->wram[address_to_print],
              (mem_index - INDEXING_OFFSET == 0) ? "<-" : "");
  mem_print_loop_end:
    mem_index++;
  }

#undef NUM_MEMORY_PRINTS

  mvwprintw(interpreter_win, i, 1, "Status: 0b%08B",
            emu_state->cpu_state->status);
  i++;
  mvwprintw(interpreter_win, i, 1, "Shutting down: %s",
            emu_state->cpu_state->shutting_down ? "Yes" : "No");

  wattroff(interpreter_win, COLOR_PAIR(2)); // Turn off green color
  wrefresh(interpreter_win);
}

void init_interpreter() { emu_state = emu_init(); }

void interpret(char *input_buffer, WINDOW *interpreter_window) {
  NodeIndex root_node = parse(input_buffer);
  printf("Root node of the returned AST: %d\n", root_node);
  printf("data from the interpret visitation of the AST: %lu\n",
         visit(root_node).as_raw_data);
  print_symtab();
  printf("\n\nPrinting AST...\n");
  visit_print(root_node);

  printf("\n\nInterpreting AST...\n");
  visit_interpret(root_node);

  display_interpreter_state(interpreter_window);
}

void kill_interpreter() { emu_clean(emu_state); }
