#pragma once

#include "visitor.h"

namespace Visitor {

class Optimize : public Base
{
	static Node::Base::Ptr multiply_nodes (std::vector<Node::Base::Ptr>& vec);

public:
	virtual boost::any visit (Node::Value& node);
	virtual boost::any visit (Node::Variable& node);
	virtual boost::any visit (Node::Function& node);
	virtual boost::any visit (Node::Power& node);
	virtual boost::any visit (Node::AdditionSubtraction& node);
	virtual boost::any visit (Node::MultiplicationDivision& node);
};

} // namespace Visitor
