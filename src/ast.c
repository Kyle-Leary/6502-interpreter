#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node ast[AST_LEN] = {
    0}; // store the literal values in a row, zero alloc them so that the types
        // are NT_NULL by default and the comparison works.

Node make_node(NodeType type, NodeIndex left, NodeIndex right, NodeData data) {
  return (Node){type, left, right, data};
}

NodeIndex add_node(Node n) {
  // skip NULL.
  for (int i = 1; i < AST_LEN; i++) {
    Node target_node = ast[i];
    if (target_node.type ==
        NT_NULL) { // if the node's type is zeroed, then it was likely just
                   // alloced or cleared in the overarching array.
      ast[i] = n;
      return i;
    }
  }

  fprintf(stderr, "Too many nodes in the AST. Exiting...\n");
  exit(1);
}

// called by the greater clean() function.
void clean_ast() {
  // TODO: optimize this, it's not necessary to blank out the whole ast space
  // if we keep track of the maximum index used in the AST.
  memset(ast, 0,
         sizeof(Node) * AST_LEN); // blank out the ast array full of nodes.
}
