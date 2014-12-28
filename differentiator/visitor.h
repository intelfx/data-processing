#pragma once

#include "node.h"

namespace Visitor {

class Base
{
public:
	virtual boost::any visit (const Node::Value& node) = 0;
	virtual boost::any visit (const Node::Variable& node) = 0;
	virtual boost::any visit (const Node::Function& node) = 0;
	virtual boost::any visit (const Node::Power& node) = 0;
	virtual boost::any visit (const Node::AdditionSubtraction& node) = 0;
	virtual boost::any visit (const Node::MultiplicationDivision& node) = 0;
};

} // namespace Visitor
