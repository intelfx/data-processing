#pragma once

#include "visitor.h"

namespace Visitor {

class Simplify : public Base
{
	std::string variable_;

	void merge_node (data_t& result_value, Node::AdditionSubtraction::Ptr& result, Node::AdditionSubtraction& node, bool node_negation);
	void merge_node (data_t& result_value, Node::MultiplicationDivision::Ptr& result, Node::MultiplicationDivision& node, bool node_reciprocation);

public:
	Simplify (const std::string& variable);

	virtual boost::any visit (Node::Value& node);
	virtual boost::any visit (Node::Variable& node);
	virtual boost::any visit (Node::Function& node);
	virtual boost::any visit (Node::Power& node);
	virtual boost::any visit (Node::AdditionSubtraction& node);
	virtual boost::any visit (Node::MultiplicationDivision& node);
};

} // namespace Visitor
