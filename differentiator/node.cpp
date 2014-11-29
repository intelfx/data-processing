#include "node.h"
#include "visitor.h"

namespace Node
{

Base::~Base() = default;


void TaggedChildList<void>::add_child (Base::Ptr&& node)
{
	children_.push_back (std::move (node));
}

void TaggedChildList<void>::add_child_front (Base::Ptr&& node)
{
	children_.push_front (std::move (node));
}

void TaggedChildList<void>::add_children_from (const Node::TaggedChildList<void>& rhs)
{
	for (const Base::Ptr& child: rhs.children_) {
		children_.push_back (child->clone());
	}
}

Value::Value (data_t value)
: value_ (value)
{
}

Variable::Variable (const std::string& name, const ::Variable& variable, bool is_error)
: name_ (name)
, variable_ (variable)
, is_error_ (is_error)
{
}

Function::Function (const std::string& name)
: name_ (name)
{
}

Value* MultiplicationDivision::get_constant()
{
	Value* constant = nullptr;

	if (!children_.empty()) {
		constant = dynamic_cast<Value*> (children_.front().node.get());
	}

	if (!constant) {
		constant = new Value (1);
		add_child_front (Base::Ptr (constant), false);
	}

	return constant;
}

IMPLEMENT_ACCEPTOR (Value);
IMPLEMENT_ACCEPTOR (Variable);
IMPLEMENT_ACCEPTOR (Function);
IMPLEMENT_ACCEPTOR (Power);
IMPLEMENT_ACCEPTOR (AdditionSubtraction);
IMPLEMENT_ACCEPTOR (MultiplicationDivision);

} // namespace Node
