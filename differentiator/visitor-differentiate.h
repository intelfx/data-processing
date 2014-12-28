#pragma once

#include "visitor.h"

namespace Visitor {

class Differentiate : public Base
{
	std::string variable_;

public:
	Differentiate (const std::string& variable);

	virtual boost::any visit (const Node::Value& node);
	virtual boost::any visit (const Node::Variable& node);
	virtual boost::any visit (const Node::Function& node);
	virtual boost::any visit (const Node::Power& node);
	virtual boost::any visit (const Node::AdditionSubtraction& node);
	virtual boost::any visit (const Node::MultiplicationDivision& node);
};

} // namespace Visitor
