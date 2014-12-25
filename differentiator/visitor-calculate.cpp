#include "visitor-calculate.h"

namespace Visitor {

boost::any Calculate::visit (Node::Value& node)
{
	return node.value();
}

boost::any Calculate::visit (Node::Variable& node)
{
	return node.value();
}

boost::any Calculate::visit (Node::Function& node)
{
	std::ostringstream reason;
	reason << "Calculate error: unknown function: '" << node.name() << "'";
	throw std::runtime_error (reason.str());
}

boost::any Calculate::visit (Node::Power& node)
{
	if (node.children().size() != 2) {
		std::ostringstream reason;
		reason << "Calculate error: power node has " << node.children().size() << " children";
		throw std::logic_error (reason.str());
	}

	auto child = node.children().begin();

	boost::any base = (*child++)->accept (*this),
	           exponent = (*child++)->accept (*this);

	if (base.empty() || exponent.empty()) {
		return boost::any();
	} else if (any_isa<rational_t> (base) &&
	           any_isa<rational_t> (exponent)) {
		rational_t exponent_r = any_to_rational (exponent);

		// only attempt rational calculations if we do not need to take roots
		if (exponent_r.denominator() == 1) {
			return pow_frac (any_to_rational (base), exponent_r.numerator());
		}
		// otherwise fall through to real-number calculations
	}

	return powl (any_to_fp (base), any_to_fp (exponent));
}

boost::any Calculate::visit (Node::AdditionSubtraction& node)
{
	rational_t result_r (0);
	data_t result_f;
	bool is_rational = true;

	for (auto& child: node.children()) {
		boost::any next = child.node->accept (*this);

		if (next.empty()) {
			return boost::any();
		} else if (is_rational && any_isa<rational_t> (next)) {
			if (child.tag.negated) {
				result_r -= any_to_rational (next);
			} else {
				result_r += any_to_rational (next);
			}
		} else {
			if (is_rational) {
				result_f = to_fp (result_r);
				is_rational = false;
			}

			if (child.tag.negated) {
				result_f -= any_to_fp (next);
			} else {
				result_f += any_to_fp (next);
			}
		}
	}

	return is_rational ? boost::any (result_r)
	                   : boost::any (result_f);
}

boost::any Calculate::visit (Node::MultiplicationDivision& node)
{
	rational_t result_r (1);
	data_t result_f;
	bool is_rational = true;

	for (auto& child: node.children()) {
		boost::any next = child.node->accept (*this);

		if (next.empty()) {
			return boost::any();
		} else if (is_rational && any_isa<rational_t> (next)) {
			if (child.tag.reciprocated) {
				result_r /= any_to_rational (next);
			} else {
				result_r *= any_to_rational (next);
			}
		} else {
			if (is_rational) {
				result_f = to_fp (result_r);
				is_rational = false;
			}

			if (child.tag.reciprocated) {
				result_f /= any_to_fp (next);
			} else {
				result_f *= any_to_fp (next);
			}
		}
	}

	return is_rational ? boost::any (result_r)
	                   : boost::any (result_f);
}

} // namespace Visitor
