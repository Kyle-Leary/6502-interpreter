#pragma once

#include "defines.h"
#include <curses.h>
#include <ncurses.h>

void init_interpreter();
// not only parse the input and do the command, but update the passed window
// with the cpu state and any other nice interpreter stuff.
void interpret(char *input_buffer, WINDOW *interpreter_window);
void kill_interpreter();
