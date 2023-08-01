#include "ast.h"
#include "cglm/types.h"
#include "defines.h"
#include "interpret.h"
#include "lexer.h"
#include "mempool.h"
#include "parse.h"
#include "path.h"
#include "symtab.h"
#include "util.h"
#include "visit.h"

#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

// clean up the ast state for the next run through.
void clean() {
  clean_ast();
  clean_symtab();
}

int main(int argc, char *argv[]) {
  // common initialization, we'll always use the mempool.
  mempool_init();

#ifdef TESTING
  test_lexer();
  test_parse();
  test_util();
  return 0;
#endif /* ifdef TESTING */

  init_interpreter();

  // Initialize ncurses
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE); // Enable special keys

  // Set up colors.
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_BLACK, COLOR_WHITE);

  // Create windows for input and debug state
  WINDOW *input_win = newwin(3, COLS, LINES - 6, 0);
  WINDOW *interpreter_win = newwin(LINES - 9, COLS, 3, 0);
  WINDOW *header_win = newwin(3, COLS, 0, 0);

  wbkgd(header_win, COLOR_PAIR(3));
  box(header_win, 0, 0);
  mvwprintw(header_win, 1, 1, "6502 INTERPRETER - type \"exit\" to exit.");
  wrefresh(header_win);

  mvwprintw(input_win, 1, 1, "Enter input: ");
  wrefresh(input_win);

  char input_buffer[INPUT_LEN] = {0};

  // Main loop
  while (1) {
    echo();
    wgetnstr(input_win, input_buffer, INPUT_LEN);

    // Check for exit command
    if (strcmp(input_buffer, "exit") == 0) {
      break;
    }

    interpret(input_buffer, interpreter_win);

    clean();

    // Clear and prepare for next input
    wclear(input_win);
    mvwprintw(input_win, 1, 1, "Enter input: ");
    wrefresh(input_win);
  }

  kill_interpreter();

  // Clean up ncurses and exit
  endwin();

  return 0;
}
