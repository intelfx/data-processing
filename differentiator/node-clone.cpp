#include "node.h"

namespace Node
{

Base::Ptr Base::clone() const
{
	Base::Ptr result = clone_bare();

	for (const Base::Ptr& child: children_) {
		result->add_child (child->clone());
	}

	return std::move (result);
}

Base::Ptr Value::clone_bare() const
{
	return Base::Ptr (new Value (value_));
}

Base::Ptr Variable::clone_bare() const
{
	return Base::Ptr (new Variable (name_, variable_));
}

Base::Ptr Function::clone_bare() const
{
	return Base::Ptr (new Function (name_));
}

Base::Ptr Power::clone_bare() const
{
	return Base::Ptr (new Power);
}

Base::Ptr AdditionSubtraction::clone_bare() const
{
	AdditionSubtraction::Ptr result (new AdditionSubtraction);
	result->negation_ = negation_;
	return std::move (result);
}

Base::Ptr MultiplicationDivision::clone_bare() const
{
	MultiplicationDivision::Ptr result (new MultiplicationDivision);
	result->reciprocation_ = reciprocation_;
	return std::move (result);
}

} // namespace Node
