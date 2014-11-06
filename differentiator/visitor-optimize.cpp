#include "visitor-optimize.h"

#include <sstream>

namespace Visitor {

boost::any Optimize::visit (Node::Value& node)
{
	/* identity */
	return static_cast<Node::Base*> (node.clone().release());
}

boost::any Optimize::visit (Node::Variable& node)
{
	/* identity */
	return static_cast<Node::Base*> (node.clone().release());
}

boost::any Optimize::visit (Node::Function& node)
{
	std::ostringstream reason;
	reason << "Simplify error: unknown function: '" << node.name() << "'";
	throw std::runtime_error (reason.str());
}

boost::any Optimize::visit (Node::Power& node)
{
	/* recurse to children */
	Node::Power::Ptr result (new Node::Power);

	for (Node::Base::Ptr& child: node.children()) {
		result->add_child (child->accept_ptr (*this));
	}

	return static_cast<Node::Base*> (result.release());
}

boost::any Optimize::visit (Node::AdditionSubtraction& node)
{
	/* recurse to children */
	Node::AdditionSubtraction::Ptr result (new Node::AdditionSubtraction);

	auto child = node.children().begin();
	auto negation = node.negation().begin();

	while (child != node.children().end()) {
		result->add_child ((*child++)->accept_ptr (*this), *negation++);
	}

	return static_cast<Node::Base*> (result.release());
}

Node::Base::Ptr Optimize::multiply_nodes (std::vector<Node::Base::Ptr>& vec)
{
	if (vec.size() == 1) {
		return std::move (vec.at (0));
	} else {
		Node::MultiplicationDivision::Ptr result (new Node::MultiplicationDivision);
		for (Node::Base::Ptr& node: vec) {
			result->add_child (std::move (node), false);
		}
		return std::move (result);
	}
}

boost::any Optimize::visit (Node::MultiplicationDivision& node)
{
	Node::MultiplicationDivision::Ptr result;
	std::vector<Node::Base::Ptr> multipliers, divisors;

	auto child = node.children().begin();
	auto reciprocation = node.reciprocation().begin();

	while (child != node.children().end()) {
		Node::Base::Ptr optimized ((*child++)->accept_ptr (*this));

		if (*reciprocation++) {
			divisors.push_back (std::move (optimized));
		} else {
			multipliers.push_back (std::move (optimized));
		}
	}

	if (divisors.empty()) {
		/* at least one multiplier is present */
		return static_cast<Node::Base*> (multiply_nodes (multipliers).release());
	} else {
		result = Node::MultiplicationDivision::Ptr (new Node::MultiplicationDivision);
		if (!multipliers.empty()) {
			result->add_child (multiply_nodes (multipliers), false);
		}
		result->add_child (multiply_nodes (divisors), true);

		return static_cast<Node::Base*> (result.release());
	}
}

} // namespace Visitor
