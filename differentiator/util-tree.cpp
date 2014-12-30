#include "util-tree.h"
#include "visitor.h"
#include "visitor-simplify.h"
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
	variables.insert (Variable::make<data_t> ("pi", M_PI, 0, true));
	variables.insert (Variable::make<data_t> ("g", 9.81, 0, true));
}

Node::Base::Ptr simplify_tree (Node::Base* tree)
{
	Visitor::Simplify simplifier;

	return tree->accept_ptr (simplifier);
}

Node::Base::Ptr simplify_tree (Node::Base* tree, const std::string& partial_variable)
{
	Visitor::Simplify simplifier (partial_variable);

	return tree->accept_ptr (simplifier);
}

Node::Base::Ptr differentiate (Node::Base* tree, const std::string& partial_variable, unsigned int order /* = 1 */)
{
	Visitor::Simplify simplifier;
	Visitor::Differentiate differentiator (partial_variable);

	Node::Base::Ptr ret = tree->accept_ptr (differentiator)
	                          ->accept_ptr (simplifier);

	for (unsigned i = 1; i < order; ++i) {
		ret = ret->accept_ptr (differentiator)
		         ->accept_ptr (simplifier);
	}

	return ret;
}
