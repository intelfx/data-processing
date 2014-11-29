#include "node.h"

namespace Node
{

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

void Function::Dump (std::ostream& str)
{
	str << "( " << name_;

	for (Base::Ptr& child: children_) {
		str << " ";
		child->Dump (str);
	}

	str << ")";
}

void Power::Dump (std::ostream& str)
{
	str << "( **";

	for (Base::Ptr& child: children_) {
		str << " ";
		child->Dump (str);
	}

	str << ")";
}

void AdditionSubtraction::Dump (std::ostream& str)
{
	str << "(";

	for (auto& child: children_) {
		str << " " << (child.tag.negated ? "-" : "+") << " ";
		child.node->Dump (str);
	}

	str << ")";
}

void MultiplicationDivision::Dump (std::ostream& str)
{
	str << "(";

	for (auto& child: children_) {
		str << " " << (child.tag.reciprocated ? "/" : "*") << " ";
		child.node->Dump (str);
	}

	str << ")";
}

} // namespace Node
