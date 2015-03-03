#include "visitor-print.h"

namespace Visitor {

Print::Print (std::ostream& stream, bool substitute)
: stream_ (stream)
, substitute_ (substitute)
, paren_left_ ("(")
, paren_right_ (")")
{
}

Print::Print (std::ostream& stream, bool substitute, std::string paren_left, std::string paren_right)
: stream_ (stream)
, substitute_ (substitute)
, paren_left_ (std::move (paren_left))
, paren_right_ (std::move (paren_right))
{
}

boost::any Print::parenthesized_visit (Node::Priority parent_priority, const Node::Base::Ptr& child)
{
	bool need_parentheses = (child->priority() <= parent_priority);

	if (need_parentheses) {
		stream_ << paren_left_;
	}

	boost::any ret = child->accept (*this);

	if (need_parentheses) {
		stream_ << paren_right_;
	}

	return ret;
}


boost::any Print::parenthesized_visit (const Node::Base& parent, const Node::Base::Ptr& child)
{
	return parenthesized_visit (parent.priority(), child);
}

void Print::maybe_print_multiplication (const Node::Base::Ptr& child)
{
	/* elide multiplication sign only we're outputting symbolic variable names */
	if (substitute_ || child->numeric_output()) {
		stream_ << " * ";
	} else {
		stream_ << " ";
	}
}

boost::any Print::visit (const Node::Value& node)
{
	rational_to_ostream (stream_, node.value());

	return boost::any();
}

boost::any Print::visit (const Node::Variable& node)
{
	if (substitute_ && node.can_be_substituted() && !node.value().empty()) {
		any_to_ostream (stream_, node.value());
	} else {
		stream_ << node.pretty_name();
	}

	return boost::any();
}

boost::any Print::visit (const Node::Function& node)
{
	stream_ << node.name() << "(";

	if (!node.children().empty()) {
		auto child = node.children().begin();

		(*child++).node->accept (*this);

		while (child != node.children().end()) {
			stream_ << ", ";
			(*child++).node->accept (*this);
		}
	}

	stream_ << ")";

	return boost::any();
}

boost::any Print::visit (const Node::Power& node)
{
	const Node::Base::Ptr &base = node.get_base(),
	                      &exponent = node.get_exponent();

	const Node::Value* exponent_value = dynamic_cast<const Node::Value*> (exponent.get());

	if (exponent_value &&
	    exponent_value->value().numerator() == 1) {
		integer_t denominator = exponent_value->value().denominator();
		if (denominator == 2) {
			stream_ << "sqrt(";
		} else if (denominator == 3) {
			stream_ << "cbrt(";
		} else {
			stream_ << "root[" << denominator << "](";
		}
		/* skip parenthesizing anything because { .. } are effectively parentheses */
		base->accept (*this);
		stream_ << ")";
	} else if (exponent_value &&
	           exponent_value->value() == rational_t (-1)) {
		stream_ << "1 / ";
		/* parenthesize as if we were a muldiv node */
		parenthesized_visit (Node::MultiplicationDivision::priority_static(), base);
	} else {
		parenthesized_visit (node, node.get_base());
		stream_ << "^";
		parenthesized_visit (node, node.get_exponent());
	}

	return boost::any();
}

boost::any Print::visit (const Node::AdditionSubtraction& node)
{
	bool first = true;

	for (auto& child: node.children()) {
		if (child.tag.negated) {
			stream_ << " - ";
		} else if (!first) {
			stream_ << " + ";
		}
		parenthesized_visit (node, child.node);

		first = false;
	}

	return boost::any();
}

boost::any Print::visit (const Node::MultiplicationDivision& node)
{
	bool first = true;

	for (auto& child: node.children()) {
		if (first) {
			if (child.tag.reciprocated) {
				stream_ << " 1 / ";
			}
		} else {
			if (child.tag.reciprocated) {
				stream_ << " / ";
			} else if (!first) {
				maybe_print_multiplication (child.node);
			}
		}
		parenthesized_visit (node, child.node);

		first = false;
	}

	return boost::any();
}

} // namespace Visitor
