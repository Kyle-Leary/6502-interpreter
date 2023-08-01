#pragma once

#include "arguments.h"
#include "defines.h"
#include <stdint.h>

typedef u16 NodeIndex; // let the indexing into the node array be 16 bits wide.

// make this typepunning bullshit just a little safer.
// explicitly tell the compiler we're doing some crazy shit with this data
// field, or else it'll freak out when we try to punn a struct to an
// arithmetic/pointer type.
typedef union NodeData {
  uint64_t as_raw_data;
  void *as_ptr;
  Arg as_arg;
} NodeData;

// pass blank zeroed out nodedata to a node data slot. make it explicit that
// we're passing blank stuff, it's better than just putting one manually and
// letting the programmer figure it out.
#define NO_NODE_DATA                                                           \
  (NodeData) { .as_raw_data = 0 }

typedef enum NodeType {
  NT_NULL = 0, // always the zero variant, a zero-bytes node is invalid and
               // skipped in insertion functions.

  // literals parse into value types. each addressing mode is its own literal
  // type?
  NT_NUMBER, // basic value type AST nodes.
  NT_CHAR,

  NT_ID,

  NT_BINOP,

  NT_EMPTY, // epsilon empty statement

  NT_INSTRUCTION,
  NT_LABEL, // just contains a NT_ID as a child for now.

  // one argument in an instruction or pragma.
  NT_ARGUMENT,

  NT_STATEMENT_LIST,

  NT_BLOCK, // this is more than just another statement list. we need to keep
            // information about blocks around in the ast for scoping reasons.

  NT_COUNT
} NodeType;

typedef enum BinopType {
  BO_NULL = 0,
  BO_ADD,
  BO_SUB,
  BO_MUL,
  BO_DIV,
  BO_COUNT,
} BinopType;

typedef enum DataType {
  DT_INVALID = 0, // invalid datatype, for recognizing non-taken slots in the
                  // symbol table.

  DT_VOID,
  DT_INT,
  DT_CHAR,
  DT_COUNT,
} DataType;

// 128 bit large structure, 64-bit align it.
typedef struct Node {
  NodeType type;
  NodeIndex left; // store indices into the ast as child nodes.
  NodeIndex right;
  NodeData data; // an arbitrary 64-bit piece of data, this can also be used as
                 // a raw integer, depending on the node type.
} Node;

// the ast array itself and ast management functionality, along with node type
// defines.
extern Node ast[AST_LEN]; // store the literal values in a row.

// then, helpers for managing the ast itself.
Node make_node(NodeType type, NodeIndex left, NodeIndex right, NodeData data);
NodeIndex add_node(Node n);
void clean_ast();
