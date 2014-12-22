#pragma once

#include <util/util.h>
#include "node.h"

/*
 * Main data
 */

extern Variable::Map variables;

/*
 * Populates the variable definition with some predefined constants.
 */

void insert_constants();

/*
 * Populates the variable definition list by reading a file.
 */

void read_variables (const char* path);

Node::Base::Ptr simplify_tree (Node::Base* tree);
Node::Base::Ptr simplify_tree (Node::Base* tree, const std::string& partial_variable);
Node::Base::Ptr differentiate (Node::Base* tree, const std::string& partial_variable);
