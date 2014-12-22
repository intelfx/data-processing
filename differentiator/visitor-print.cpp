#include "visitor-print.h"

namespace Visitor {

Print::Print (std::ostream& stream, bool substitute)
: stream_ (stream)
, substitute_ (substitute)
{
}

boost::any Print::parenthesized_visit (Node::Base& parent, Node::Base::Ptr& child)
{
	bool need_parentheses = (child->priority() <= parent.priority());

	if (need_parentheses) {
		stream_ << "(";
	}

	boost::any ret = child->accept (*this);

	if (need_parentheses) {
		stream_ << ")";
	}

	return ret;
}

boost::any Print::visit (Node::Value& node)
{
	if (node.value().denominator() == 1) {
		stream_ << node.value().numerator();
	} else {
		stream_ << "(" << node.value() << ")";
	}

	return boost::any();
}

boost::any Print::visit (Node::Variable& node)
{
	if (substitute_ && node.can_be_substituted()) {
		stream_ << node.value();
	} else {
		stream_ << node.pretty_name();
	}

	return boost::any();
}

boost::any Print::visit (Node::Function& node)
{
	stream_ << node.name() << "(";

	if (!node.children().empty()) {
		auto child = node.children().begin();

		(*child++)->accept (*this);

		while (child != node.children().end()) {
			stream_ << ", ";
			(*child++)->accept (*this);
		}
	}

	stream_ << ")";

	return boost::any();
}

boost::any Print::visit (Node::Power& node)
{
	auto child = node.children().begin();

	parenthesized_visit (node, *child++);
	stream_ << "^";
	parenthesized_visit (node, *child++);

	return boost::any();
}

boost::any Print::visit (Node::AdditionSubtraction& node)
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

boost::any Print::visit (Node::MultiplicationDivision& node)
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
				stream_ << " * ";
			}
		}
		parenthesized_visit (node, child.node);

		first = false;
	}

	return boost::any();
}

} // namespace Visitor
