#pragma once

#include "visitor.h"

namespace Visitor {

class Simplify : public Base
{
	std::string simplification_variable_;

	void simplify_nested_nodes (rational_t& result_value, Node::AdditionSubtraction::Ptr& result, Node::AdditionSubtraction& node, bool node_negation);
	void simplify_nested_nodes (rational_t& result_value, Node::MultiplicationDivision::Ptr& result, Node::MultiplicationDivision& node, bool node_reciprocation);

	void fold_with_children (Node::AdditionSubtraction::Ptr& result, Node::Base::Ptr& node, bool& node_negation);
	void fold_with_children (Node::MultiplicationDivision::Ptr& result, Node::Base::Ptr& node, bool& node_negation);

public:
	Simplify (const std::string& variable);
	Simplify();

	virtual boost::any visit (Node::Value& node);
	virtual boost::any visit (Node::Variable& node);
	virtual boost::any visit (Node::Function& node);
	virtual boost::any visit (Node::Power& node);
	virtual boost::any visit (Node::AdditionSubtraction& node);
	virtual boost::any visit (Node::MultiplicationDivision& node);
};

} // namespace Visitor
