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
	variables.insert (Variable::make ("pi", M_PI, 0, true));
	variables.insert (Variable::make ("g", 9.81, 0, true));
}

Node::Base::Ptr simplify_tree (Node::Base* tree)
{
	Visitor::Optimize optimizer;
	Visitor::Simplify simplifier;

	return tree->accept_ptr (simplifier)
	           ->accept_ptr (optimizer);
}

Node::Base::Ptr simplify_tree (Node::Base* tree, const std::string& partial_variable)
{
	Visitor::Optimize optimizer;
	Visitor::Simplify simplifier (partial_variable);

	return tree->accept_ptr (simplifier)
	           ->accept_ptr (optimizer);
}

Node::Base::Ptr differentiate (Node::Base* tree, const std::string& partial_variable, unsigned int order /* = 1 */)
{
	Visitor::Simplify simplifier;
	Visitor::Optimize optimizer;
	Visitor::Differentiate differentiator (partial_variable);

	Node::Base::Ptr ret = tree->accept_ptr (simplifier)
	                          ->accept_ptr (optimizer);

	for (unsigned i = 0; i < order; ++i) {
		ret = ret->accept_ptr (differentiator);
	}

	return ret->accept_ptr (simplifier)
	          ->accept_ptr (optimizer);
}
