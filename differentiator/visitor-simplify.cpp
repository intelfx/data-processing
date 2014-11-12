#include "visitor-simplify.h"

#include <sstream>

namespace Visitor {

Simplify::Simplify (const std::string& variable)
: simplification_variable_ (variable)
{
}

Simplify::Simplify()
{
}

boost::any Simplify::visit (Node::Value& node)
{
	return static_cast<Node::Base*> (node.clone().release());
}

boost::any Simplify::visit (Node::Variable& node)
{
	if (simplification_variable_.empty() || node.is_target_variable (simplification_variable_)) {
		return static_cast<Node::Base*> (node.clone().release());
	} else {
		return static_cast<Node::Base*> (new Node::Value (node.value()));
	}
}

boost::any Simplify::visit (Node::Function& node)
{
	std::ostringstream reason;
	reason << "Simplify error: unknown function: '" << node.name() << "'";
	throw std::runtime_error (reason.str());
}

boost::any Simplify::visit (Node::Power& node)
{
	auto child = node.children().begin();

	Node::Base::Ptr base ((*child++)->accept_ptr (*this)),
	                exponent ((*child++)->accept_ptr (*this));

	Node::Value *base_value = dynamic_cast<Node::Value*> (base.get()),
	            *exponent_value = dynamic_cast<Node::Value*> (exponent.get());
	Node::MultiplicationDivision* base_muldiv = dynamic_cast<Node::MultiplicationDivision*> (base.get());
	Node::Power* base_power = dynamic_cast<Node::Power*> (base.get());

	if (base_value && exponent_value) {
		return static_cast<Node::Base*> (new Node::Value (powl (base_value->value(), exponent_value->value())));
	} else if (base_value && fp_cmp (base_value->value(), 0)) {
		return static_cast<Node::Base*> (new Node::Value (0));
	} else if (base_value && fp_cmp (base_value->value(), 1)) {
		return static_cast<Node::Base*> (new Node::Value (1));
	} else if (exponent_value && fp_cmp (exponent_value->value(), 0)) {
		return static_cast<Node::Base*> (new Node::Value (1));
	} else if (exponent_value && fp_cmp (exponent_value->value(), 1)) {
		return static_cast<Node::Base*> (base.release());
	} else if (base_muldiv) {
		for (Node::Base::Ptr& child: base_muldiv->children()) {
			Node::Power::Ptr child_pwr (new Node::Power);
			child_pwr->add_child (std::move (child));
			child_pwr->add_child (exponent->clone());
			child = std::move (child_pwr);
		}
		return base->accept_ptr (*this).release();
	} else if (base_power) {
		child = base_power->children().begin();

		Node::Base::Ptr base_base (std::move (*child++)),
		                base_exponent (std::move (*child++));

		Node::MultiplicationDivision::Ptr result_exponent (new Node::MultiplicationDivision);
		result_exponent->add_child (std::move (base_exponent), false);
		result_exponent->add_child (std::move (exponent), false);

		Node::Power::Ptr result (new Node::Power);
		result->add_child (std::move (base_base));
		result->add_child (std::move (result_exponent));
		return result->accept_ptr (*this).release();
	} else {
		Node::Power::Ptr result (new Node::Power);
		result->add_child (std::move (base));
		result->add_child (std::move (exponent));
		return static_cast<Node::Base*> (result.release());
	}
}

void Simplify::merge_node (data_t& result_value, Node::AdditionSubtraction::Ptr& result, Node::AdditionSubtraction& node, bool node_negation)
{
	auto child = node.children().begin();
	auto negation = node.negation().begin();

	while (child != node.children().end()) {
		Node::Base::Ptr simplified ((*child++)->accept_ptr (*this));
		Node::Value* simplified_value = dynamic_cast<Node::Value*> (simplified.get());
		Node::AdditionSubtraction* simplified_addsub = dynamic_cast<Node::AdditionSubtraction*> (simplified.get());

		if (simplified_value) {
			if (*negation++ ^ node_negation) {
				result_value -= simplified_value->value();
			} else {
				result_value += simplified_value->value();
			}
		} else {
			if (!result) {
				result = Node::AdditionSubtraction::Ptr (new Node::AdditionSubtraction);
			}

			if (simplified_addsub) {
				merge_node (result_value, result, *simplified_addsub, *negation++ ^ node_negation);
			} else {
				result->add_child (std::move (simplified), *negation++ ^ node_negation);
			}
		}
	}
}

boost::any Simplify::visit (Node::AdditionSubtraction& node)
{
	data_t result_value = 0;
	Node::AdditionSubtraction::Ptr result;

	merge_node (result_value, result, node, false);

	if (result) {
		if (!fp_cmp (result_value, 0)) {
			result->add_child (Node::Base::Ptr (new Node::Value (fabsl (result_value))), result_value < 0);
		}

		if (result->children().size() == 1) {
			Node::Base::Ptr& child = result->children().front();

			if (!result->negation().front()) {
				return static_cast<Node::Base*> (child.release());
			} else {
				Node::MultiplicationDivision* child_muldiv = dynamic_cast<Node::MultiplicationDivision*> (child.get());
				if (child_muldiv) {
					// push sign into the mul-div node
					// the constant, if exists, is the first child of a mul-div
					Node::Base::Ptr& child_child = child->children().front();
					Node::Value* child_child_value = dynamic_cast<Node::Value*> (child_child.get());
					if (child_child_value) {
						// multiply the constant by -1, if it exists
						child_child_value->set_value (child_child_value->value() * -1);
					} else {
						// insert the constant, if it doesn't
						child_muldiv->add_child_front (Node::Base::Ptr (new Node::Value (-1)), false);
					}

					return static_cast<Node::Base*> (child.release());
				}
			}
		}

		return static_cast<Node::Base*> (result.release());
	} else {
		return static_cast<Node::Base*> (new Node::Value (result_value));
	}
}

void Simplify::merge_node (data_t& result_value, Node::MultiplicationDivision::Ptr& result, Node::MultiplicationDivision& node, bool node_reciprocation)
{
	auto child = node.children().begin();
	auto reciprocation = node.reciprocation().begin();

	while (child != node.children().end()) {
		Node::Base::Ptr simplified ((*child++)->accept_ptr (*this));
		Node::Value* simplified_value = dynamic_cast<Node::Value*> (simplified.get());
		Node::MultiplicationDivision* simplified_muldiv = dynamic_cast<Node::MultiplicationDivision*> (simplified.get());

		if (simplified_value) {
			if (*reciprocation++ ^ node_reciprocation) {
				result_value /= simplified_value->value();
			} else {
				result_value *= simplified_value->value();
			}
		} else {
			if (!result) {
				result = Node::MultiplicationDivision::Ptr (new Node::MultiplicationDivision);
			}

			if (simplified_muldiv) {
				merge_node (result_value, result, *simplified_muldiv, *reciprocation++ ^ node_reciprocation);
			} else {
				result->add_child (std::move (simplified), *reciprocation++ ^ node_reciprocation);
			}
		}
	}
}

boost::any Simplify::visit (Node::MultiplicationDivision& node)
{
	data_t result_value = 1;
	Node::MultiplicationDivision::Ptr result;

	merge_node (result_value, result, node, false);

	if (result && !fp_cmp (result_value, 0)) {
		if (!fp_cmp (result_value, 1)) {
			if (fabsl (result_value) < 1) {
				result->add_child_front (Node::Base::Ptr (new Node::Value (1 / result_value)), true);
			} else {
				result->add_child_front (Node::Base::Ptr (new Node::Value (result_value)), false);
			}
		}

		if ((result->children().size() == 1) && !result->reciprocation().front()) {
			return static_cast<Node::Base*> (result->children().front().release());
		} else {
			return static_cast<Node::Base*> (result.release());
		}
	} else {
		return static_cast<Node::Base*> (new Node::Value (result_value));
	}
}

} // namespace Visitor
