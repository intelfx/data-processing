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

	for (auto& child: node.children()) {
		result->add_child (child.node->accept_ptr (*this), child.tag);
	}

	return static_cast<Node::Base*> (result.release());
}

boost::any Differentiate::visit (Node::MultiplicationDivision& node)
{
	Node::MultiplicationDivision::Ptr so_far (new Node::MultiplicationDivision);
	Node::Base::Ptr deriv_so_far;

	for (auto& child: node.children()) {
		/* common: g, g' */
		Node::Base::Ptr g (child.node->clone()),
		                deriv_g (child.node->accept_ptr (*this));

		/* common: f'g, fg'
		 * (skipped on the first iteration because f = 1, f' = 0) */
		Node::MultiplicationDivision::Ptr deriv_f_g, f_deriv_g;
		if (deriv_so_far) {
			/* f'g */
			deriv_f_g = Node::MultiplicationDivision::Ptr (new Node::MultiplicationDivision);
			deriv_f_g->add_child (std::move (deriv_so_far), false);
			deriv_f_g->add_child (g->clone() /* g is needed later if reciprocation == true */, false);

			/* fg' */
			f_deriv_g = Node::MultiplicationDivision::Ptr (new Node::MultiplicationDivision);
			f_deriv_g->add_child (so_far->clone() /* f is incremental */, false);
			f_deriv_g->add_child (std::move (deriv_g), false);
		}

		if (child.tag.reciprocated) {
			/* g^2 */
			Node::Power::Ptr g_squared (new Node::Power);
			g_squared->add_child (std::move (g));
			g_squared->add_child (Node::Base::Ptr (new Node::Value (2)));

			/* f'g - fg' */
			Node::AdditionSubtraction::Ptr top (new Node::AdditionSubtraction);
			if (deriv_f_g) {
				top->add_child (std::move (deriv_f_g), false);
				top->add_child (std::move (f_deriv_g), true);
			} else {
				/* first iteration, see above */
				top->add_child (std::move (deriv_g), true);
			}

			/* (f'g - fg') / g^2 */
			Node::MultiplicationDivision::Ptr current (new Node::MultiplicationDivision);
			current->add_child (std::move (top), false);
			current->add_child (std::move (g_squared), true);
			deriv_so_far = std::move (current);
		} else {
			/* f'g + fg' */
			if (deriv_f_g) {
				Node::AdditionSubtraction::Ptr current (new Node::AdditionSubtraction);
				current->add_child (std::move (deriv_f_g), false);
				current->add_child (std::move (f_deriv_g), false);
				deriv_so_far = std::move (current);
			} else {
				/* first iteration, see above */
				deriv_so_far = std::move (deriv_g);
			}
		}

		so_far->add_child (child.node->clone(), child.tag);
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
