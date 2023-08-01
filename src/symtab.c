#include "symtab.h"
#include "ast.h"
#include "defines.h"

#include <stdio.h>
#include <string.h>

Symbol symtab[SYMTAB_LEN] = {0}; // zero alloc for similar reasons to the ast.

// use a hashmap to store based on string keys and quickly operate on the
// symtab. use this hashing function for map access.
static unsigned long djb2(char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

// i think we still need to keep around the char *name, maybe even the length in
// the Symbol itself.
Symbol make_symbol(DataType type, char *name, SymbolValue value) {
  return (Symbol){type, name, value};
}

// make sure that the symbol names coming from the lexer are null-terminated,
// else the djb2 hash will probably segfault.
void insert_symbol(Symbol s) {
  printf("Inserting the symbol named %s to the symtab with value %lu.\n",
         s.name, s.value);
  symtab[djb2(s.name) % SYMTAB_LEN] = s;
}

// called by the greater clean() function.
void clean_symtab() { memset(symtab, 0, sizeof(Symbol) * SYMTAB_LEN); }

void print_symtab() {
  printf("Printing symbol table...\n");

  // iterate over the entire symbol table
  for (int i = 0; i < SYMTAB_LEN; i++) {
    Symbol s = symtab[i];

    // check if the name is NULL, which would mean the symbol is empty
    if (s.name != NULL) {
      printf("Index: %d\n", i);
      printf("Name: %s\n", s.name);
      printf("Type: %d\n", s.type);

      // here we assume that SymbolValue is a type that can be printed with %d
      printf("Value: %lu\n", s.value);
      printf("-------------------------\n");
    }
  }

  printf("End of symbol table.\n");
}
