#include "visitor-differentiate.h"

namespace Visitor {

Differentiate::Differentiate(const std::string& variable)
: variable_ (variable)
{
}

boost::any Differentiate::visit (const Node::Value&)
{
	return static_cast<Node::Base*> (new Node::Value (0));
}

boost::any Differentiate::visit (const Node::Variable& node)
{
	if (node.is_target_variable (variable_)) {
		return static_cast<Node::Base*> (new Node::Value (1));
	} else {
		return static_cast<Node::Base*> (new Node::Value (0));
	}
}

boost::any Differentiate::visit (const Node::Function& node)
{

	typedef std::list<Node::TaggedChild<void>> Children;
	typedef std::function<Node::Base*(Base&, const Children&)> Differentiator;

	static std::unordered_map<std::string, Differentiator> differentiators {
		{ "ln", [](Base& visitor, const Children& children) -> Node::Base* {
			Node::MultiplicationDivision::Ptr result (new Node::MultiplicationDivision);

			result->add_child (children.front().node->accept_ptr (visitor), false);
			result->add_child (children.front().node->clone(), true);

			return result.release();
		} }
	};

	auto it = differentiators.find (node.name());
	if (it != differentiators.end()) {
			return it->second (*this, node.children());
	} else {
		ERROR (std::runtime_error, "Differentiate error: unknown function: '" << node.name() << "'");
	}
}

boost::any Differentiate::visit (const Node::AdditionSubtraction& node)
{
	Node::AdditionSubtraction::Ptr result (new Node::AdditionSubtraction);

	for (auto& child: node.children()) {
		result->add_child (child.node->accept_ptr (*this), child.tag.negated);
	}

	return static_cast<Node::Base*> (result.release());
}

boost::any Differentiate::visit (const Node::MultiplicationDivision& node)
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
			g_squared->set_base (std::move (g));
			g_squared->set_exponent (Node::Base::Ptr (new Node::Value (2)));

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

		so_far->add_child (child.node->clone(), child.tag.reciprocated);
	}

	return static_cast<Node::Base*> (deriv_so_far.release());
}

boost::any Differentiate::visit (const Node::Power& node)
{
	/* f, a */
	const Node::Base::Ptr &base = node.get_base(),
	                      &exponent = node.get_exponent();

	const Node::Value* exponent_value = dynamic_cast<const Node::Value*> (exponent.get());
	if (!exponent_value) {
		ERROR (std::runtime_error, "Differentiate error: sorry, unimplemented: exponent is not a constant");
	}

	/* f^(a-1) */
	Node::Power::Ptr pwr_minus_one (new Node::Power);
	pwr_minus_one->set_base (base->clone());
	pwr_minus_one->set_exponent (Node::Base::Ptr (new Node::Value (exponent_value->value() - 1)));

	/* a * f^(a-1) * f' */
	Node::MultiplicationDivision::Ptr result (new Node::MultiplicationDivision);
	result->add_child (exponent->clone(), false);
	result->add_child (std::move (pwr_minus_one), false);
	result->add_child (base->accept_ptr (*this), false);

	return static_cast<Node::Base*> (result.release());
}

} // namespace Visitor
