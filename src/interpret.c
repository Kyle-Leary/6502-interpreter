#include "interpret.h"
#include "assembler.h"
#include "ast.h"
#include "cpu.h"
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

  // Set up colors.
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);

  box(interpreter_win, 0, 0); // Add a border to the window

  wattron(interpreter_win, COLOR_PAIR(1)); // White color
  mvwprintw(interpreter_win, 1, 1, "CPU State:");

  wattron(interpreter_win, COLOR_PAIR(2)); // Green color
  mvwprintw(interpreter_win, 2, 1, "PC: 0x%04X", emu_state->cpu_state->pc);
  mvwprintw(interpreter_win, 3, 1, "A: 0x%02X", emu_state->cpu_state->a);
  mvwprintw(interpreter_win, 4, 1, "X: 0x%02X", emu_state->cpu_state->x);
  mvwprintw(interpreter_win, 5, 1, "Y: 0x%02X", emu_state->cpu_state->y);
  mvwprintw(interpreter_win, 6, 1, "Status: 0b%08B",
            emu_state->cpu_state->status);
  mvwprintw(interpreter_win, 7, 1, "Shutting down: %s",
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
