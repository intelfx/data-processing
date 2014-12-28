#include "node.h"

namespace Node
{

void Value::Dump (std::ostream& str) const
{
	str << value_;
}

void Variable::Dump (std::ostream& str) const
{
	str << name_;

	if (!variable_.value.empty()) {
		str << " [";

		any_to_ostream (str, variable_.value);

		if (!variable_.no_error()) {
			str << " ± ";
			any_to_ostream (str, variable_.error);
		}

		str << "]";
	}
}

void Function::Dump (std::ostream& str) const
{
	str << "( " << name_;

	for (const auto& child: children_) {
		str << " ";
		child.node->Dump (str);
	}

	str << ")";
}

void Power::Dump (std::ostream& str) const
{
	str << "( ** ";
	base_->Dump (str);
	str << " ";
	exponent_->Dump (str);
	str << " )";
}

void AdditionSubtraction::Dump (std::ostream& str) const
{
	str << "(";

	for (auto& child: children_) {
		str << " " << (child.tag.negated ? "-" : "+") << " ";
		child.node->Dump (str);
	}

	str << ")";
}

void MultiplicationDivision::Dump (std::ostream& str) const
{
	str << "(";

	for (auto& child: children_) {
		str << " " << (child.tag.reciprocated ? "/" : "*") << " ";
		child.node->Dump (str);
	}

	str << ")";
}

} // namespace Node
