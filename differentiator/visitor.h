#pragma once

#include <util.h>
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

namespace Util
{

inline Node::Base::Ptr apply_transformation (Node::Base::Ptr& tree, Visitor::Base&& modifier)
{
	return Node::Base::Ptr (boost::any_cast<Node::Base*> (tree->accept (modifier)));
}

} // namespace Util

} // namespace Visitor
