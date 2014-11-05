#include "visitor-print.h"

PrintVisitor::PrintVisitor (std::ostream& stream)
: stream_ (stream)
{
}

boost::any PrintVisitor::parenthesized_visit (Node::Base& parent, Node::Base::Ptr& child)
{
	bool need_parentheses = (child->priority() < parent.priority());

	if (need_parentheses) {
		stream_ << "(";
	}

	boost::any ret = child->accept (*this);

	if (need_parentheses) {
		stream_ << ")";
	}

	return std::move (ret);
}

boost::any PrintVisitor::parenthesized_visit (Node::Base& parent, size_t child_idx)
{
	return parenthesized_visit (parent, parent.children().at (child_idx));
}

boost::any PrintVisitor::visit (Node::Value& node)
{
	stream_ << node.value();

	return boost::any();
}

boost::any PrintVisitor::visit (Node::Variable& node)
{
	stream_ << node.name();

	return boost::any();
}

boost::any PrintVisitor::visit (Node::Function& node)
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

boost::any PrintVisitor::visit (Node::Power& node)
{
	parenthesized_visit (node, 0);
	stream_ << "^";
	parenthesized_visit (node, 1);

	return boost::any();
}

boost::any PrintVisitor::visit (Node::AdditionSubtraction& node)
{
	auto negation = node.negation().begin();
	auto child = node.children().begin();

	if (*negation++) {
		stream_ << "- ";
	}
	parenthesized_visit (node, *child++);

	while (child != node.children().end()) {
		if (*negation++) {
			stream_ << " - ";
		} else {
			stream_ << " + ";
		}
		parenthesized_visit (node, *child++);
	}

	return boost::any();
}

boost::any PrintVisitor::visit (Node::MultiplicationDivision& node)
{
	auto reciprocation = node.reciprocation().begin();
	auto child = node.children().begin();

	if (*reciprocation++) {
		stream_ << "/ ";
	}
	parenthesized_visit (node, *child++);

	while (child != node.children().end()) {
		if (*reciprocation++) {
			stream_ << " / ";
		} else {
			stream_ << " * ";
		}
		parenthesized_visit (node, *child++);
	}

	return boost::any();
}
