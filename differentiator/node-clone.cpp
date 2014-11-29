#include "node.h"

namespace {

template <typename T>
Node::Base::Ptr add_children_and_return (Node::TaggedChildList<T>* created_node, const Node::TaggedChildList<T>* src)
{
	created_node->add_children_from (*src);
	return Node::Base::Ptr (created_node);
}

} // anonymous namespace

namespace Node
{

Base::Ptr Value::clone() const
{
	return Base::Ptr (new Value (value_));
}

Base::Ptr Variable::clone() const
{
	return Base::Ptr (new Variable (name_, variable_, is_error_));
}

Base::Ptr Function::clone() const
{
	return add_children_and_return (new Function (name_), this);
}

Base::Ptr Power::clone() const
{
	return add_children_and_return (new Power, this);
}

Base::Ptr AdditionSubtraction::clone() const
{
	return add_children_and_return (new AdditionSubtraction, this);
}

Base::Ptr MultiplicationDivision::clone() const
{
	return add_children_and_return (new MultiplicationDivision, this);
}

} // namespace Node
