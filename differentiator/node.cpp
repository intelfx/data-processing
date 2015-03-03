#include "node.h"
#include "visitor.h"

namespace Node
{

std::ostream& operator<< (std::ostream& out, const Node::Base& node)
{
	node.Dump (out);
	return out;
}

Base::~Base() = default;

Value::Value (rational_t value)
: value_ (value)
{
}

Value::Value (integer_t value)
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

rational_t Power::get_exponent_constant (bool release)
{
	rational_t result;
	Node::MultiplicationDivision* exponent_muldiv = dynamic_cast<Node::MultiplicationDivision*> (exponent_.get());
	Node::Value* exponent_value = dynamic_cast<Node::Value*> (exponent_.get());

	if (exponent_muldiv) {
		result = exponent_muldiv->get_constant (release);
		exponent_muldiv->decay_assign (exponent_);
	} else if (exponent_value) {
		result = exponent_value->value();
		if (release) {
			exponent_value->set_value (rational_t (1));
		}
	} else {
		result = rational_t (1);
	}

	return result;
}

void Power::insert_exponent_constant (rational_t value)
{
	Node::MultiplicationDivision* exponent_muldiv = dynamic_cast<Node::MultiplicationDivision*> (exponent_.get());
	Node::Value* exponent_value = dynamic_cast<Node::Value*> (exponent_.get());

	if (exponent_muldiv) {
		exponent_muldiv->insert_constant (value);
	} else if (exponent_value) {
		exponent_value->set_value (exponent_value->value() * value);
	} else if (exponent_) {
		Node::MultiplicationDivision::Ptr new_exponent_muldiv (new Node::MultiplicationDivision);
		new_exponent_muldiv->insert_constant (value);
		new_exponent_muldiv->add_child (std::move (exponent_), false);
		exponent_ = std::move (new_exponent_muldiv);
	} else {
		exponent_ = Node::Base::Ptr (new Node::Value (value));
	}
}

Base::Ptr Power::decay_move (Base::Ptr&& self)
{
	Node::Value* exponent_value = dynamic_cast<Node::Value*> (exponent_.get());
	if (exponent_value &&
	    (exponent_value->value() == 1)) {
		return std::move (base_);
	} else {
		return std::move (self);
	}
}

void Power::decay_assign (Base::Ptr& dest)
{
	Node::Value* exponent_value = dynamic_cast<Node::Value*> (exponent_.get());
	if (exponent_value &&
	    (exponent_value->value() == 1)) {
		dest = std::move (base_);
	}
}

Base::Ptr AdditionSubtraction::decay_move (Base::Ptr&& self)
{
	auto it = children().begin();

	if ((std::next (it) == children().end()) &&
	    !it->tag.negated) {
		return take_set (children(), it).node;
	} else {
		return std::move (self);
	}
}

void AdditionSubtraction::decay_assign (Base::Ptr& dest)
{
	auto it = children().begin();

	if ((std::next (it) == children().end()) &&
	    !it->tag.negated) {
		dest = take_set (children(), it).node;
	}
}

rational_t MultiplicationDivision::get_constant (bool release)
{
	const Value* node = nullptr;
	rational_t value (1);

	if (!children_.empty()) {
		auto iter = children_.begin();

		node = dynamic_cast<const Value*> (iter->node.get());
		if (node) {
			if (iter->tag.reciprocated) {
				value = 1 / node->value();
			} else {
				value = node->value();
			}

			if (release) {
				children_.erase (iter);
			}
		}
	}

	return value;
}

void MultiplicationDivision::insert_constant (rational_t value)
{
	rational_t constant = get_constant (true) * value;

	if (constant != 1) {
		add_child (Value::Ptr (new Node::Value (constant)), false);
	}
}

Base::Ptr MultiplicationDivision::decay_move (Base::Ptr&& self)
{
	auto it = children().begin();

	if (it == children_.end()) {
		return Base::Ptr();
	} else if ((std::next (it) == children_.end()) &&
	           !it->tag.reciprocated) {
		return take_set (children_, it).node;
	} else {
		return std::move (self);
	}
}

void MultiplicationDivision::decay_assign (Base::Ptr& dest)
{
	auto it = children().begin();

	if (it == children_.end()) {
		dest.reset();
	} else if ((std::next (it) == children_.end()) &&
	           !it->tag.reciprocated) {
		dest = take_set (children(), it).node;
	}
}

IMPLEMENT (Value);
IMPLEMENT (Variable);
IMPLEMENT (Function);
IMPLEMENT (Power);
IMPLEMENT (AdditionSubtraction);
IMPLEMENT (MultiplicationDivision);

} // namespace Node
