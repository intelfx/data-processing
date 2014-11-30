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
		for (auto& child: base_muldiv->children()) {
			/* replace each child with its power */
			Node::Power::Ptr child_pwr (new Node::Power);
			child_pwr->add_child (std::move (child.node));
			child_pwr->add_child (exponent->clone());
			child.node = std::move (child_pwr);
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

struct DisassembledNode
{
	data_t constant;
	Node::Base::Ptr subtree;
};

DisassembledNode disassemble_muldiv (const Node::Base::Ptr& node, bool negate)
{
	DisassembledNode result { 1, node->clone() };

	Node::MultiplicationDivision* result_muldiv = dynamic_cast<Node::MultiplicationDivision*> (result.subtree.get());
	if (result_muldiv) {
		result.constant = result_muldiv->get_constant_value_and_release();
		if (result_muldiv->children().size() == 1) {
			result.subtree = std::move (result_muldiv->children().front().node);
		}
	}

	if (negate) {
		result.constant = -result.constant;
	}

	return std::move (result);
}

Node::Base::Ptr assemble_muldiv (DisassembledNode&& node_data)
{
	if (fp_cmp (node_data.constant, 0)) {
		return Node::Base::Ptr (new Node::Value (0));
	} else if (fp_cmp (node_data.constant, 1)) {
		return std::move (node_data.subtree);
	} else {
		Node::MultiplicationDivision* node_muldiv = dynamic_cast<Node::MultiplicationDivision*> (node_data.subtree.get());
		if (node_muldiv) {
			node_muldiv->insert_constant (node_data.constant);
			return std::move (node_data.subtree);
		} else {
			Node::MultiplicationDivision::Ptr new_muldiv (new Node::MultiplicationDivision);
			new_muldiv->insert_constant (node_data.constant);
			new_muldiv->add_child (std::move (node_data.subtree), false);
			return std::move (new_muldiv);
		}
	}
}

void Simplify::fold_with_children (Node::AdditionSubtraction::Ptr& result, Node::Base::Ptr& node, bool& node_negation)
{
	/* Folding semantics:
	 * Merge "a * X" specified by pair (node; node_negation) into any "b * X" specified by one of result's children,
	 * producing "(a + b) * X". The result must be simplified and written into (node; node_negation).
	 * The child node that took part in the folding must be removed. */

	/* Do not fold constants, this is meaningless. */
	if (dynamic_cast<Node::Value*> (node.get())) {
		return;
	}

	/* Verify that we have any nodes to attempt folding with. */
	if (!result) {
		return;
	}

	/* disassemble (node; node_negation) into a constant and a subtree. */
	DisassembledNode node_data = disassemble_muldiv (node, node_negation);

	for (auto child = result->children().begin(); child != result->children().end(); ++child) {
		DisassembledNode child_data = disassemble_muldiv (child->node, child->tag.negated);

		if (child_data.subtree->compare (node_data.subtree)) {
			/* Subtrees match.
			 * Do the folding (sum up constants). */
			node_data.constant += child_data.constant;

			/* Avoid "+(-x)"
			 * FIXME maybe this is something to do unconditionally in simplify_nested_nodes() */
			if (node_data.constant < 0) {
				node_data.constant = -node_data.constant;
				node_negation = true;
			} else {
				node_negation = false;
			}

			/* Build the node and replace source node with the folded version. */
			node = assemble_muldiv (std::move (node_data));

			/* Erase the second node participated in folding.
			 * Iterator becomes invalid - return immediately. */
			result->children().erase (child);
			return;
		}
	}
}

void Simplify::simplify_nested_nodes (data_t& result_value, Node::AdditionSubtraction::Ptr& result, Node::AdditionSubtraction& node, bool node_negation)
{
	for (auto& child: node.children()) {
		bool negation = child.tag.negated ^ node_negation;
		Node::Base::Ptr simplified (child.node->accept_ptr (*this));

		fold_with_children (result, simplified, negation);

		Node::Value* simplified_value = dynamic_cast<Node::Value*> (simplified.get());
		Node::AdditionSubtraction* simplified_addsub = dynamic_cast<Node::AdditionSubtraction*> (simplified.get());

		if (simplified_value) {
			if (negation) {
				result_value -= simplified_value->value();
			} else {
				result_value += simplified_value->value();
			}
		} else {
			if (!result) {
				result = Node::AdditionSubtraction::Ptr (new Node::AdditionSubtraction);
			}

			if (simplified_addsub) {
				simplify_nested_nodes (result_value, result, *simplified_addsub, negation);
			} else {
				result->add_child (std::move (simplified), negation);
			}
		}
	}
}

boost::any Simplify::visit (Node::AdditionSubtraction& node)
{
	data_t result_value = 0;
	Node::AdditionSubtraction::Ptr result;

	simplify_nested_nodes (result_value, result, node, false);

	if (result) {
		/* we have at least one non-constant node.
		 * add the constant (as the first child) if needed */
		if (!fp_cmp (result_value, 0)) {
			result->add_child (Node::Base::Ptr (new Node::Value (result_value)), false);
		}

		/* if we still have only one child, try to get rid of our node entirely */
		if (result->children().size() == 1) {
			Node::Base::Ptr& child = result->children().front().node;

			if (result->children().front().tag.negated) {
				/* our only child is negated, check if we can push the sign into it */
				Node::MultiplicationDivision* child_muldiv = dynamic_cast<Node::MultiplicationDivision*> (child.get());
				if (child_muldiv) {
					/* mul-div nodes have a constant which can be multiplied by -1 */
					child_muldiv->insert_constant (-1);
					return static_cast<Node::Base*> (child.release());
				}
			} else {
				/* our only child is non-negated, just return it as-is (+x == x) */
				return static_cast<Node::Base*> (child.release());
			}
		}

		return static_cast<Node::Base*> (result.release());
	} else {
		/* we do not have any nodes besides the constant, return it directly */
		return static_cast<Node::Base*> (new Node::Value (result_value));
	}
}

DisassembledNode disassemble_power (const Node::Base::Ptr& node, bool reciprocate)
{
	DisassembledNode result { 1, node->clone() };

	Node::Power* result_power = dynamic_cast<Node::Power*> (result.subtree.get());
	if (result_power) {
		auto child = result_power->children().begin();
		Node::Base::Ptr& result_base = *child++;
		Node::Value* result_exponent_value = dynamic_cast<Node::Value*> (child->get());

		if (result_exponent_value) {
			result.constant = result_exponent_value->value();
			result.subtree = std::move (result_base);
		}
	}

	if (reciprocate) {
		result.constant = -result.constant;
	}

	return std::move (result);
}

Node::Base::Ptr assemble_power (DisassembledNode&& node_data, Simplify& simplifier)
{
	if (fp_cmp (node_data.constant, 0)) {
		return Node::Base::Ptr (new Node::Value (1));
	} else if (fp_cmp (node_data.constant, 1)) {
		return std::move (node_data.subtree);
	} else {
		Node::Power::Ptr new_power (new Node::Power);
		new_power->add_child (std::move (node_data.subtree));
		new_power->add_child (Node::Base::Ptr (new Node::Value (node_data.constant)));
		return new_power->accept_ptr (simplifier);
	}
}

void Simplify::fold_with_children (Node::MultiplicationDivision::Ptr& result, Node::Base::Ptr& node, bool& node_reciprocation)
{
	/* Folding semantics:
	 * Merge "X^a" specified by pair (node; node_reciprocation) into any "X^b" specified by one of result's children,
	 * producing "X^(a+b)". The result must be simplified and written into (node; node_reciprocation).
	 * The child node that took part in the folding must be removed. */

	/* Do not fold constants, this is meaningless. */
	if (dynamic_cast<Node::Value*> (node.get())) {
		return;
	}

	/* Verify that we have any nodes to attempt folding with. */
	if (!result) {
		return;
	}

	/* disassemble (node; node_negation) into a constant and a subtree. */
	DisassembledNode node_data = disassemble_power (node, node_reciprocation);

	for (auto child = result->children().begin(); child != result->children().end(); ++child) {
		DisassembledNode child_data = disassemble_power (child->node, child->tag.reciprocated);

		if (child_data.subtree->compare (node_data.subtree)) {
			/* Subtrees match.
			 * Do the folding (sum up constants). */
			node_data.constant += child_data.constant;

			/* Avoid "*(x^-1)"
			 * FIXME maybe this is something to do unconditionally in simplify_nested_nodes() */
			if (node_data.constant < 0) {
				node_data.constant = -node_data.constant;
				node_reciprocation = true;
			} else {
				node_reciprocation = false;
			}

			/* Build the node and replace source node with the folded version. */
			node = assemble_power (std::move (node_data), *this);

			/* Erase the second node participated in folding.
			 * Iterator becomes invalid - return immediately. */
			result->children().erase (child);
			return;
		}
	}
}

void Simplify::simplify_nested_nodes (data_t& result_value, Node::MultiplicationDivision::Ptr& result, Node::MultiplicationDivision& node, bool node_reciprocation)
{
	for (auto& child: node.children()) {
		Node::Base::Ptr simplified (child.node->accept_ptr (*this));
		bool reciprocation = child.tag.reciprocated ^ node_reciprocation;

		fold_with_children (result, simplified, reciprocation);

		Node::Value* simplified_value = dynamic_cast<Node::Value*> (simplified.get());
		Node::MultiplicationDivision* simplified_muldiv = dynamic_cast<Node::MultiplicationDivision*> (simplified.get());

		if (simplified_value) {
			if (reciprocation) {
				result_value /= simplified_value->value();
			} else {
				result_value *= simplified_value->value();
			}
		} else {
			if (!result) {
				result = Node::MultiplicationDivision::Ptr (new Node::MultiplicationDivision);
			}

			if (simplified_muldiv) {
				simplify_nested_nodes (result_value, result, *simplified_muldiv, reciprocation);
			} else {
				result->add_child (std::move (simplified), reciprocation);
			}
		}
	}
}

boost::any Simplify::visit (Node::MultiplicationDivision& node)
{
	data_t result_value = 1;
	Node::MultiplicationDivision::Ptr result;

	simplify_nested_nodes (result_value, result, node, false);

	if (result && !fp_cmp (result_value, 0)) {
		/* we have at least one non-constant node and we are not multiplied by zero.
		 * add the constant (as the first child) if needed */
		result->insert_constant (result_value);

		/* if we still have only one child, try to get rid of our node entirely */
		if (result->children().size() == 1) {
			if (!result->children().front().tag.reciprocated) {
				/* our only child is non-reciprocated, just return it as-is */
				return static_cast<Node::Base*> (result->children().front().node.release());
			}
		}

		return static_cast<Node::Base*> (result.release());
	} else {
		/* we do not have any nodes besides the constant, return it directly */
		return static_cast<Node::Base*> (new Node::Value (result_value));
	}

}

} // namespace Visitor
