#include "node.h"
#include "visitor.h"

namespace Node
{

Base::~Base() = default;

void Base::add_child (Ptr&& node)
{
	children_.push_back (std::move (node));
}

Value::Value (data_t value)
: value_ (value)
{
}

Variable::Variable (const std::string& name, const ::Variable& variable)
: name_ (name)
, variable_ (variable)
{
}

Power::Power (Base::Ptr&& base, Base::Ptr&& exponent)
{
	add_child (std::move (base));
	add_child (std::move (exponent));
}

void AdditionSubtraction::add_child (Base::Ptr&& node, bool negated)
{
	Base::add_child (std::move (node));
	negation_.push_back (negated);
}

void MultiplicationDivision::add_child (Base::Ptr&& node, bool reciprocated)
{
	Base::add_child (std::move (node));
	reciprocation_.push_back (reciprocated);
}

IMPLEMENT_ACCEPTOR (Value);
IMPLEMENT_ACCEPTOR (Variable);
IMPLEMENT_ACCEPTOR (Power);
IMPLEMENT_ACCEPTOR (AdditionSubtraction);
IMPLEMENT_ACCEPTOR (MultiplicationDivision);

} // namespace Node
