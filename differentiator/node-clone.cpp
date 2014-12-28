#include "node.h"

namespace {

template <typename T>
Node::Base::Ptr add_children_and_return (T* created_node, const T* src)
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
	Node::Power::Ptr result (new Node::Power);
	result->set_base (base_->clone());
	result->set_exponent (exponent_->clone());
	return std::move (result);
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
