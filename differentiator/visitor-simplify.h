#pragma once

#include "visitor.h"

namespace Visitor {

class Simplify : public Base
{
	std::string simplification_variable_;

public:

	Simplify (const std::string& variable);
	Simplify();

	virtual boost::any visit (const Node::Value& node);
	virtual boost::any visit (const Node::Variable& node);
	virtual boost::any visit (const Node::Function& node);
	virtual boost::any visit (const Node::Power& node);
	virtual boost::any visit (const Node::AdditionSubtraction& node);
	virtual boost::any visit (const Node::MultiplicationDivision& node);
};

} // namespace Visitor
