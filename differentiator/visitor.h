#pragma once

#include "node.h"

namespace Visitor {

class Base
{
public:
	virtual boost::any visit (Node::Value& node) = 0;
	virtual boost::any visit (Node::Variable& node) = 0;
	virtual boost::any visit (Node::Function& node) = 0;
	virtual boost::any visit (Node::Power& node) = 0;
	virtual boost::any visit (Node::AdditionSubtraction& node) = 0;
	virtual boost::any visit (Node::MultiplicationDivision& node) = 0;
};

} // namespace Visitor
