#pragma once

#include "visitor.h"

namespace Visitor {

class Print : public Base
{
	std::ostream& stream_;

	boost::any parenthesized_visit (Node::Base& node, Node::Base::Ptr& child);
	boost::any parenthesized_visit (Node::Base& node, size_t child);

public:
	Print (std::ostream& stream);

	virtual boost::any visit (Node::Value& node);
	virtual boost::any visit (Node::Variable& node);
	virtual boost::any visit (Node::Function& node);
	virtual boost::any visit (Node::Power& node);
	virtual boost::any visit (Node::AdditionSubtraction& node);
	virtual boost::any visit (Node::MultiplicationDivision& node);
};

} // namespace Visitor
