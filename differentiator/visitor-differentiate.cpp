#include "visitor-differentiate.h"

#include <sstream>

DifferentiateVisitor::DifferentiateVisitor(const std::string& variable)
: variable_ (variable)
{
}

boost::any DifferentiateVisitor::visit (Node::Value&)
{
	return static_cast<Node::Base*> (new Node::Value (0));
}

boost::any DifferentiateVisitor::visit (Node::Variable& node)
{
	if (node.name() == variable_) {
		return static_cast<Node::Base*> (new Node::Value (1));
	} else {
		return static_cast<Node::Base*> (new Node::Value (0));
	}
}

boost::any DifferentiateVisitor::visit (Node::Function& node)
{
	std::ostringstream reason;
	reason << "Differentiate error: unknown function: '" << node.name() << "'";
	throw std::runtime_error (reason.str());
}

boost::any DifferentiateVisitor::visit (Node::AdditionSubtraction& node)
{
	Node::AdditionSubtraction::Ptr result;

	auto negation = node.negation().begin();
	auto child = node.children().begin();

	while (child != node.children().end()) {
		Node::Base* derivative = boost::any_cast<Node::Base*> ((*child++)->accept (*this));
		if (derivative) {
			if (!result) {
				result = Node::AdditionSubtraction::Ptr (new Node::AdditionSubtraction);
			}
			result->add_child (Node::Base::Ptr (derivative), *negation++);
		}
	}

	return static_cast<Node::Base*> (result.release());
}

boost::any DifferentiateVisitor::visit (Node::MultiplicationDivision& node)
{
	Node::Base::Ptr result;

	auto reciprocation = node.reciprocation().begin();
	auto child = node.children().begin();

	/*
	 * Handle first multiplier/divisor specially to simplify the expression.
	 */
	if (*reciprocation++) {
		/* f, f' */
		Node::Base::Ptr term ((*child++)->clone()),
		                deriv (boost::any_cast<Node::Base*> (term->accept (*this)));

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
		result = std::move (current);
	} else {
		/* just f' */
		result = Node::Base::Ptr (boost::any_cast<Node::Base*> ((*child++)->accept (*this)));
	}

	while (child != node.children().end()) {
		/* common: f, g, f', g' */
		Node::Base::Ptr f (std::move (result)),
		                g ((*child++)->clone()),
		                deriv_f (boost::any_cast<Node::Base*> (f->accept (*this))),
		                deriv_g (boost::any_cast<Node::Base*> (g->accept (*this)));

		/* common: f'g */
		Node::MultiplicationDivision::Ptr deriv_f_g (new Node::MultiplicationDivision);
		deriv_f_g->add_child (std::move (deriv_f), false);
		deriv_f_g->add_child (g->clone() /* g is needed later if reciprocation == true */, false);

		/* common: fg' */
		Node::MultiplicationDivision::Ptr f_deriv_g (new Node::MultiplicationDivision);
		f_deriv_g->add_child (std::move (f), false);
		f_deriv_g->add_child (std::move (deriv_g), false);

		if (*reciprocation++) {
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
			result = std::move (current);
		} else {
			/* f'g + fg' */
			Node::AdditionSubtraction::Ptr current (new Node::AdditionSubtraction);
			current->add_child (std::move (deriv_f_g), false);
			current->add_child (std::move (f_deriv_g), false);
			result = std::move (current);
		}
	}

	return static_cast<Node::Base*> (result.release());
}

boost::any DifferentiateVisitor::visit (Node::Power& node)
{
	/* f, a */
	Node::Base::Ptr &base = node.children().at (0),
	                &exponent = node.children().at (1);

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
	result->add_child (Node::Base::Ptr (boost::any_cast<Node::Base*> (base->accept (*this))), false);

	return static_cast<Node::Base*> (result.release());
}
