#include "visitor-differentiate.h"

#include <sstream>

namespace Visitor {

Differentiate::Differentiate(const std::string& variable)
: variable_ (variable)
{
}

boost::any Differentiate::visit (Node::Value&)
{
	return static_cast<Node::Base*> (new Node::Value (0));
}

boost::any Differentiate::visit (Node::Variable& node)
{
	if (node.is_target_variable (variable_)) {
		return static_cast<Node::Base*> (new Node::Value (1));
	} else {
		return static_cast<Node::Base*> (new Node::Value (0));
	}
}

boost::any Differentiate::visit (Node::Function& node)
{
	std::ostringstream reason;
	reason << "Differentiate error: unknown function: '" << node.name() << "'";
	throw std::runtime_error (reason.str());
}

boost::any Differentiate::visit (Node::AdditionSubtraction& node)
{
	Node::AdditionSubtraction::Ptr result (new Node::AdditionSubtraction);

	auto negation = node.negation().begin();
	auto child = node.children().begin();

	while (child != node.children().end()) {
		result->add_child ((*child++)->accept_ptr (*this), *negation++);
	}

	return static_cast<Node::Base*> (result.release());
}

boost::any Differentiate::visit (Node::MultiplicationDivision& node)
{
	Node::MultiplicationDivision::Ptr so_far (new Node::MultiplicationDivision);
	Node::Base::Ptr deriv_so_far;

	auto reciprocation = node.reciprocation().begin();
	auto child = node.children().begin();

	/*
	 * Handle first multiplier/divisor specially to simplify the expression.
	 */
	if (*reciprocation) {
		/* f, f' */
		Node::Base::Ptr term ((*child)->clone()),
		                deriv ((*child)->accept_ptr (*this));

		/* f^2 */
		Node::Power::Ptr squared (new Node::Power);
		squared->add_child (std::move (term));
		squared->add_child (Node::Base::Ptr (new Node::Value (2)));

		/* -f' */
		Node::AdditionSubtraction::Ptr minus_deriv (new Node::AdditionSubtraction);
		minus_deriv->add_child (std::move (deriv), true);

		/* -f'/f^2 */
		Node::MultiplicationDivision::Ptr current (new Node::MultiplicationDivision);
		current->add_child (std::move (minus_deriv), false);
		current->add_child (std::move (squared), true);
		deriv_so_far = std::move (current);
	} else {
		/* just f' */
		deriv_so_far = (*child)->accept_ptr (*this);
	}
	so_far->add_child ((*child++)->clone(), *reciprocation++);

	while (child != node.children().end()) {
		/* common: g, g' */
		Node::Base::Ptr g ((*child)->clone()),
		                deriv_g ((*child)->accept_ptr (*this));

		/* common: f'g */
		Node::MultiplicationDivision::Ptr deriv_f_g (new Node::MultiplicationDivision);
		deriv_f_g->add_child (std::move (deriv_so_far), false);
		deriv_f_g->add_child (g->clone() /* g is needed later if reciprocation == true */, false);

		/* common: fg' */
		Node::MultiplicationDivision::Ptr f_deriv_g (new Node::MultiplicationDivision);
		f_deriv_g->add_child (so_far->clone() /* f is incremental */, false);
		f_deriv_g->add_child (std::move (deriv_g), false);

		if (*reciprocation) {
			/* g^2 */
			Node::Power::Ptr g_squared (new Node::Power);
			g_squared->add_child (std::move (g));
			g_squared->add_child (Node::Base::Ptr (new Node::Value (2)));

			/* f'g - fg' */
			Node::AdditionSubtraction::Ptr top (new Node::AdditionSubtraction);
			top->add_child (std::move (deriv_f_g), false);
			top->add_child (std::move (f_deriv_g), true);

			/* (f'g - fg') / g^2 */
			Node::MultiplicationDivision::Ptr current (new Node::MultiplicationDivision);
			current->add_child (std::move (top), false);
			current->add_child (std::move (g_squared), true);
			deriv_so_far = std::move (current);
		} else {
			/* f'g + fg' */
			Node::AdditionSubtraction::Ptr current (new Node::AdditionSubtraction);
			current->add_child (std::move (deriv_f_g), false);
			current->add_child (std::move (f_deriv_g), false);
			deriv_so_far = std::move (current);
		}
		so_far->add_child ((*child++)->clone(), *reciprocation++);
	}

	return static_cast<Node::Base*> (deriv_so_far.release());
}

boost::any Differentiate::visit (Node::Power& node)
{
	auto child = node.children().begin();

	/* f, a */
	Node::Base::Ptr &base = *child++,
	                &exponent = *child++;

	Node::Value* exponent_value = dynamic_cast<Node::Value*> (exponent.get());
	if (!exponent_value) {
		std::ostringstream reason;
		reason << "Differentiate error: sorry, unimplemented: exponent is not a constant";
		throw std::runtime_error (reason.str());
	}

	/* f^(a-1) */
	Node::Power::Ptr pwr_minus_one (new Node::Power);
	pwr_minus_one->add_child (base->clone());
	pwr_minus_one->add_child (Node::Base::Ptr (new Node::Value (exponent_value->value() - 1)));

	/* a * f^(a-1) * f' */
	Node::MultiplicationDivision::Ptr result (new Node::MultiplicationDivision);
	result->add_child (exponent->clone(), false);
	result->add_child (std::move (pwr_minus_one), false);
	result->add_child (base->accept_ptr (*this), false);

	return static_cast<Node::Base*> (result.release());
}

} // namespace Visitor
