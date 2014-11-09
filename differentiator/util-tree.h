#pragma once

#include "util.h"
#include "node.h"

/*
 * Main data
 */

extern VariableMap variables;

/*
 * Populates the variable definition with some predefined constants.
 */

void insert_constants();

/*
 * Populates the variable definition list by reading a file.
 */

void read_variables (const char* path);

Node::Base::Ptr simplify_tree (Node::Base::Ptr& tree);
Node::Base::Ptr simplify_tree (Node::Base::Ptr& tree, const std::string& partial_variable);
Node::Base::Ptr differentiate (Node::Base::Ptr& tree, const std::string& partial_variable);
