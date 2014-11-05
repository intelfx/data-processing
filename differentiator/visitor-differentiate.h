#pragma once

#include "visitor.h"

class DifferentiateVisitor : public Visitor
{
	std::string variable_;

public:
	DifferentiateVisitor (const std::string& variable);

	virtual boost::any visit (Node::Value& node);
	virtual boost::any visit (Node::Variable& node);
	virtual boost::any visit (Node::Function& node);
	virtual boost::any visit (Node::Power& node);
	virtual boost::any visit (Node::AdditionSubtraction& node);
	virtual boost::any visit (Node::MultiplicationDivision& node);
};
