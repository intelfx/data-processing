#include "visitor-calculate.h"

#include <sstream>

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

	data_t base = boost::any_cast<data_t> ((*child++)->accept (*this)),
	       exponent = boost::any_cast<data_t> ((*child++)->accept (*this));

	return powl (base, exponent);
}

boost::any Calculate::visit (Node::AdditionSubtraction& node)
{
	data_t result = 0;

	auto negation = node.negation().begin();
	auto child = node.children().begin();

	while (child != node.children().end()) {
		if (*negation++) {
			result -= boost::any_cast<data_t> ((*child++)->accept (*this));
		} else {
			result += boost::any_cast<data_t> ((*child++)->accept (*this));
		}
	}

	return result;
}

boost::any Calculate::visit (Node::MultiplicationDivision& node)
{
	data_t result = 1;

	auto reciprocation = node.reciprocation().begin();
	auto child = node.children().begin();

	while (child != node.children().end()) {
		if (*reciprocation++) {
			result /= boost::any_cast<data_t> ((*child++)->accept (*this));
		} else {
			result *= boost::any_cast<data_t> ((*child++)->accept (*this));
		}
	}

	return result;
}

} // namespace Visitor
