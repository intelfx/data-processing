#include "node.h"

namespace Node
{

void Base::Dump (std::ostream& str)
{
	str << "( ERR )";
}

void Value::Dump (std::ostream& str)
{
	str << value_;
}

void Variable::Dump (std::ostream& str)
{
	str << name_ << " [" << variable_.value;

	if (!variable_.no_error()) {
		str << " ± " << variable_.error;
	}

	str << "]";
}

void Power::Dump (std::ostream& str)
{
	str << "(**";

	for (Base::Ptr& child: children_) {
		str << " ";
		child->Dump (str);
	}

	str << ")";
}

void AdditionSubtraction::Dump (std::ostream& str)
{
	str << "(";

	auto it = negation_.begin();
	for (Base::Ptr& child: children_) {
		str << " " << (*it ? "-" : "+") << " ";
		child->Dump (str);
		++it;
	}

	str << ")";
}

void MultiplicationDivision::Dump (std::ostream& str)
{
	str << "(";

	auto it = reciprocation_.begin();
	for (Base::Ptr& child: children_) {
		str << " " << (*it ? "/" : "*") << " ";
		child->Dump (str);
		++it;
	}

	str << ")";
}

} // namespace Node
