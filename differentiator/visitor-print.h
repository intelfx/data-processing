#pragma once

#include "visitor.h"

namespace Visitor {

class Print : public Base
{
protected:
	std::ostream& stream_;
	bool substitute_;
	const std::string paren_left_, paren_right_;

	boost::any parenthesized_visit (const Node::Base& node, const Node::Base::Ptr& child);
	void maybe_print_multiplication (const Node::Base::Ptr& child);

public:
	Print (std::ostream& stream, bool substitute);
	Print (std::ostream& stream, bool substitute, std::string paren_left, std::string paren_right);

	virtual boost::any visit (const Node::Value& node);
	virtual boost::any visit (const Node::Variable& node);
	virtual boost::any visit (const Node::Function& node);
	virtual boost::any visit (const Node::Power& node);
	virtual boost::any visit (const Node::AdditionSubtraction& node);
	virtual boost::any visit (const Node::MultiplicationDivision& node);
};

} // namespace Visitor
