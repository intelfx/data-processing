#pragma once

#include "visitor.h"

namespace Visitor {

class Differentiate : public Base
{
	std::string variable_;

public:
	Differentiate (const std::string& variable);

	virtual boost::any visit (Node::Value& node);
	virtual boost::any visit (Node::Variable& node);
	virtual boost::any visit (Node::Function& node);
	virtual boost::any visit (Node::Power& node);
	virtual boost::any visit (Node::AdditionSubtraction& node);
	virtual boost::any visit (Node::MultiplicationDivision& node);
};

} // namespace Visitor
