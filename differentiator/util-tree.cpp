#include "util-tree.h"
#include "visitor.h"
#include "visitor-simplify.h"
#include "visitor-optimize.h"
#include "visitor-differentiate.h"

/*
 * Main data
 */


// all variables being considered
Variable::Map variables;

/*
 * Populates the variable definition with some predefined constants.
 */

void insert_constants()
{
	variables.insert (Variable::make ("pi", M_PI, 0));
	variables.insert (Variable::make ("g", 9.81, 0));
}

/*
 * Populates the variable definition list by reading a file.
 */

void read_variables (const char* path)
{
	std::ifstream variable_file;
	open (variable_file, path);

	while (!(variable_file >> std::ws, variable_file.eof())) {
		parse_variable (variables, variable_file);
	}
}

Node::Base::Ptr simplify_tree (Node::Base* tree)
{
	Visitor::Optimize optimizer;
	Visitor::Simplify simplifier;

	return tree->accept_ptr (simplifier)
	           ->accept_ptr (optimizer);
}

Visitor::Simplify maybe_partial_simplifier (const std::string& partial_variable)
{
	if (getenv ("SIMPLIFY")) {
		return Visitor::Simplify (partial_variable);
	} else {
		return Visitor::Simplify();
	}
}

Node::Base::Ptr simplify_tree (Node::Base* tree, const std::string& partial_variable)
{
	Visitor::Optimize optimizer;
	Visitor::Simplify simplifier = maybe_partial_simplifier (partial_variable);

	return tree->accept_ptr (simplifier)
	           ->accept_ptr (optimizer);
}

Node::Base::Ptr differentiate (Node::Base* tree, const std::string& partial_variable)
{
	Visitor::Simplify simplifier = maybe_partial_simplifier (partial_variable);
	Visitor::Optimize optimizer;
	Visitor::Differentiate differentiator (partial_variable);

	return tree->accept_ptr (simplifier)
	           ->accept_ptr (optimizer)
	           ->accept_ptr (differentiator)
	           ->accept_ptr (simplifier)
	           ->accept_ptr (optimizer);
}
