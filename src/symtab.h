#pragma once

#include "ast.h"
#include "defines.h"

#include <stdint.h>

typedef uint64_t SymbolValue;

// all the data associated with the symbol, the symbol goes directly into the
// global array, which is considered our "symbol table".
typedef struct Symbol {
  DataType type;
  char *name; // the name is already malloced from the ID node, so this doesn't
              // really matter (other than memory fragmentation!!!)
  SymbolValue value;
} Symbol;

// unravel the recursive list from the argument_list node into a char* array.
typedef struct FunctionData { // this structure is to be pointed to in the
                              // SymbolValue field of the Symbol in the table.
  // don't need to store the function_name, that's already the Symbol name.
  char **arg_names;
} FunctionData;

typedef struct FunctionCommonData {
  char *name;
  char **arg_names;
} FunctionCommonData;

extern Symbol symtab[SYMTAB_LEN];

Symbol make_symbol(DataType type, char *name, SymbolValue value);
void insert_symbol(Symbol s);
void clean_symtab();
void print_symtab();
