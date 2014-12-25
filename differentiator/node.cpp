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

std::pair<Value*, MultiplicationDivisionTag*> MultiplicationDivision::get_constant()
{
	std::pair<Value*, MultiplicationDivisionTag*> result (nullptr, nullptr);

	if (!children_.empty() &&
	    (result.first = dynamic_cast<Value*> (children_.front().node.get()))) {
		result.second = &children_.front().tag;
	}

	return result;
}

void MultiplicationDivision::release_constant()
{
	children_.pop_front();
}

void MultiplicationDivision::release_constant_if_exists()
{
	if (!children_.empty() &&
	    (dynamic_cast<Value*> (children_.front().node.get()))) {
		release_constant();
	}
}

std::pair<Value*, MultiplicationDivisionTag*> MultiplicationDivision::create_constant()
{
	std::pair<Value*, MultiplicationDivisionTag*> result;

	result.first = new Value (1);
	add_child_front (Base::Ptr (result.first), false);
	result.second = &children_.front().tag;

	return result;
}

std::pair<Value*, MultiplicationDivisionTag*> MultiplicationDivision::get_or_create_constant()
{
	auto result = get_constant();

	if (result.first) {
		return result;
	} else {
		return create_constant();
	}
}

rational_t MultiplicationDivision::calculate_constant_value (const std::pair<Value*, MultiplicationDivisionTag*> constant)
{
	if (constant.second->reciprocated) {
		return 1 / constant.first->value();
	} else {
		return constant.first->value();
	}
}

rational_t MultiplicationDivision::get_constant_value()
{
	auto constant = get_constant();

	if (constant.first) {
		return calculate_constant_value (constant);
	} else {
		return rational_t (1);
	}
}

rational_t MultiplicationDivision::get_constant_value_and_release()
{
	auto constant = get_constant();

	if (constant.first) {
		rational_t result = calculate_constant_value (constant);
		release_constant();
		return result;
	} else {
		return rational_t (1);
	}
}

void MultiplicationDivision::insert_constant (rational_t value)
{
	auto constant = get_constant();

	/* Multiply/divide by existing constant (if it exists) */
	if (constant.first) {
		if (constant.second->reciprocated) {
			value /= constant.first->value();
		} else {
			value *= constant.first->value();
		}
	}

	if (value != 1) {
		/* Write the new constant (creating it if it doesn't exist) */
		if (!constant.first) {
			constant = create_constant();
		}
		constant.first->set_value (value);
		constant.second->reciprocated = false;
	} else {
		/* Release the existing constant (== write 1) */
		if (constant.first) {
			release_constant();
		}
	}
}

IMPLEMENT_ACCEPTOR (Value);
IMPLEMENT_ACCEPTOR (Variable);
IMPLEMENT_ACCEPTOR (Function);
IMPLEMENT_ACCEPTOR (Power);
IMPLEMENT_ACCEPTOR (AdditionSubtraction);
IMPLEMENT_ACCEPTOR (MultiplicationDivision);

} // namespace Node
