#include "visitor-calculate.h"

namespace Visitor {

boost::any Calculate::visit (Node::Value& node)
{
	return node.value();
}

boost::any Calculate::visit (Node::Variable& node)
{
	return node.value();
}

boost::any Calculate::visit (Node::Function& node)
{
	std::ostringstream reason;
	reason << "Calculate error: unknown function: '" << node.name() << "'";
	throw std::runtime_error (reason.str());
}

boost::any Calculate::visit (Node::Power& node)
{
	if (node.children().size() != 2) {
		std::ostringstream reason;
		reason << "Calculate error: power node has " << node.children().size() << " children";
		throw std::logic_error (reason.str());
	}

	auto child = node.children().begin();

	data_t base = (*child++)->accept_value (*this),
	       exponent = (*child++)->accept_value (*this);

	return powl (base, exponent);
}

boost::any Calculate::visit (Node::AdditionSubtraction& node)
{
	data_t result = 0;

	for (auto& child: node.children()) {
		if (child.tag.negated) {
			result -= child.node->accept_value (*this);
		} else {
			result += child.node->accept_value (*this);
		}
	}

	return result;
}

boost::any Calculate::visit (Node::MultiplicationDivision& node)
{
	data_t result = 1;

	for (auto& child: node.children()) {
		if (child.tag.reciprocated) {
			result /= child.node->accept_value (*this);
		} else {
			result *= child.node->accept_value (*this);
		}
	}

	return result;
}

} // namespace Visitor
