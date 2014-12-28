#include "node.h"

namespace Node
{

bool Base::compare (const Base::Ptr& rhs) const
{
	if (typeid (*this) == typeid (*rhs.get())) {
		return compare_same_type (rhs);
	} else {
		return false;
	}
}

bool Base::less (const Base::Ptr& rhs) const
{
	if (typeid (*this) == typeid (*rhs.get())) {
		return less_same_type (rhs);
	} else {
		return get_type() < rhs->get_type();
	}
}

#define COMPARE_CHECK_TYPE(Type)                             \
	const Type* node = static_cast<const Type*> (rhs.get()); \

bool Value::compare_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Value);

	return (value_ == node->value_);
}

bool Variable::compare_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Variable);

	return (name_ == node->name_) &&
	       (is_error_ == node->is_error_);
}

bool Function::compare_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Function);

	return (name_ == node->name_) &&
	       (children_ == node->children_);
}

bool Power::compare_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Power);

	return base_->compare (node->base_) &&
	       exponent_->compare (node->exponent_);
}

bool AdditionSubtraction::compare_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(AdditionSubtraction);

	return children_ == node->children_;
}

bool MultiplicationDivision::compare_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(MultiplicationDivision);

	return children_ == node->children_;
}


#define LEXICOGRAPHICAL_COMPARE_CHAIN(less, equal) \
	if (less) return true;                         \
	else if (!(equal)) return false;

#define LEXICOGRAPHICAL_COMPARE_LAST(less)         \
	return (less);

bool Value::less_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Value);

	LEXICOGRAPHICAL_COMPARE_LAST(value_ < node->value_);
}

bool Variable::less_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Variable);

	LEXICOGRAPHICAL_COMPARE_CHAIN(is_error_ < node->is_error_,
	                              is_error_ == node->is_error_);
	LEXICOGRAPHICAL_COMPARE_LAST(name_ < node->name_);
}

bool Function::less_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Function);

	LEXICOGRAPHICAL_COMPARE_LAST(name_ < node->name_);
}

bool Power::less_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Power);

	LEXICOGRAPHICAL_COMPARE_CHAIN(exponent_->less (node->exponent_),
	                              exponent_->compare (node->exponent_));
	LEXICOGRAPHICAL_COMPARE_LAST(base_->less (node->base_));
}

bool AdditionSubtraction::less_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(AdditionSubtraction);

	LEXICOGRAPHICAL_COMPARE_LAST(children_ < node->children_);
}

bool MultiplicationDivision::less_same_type (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(MultiplicationDivision);

	LEXICOGRAPHICAL_COMPARE_LAST(children_ < node->children_);
}
} // namespace Node
