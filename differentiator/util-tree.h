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

Node::Base::Ptr simplify_tree (Node::Base* tree);
Node::Base::Ptr simplify_tree (Node::Base* tree, const std::string& partial_variable);
Node::Base::Ptr differentiate (Node::Base* tree, const std::string& partial_variable, unsigned order = 1);
