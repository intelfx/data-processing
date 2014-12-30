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

	struct Options
	{
		bool expand_product = false;
		bool sum_fractions = true;
	};

	static Options options;
};

} // namespace Visitor
