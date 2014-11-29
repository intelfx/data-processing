#include "node.h"

namespace {

template <typename Tag>
bool compare_children (const Node::TaggedChildList<Tag>& _1, const Node::TaggedChildList<Tag>& _2)
{
	if (_1.children().size() != _2.children().size()) {
		return false;
	}

	std::vector<bool> matched_children (_1.children().size());

	for (auto& child_1: _1.children()) {
		bool ok = false;
		int cnt = 0;

		for (auto& child_2: _2.children()) {
			if (!matched_children.at (cnt) &&
				Node::TaggedChildList<Tag>::compare_child (child_1, child_2)) {
				ok = true;
				break;
			}
			++cnt;
		}

		if (ok) {
			matched_children.at (cnt) = true;
		} else {
			return false;
		}
	}

	return true;
}

template <typename Tag>
bool compare_children_ordered (const Node::TaggedChildList<Tag>& _1, const Node::TaggedChildList<Tag>& _2)
{
	if (_1.children().size() != _2.children().size()) {
		return false;
	}

	auto child_1 = _1.children().begin(),
	     child_2 = _2.children().begin();
	while (child_1 != _1.children().end()) {
		if (!Node::TaggedChildList<Tag>::compare_child (*child_1++, *child_2++)) {
			return false;
		}
	}

	return true;
}

} // anonymous namespace

namespace Node
{

#define COMPARE_CHECK_TYPE(Type)                              \
	const Type* node = dynamic_cast<const Type*> (rhs.get()); \
	if (!node) {                                              \
		return false;                                         \
	}

bool Value::compare (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Value);

	return (value_ == node->value_);
}

bool Variable::compare (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Variable);

	return (name_ == node->name_) &&
	       (is_error_ == node->is_error_);
}

bool Function::compare (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Function);

	return (name_ == node->name_) &&
	       compare_children_ordered (*this, *node);
}

bool Power::compare (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(Power);

	return compare_children_ordered (*this, *node);
}

bool AdditionSubtraction::compare (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(AdditionSubtraction);

	return compare_children (*this, *node);
}

bool MultiplicationDivision::compare (const Base::Ptr& rhs) const
{
	COMPARE_CHECK_TYPE(MultiplicationDivision);

	return compare_children (*this, *node);
}

} // namespace Node
