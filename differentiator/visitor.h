#pragma once

#include <util.h>
#include "node.h"

class Visitor
{
public:
	virtual boost::any visit (Node::Value& node) = 0;
	virtual boost::any visit (Node::Variable& node) = 0;
	virtual boost::any visit (Node::Function& node) = 0;
	virtual boost::any visit (Node::Power& node) = 0;
	virtual boost::any visit (Node::AdditionSubtraction& node) = 0;
	virtual boost::any visit (Node::MultiplicationDivision& node) = 0;
};
