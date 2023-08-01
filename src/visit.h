#pragma once

#include "ast.h"
#include "defines.h"

// define multiple different visitation functions here, interpretation,
// printing, semantics, etc.
NodeData visit(NodeIndex n_idx);
void visit_print(NodeIndex n_idx);
