#include "visit.h"

#include "ast.h"
#include "cpu.h"
#include "defines.h"
#include "lexer.h"
#include "mempool.h"
#include "symtab.h"
#include <stdio.h>

// it may be better to cut the recursion out of this function and get rid of the
// function call overhead.
NodeData visit(NodeIndex n_idx) {
  Node n = ast[n_idx];

  switch (n.type) {

  case NT_NULL:
    return NO_NODE_DATA;
    break;

  case NT_NUMBER: {
    return n.data;
  } break;

  case NT_CHAR: {
    return n.data;
  } break;

  case NT_BINOP: {
    switch (n.data.as_raw_data) { // switch over the binop type in the nodedata

    case BO_ADD:
      return (NodeData){.as_raw_data = visit(n.left).as_raw_data +
                                       visit(n.right).as_raw_data};
      break;
    case BO_SUB:
      return (NodeData){.as_raw_data = visit(n.left).as_raw_data -
                                       visit(n.right).as_raw_data};
      break;
    case BO_MUL:
      return (NodeData){.as_raw_data = visit(n.left).as_raw_data *
                                       visit(n.right).as_raw_data};
      break;
    case BO_DIV:
      return (NodeData){.as_raw_data = visit(n.left).as_raw_data /
                                       visit(n.right).as_raw_data};
      break;

    default:
      return NO_NODE_DATA;
      break;
    }
  } break;

  case NT_EMPTY: {
    return NO_NODE_DATA;
  } break;

  case NT_ID: {
    // return the ptr to the string that NT_ID nodes SHOULD contain.
    return n.data;
  } break;

  // for the st_list, just do all the things in the statement list node slots.
  case NT_STATEMENT_LIST: {
    // always a statement in the left slot, even if it's empty.
    visit(n.left);
    if (n.right != NULL_INDEX) {
      visit(n.right);
    }
    // just return nothing either way.
    return NO_NODE_DATA;
  } break;

  case NT_BLOCK: {
    // block only contains one st list.
    visit(n.left);
    return NO_NODE_DATA;
  } break;

  default:
    return NO_NODE_DATA;
    break;
  }
}

void visit_print(NodeIndex n_idx) {
  Node n = ast[n_idx];

  printf("  ");

  switch (n.type) {

  case NT_NULL: {
    printf("(INVALID NODE)\n");
  } break;

  // number literal
  case NT_NUMBER: {
    printf("(NUM: %lu)\n", n.data.as_raw_data);
  } break;

  // char literal
  case NT_CHAR: {
    printf("(CHAR: %lu)\n", n.data.as_raw_data);
  } break;

  case NT_BINOP: {
    printf("(BINOP: ");
    switch (n.data.as_raw_data) {
    case BO_ADD:
      printf("+)\n");
      break;
    case BO_SUB:
      printf("-)\n");
      break;
    case BO_MUL:
      printf("*)\n");
      break;
    case BO_DIV:
      printf("/)\n");
      break;
    default:
      printf("unknown)\n");
      break;
    }
  } break;

  case NT_EMPTY: {
    printf("(EMPTY STATEMENT)\n");
  } break;

  case NT_ID: {
    printf("(ID: %s)\n", (char *)n.data.as_ptr);
  } break;

  case NT_STATEMENT_LIST: {
    // just print instead of normally visiting this time.
    visit_print(n.left);
    if (n.right != NULL_INDEX) {
      visit_print(n.right);
    }
  } break;

  case NT_INSTRUCTION: {
    printf("(INSTRUCTION: %d)\n", (Lexeme)n.data.as_raw_data);
    if (n.left != NULL_INDEX)
      visit_print(n.left);
  } break;

  case NT_ARGUMENT: {
    Arg a = n.data.as_arg;
    printf("(ARGUMENT: [AddrMode %d] [Value %d])\n", (AddrMode)a.mode, a.value);
  } break;

  case NT_LABEL: {
    printf("(LABEL: %s)\n", (char *)visit(n.left).as_ptr);
  } break;

  case NT_BLOCK: {
    // block only contains one st list.
    printf("(BLOCK)\n");
    visit_print(n.left);
  } break;

  default:
    printf("(UNKNOWN NODE)\n");
    break;
  }
}
