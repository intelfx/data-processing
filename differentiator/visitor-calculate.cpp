#include "visitor-calculate.h"

namespace Visitor {

boost::any Calculate::visit (const Node::Value& node)
{
	return node.value();
}

boost::any Calculate::visit (const Node::Variable& node)
{
	return node.value();
}

boost::any Calculate::visit (const Node::Function& node)
{
	typedef std::list<Node::TaggedChild<void>> Children;
	typedef std::function<boost::any(Base&, const Children&)> Calculator;

	static std::unordered_map<std::string, Calculator> calculators {
		{ "ln", [](Base& visitor, const Children& children) -> boost::any { return std::log (any_to_fp (children.front().node->accept (visitor))); } }
	};

	auto it = calculators.find (node.name());
	if (it != calculators.end()) {
		try {
			return it->second (*this, node.children());
		} catch (boost::bad_any_cast& e) {
			return boost::any();
		}
	} else {
		std::cerr << "Calculate warning: unknown function: '" << node.name() << "'";
		return boost::any();
	}
}

boost::any Calculate::visit (const Node::Power& node)
{
	boost::any base = node.get_base()->accept (*this),
	           exponent = node.get_exponent()->accept (*this);

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

boost::any Calculate::visit (const Node::AdditionSubtraction& node)
{
	rational_t result_r (0);
	data_t result_f (0);
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

boost::any Calculate::visit (const Node::MultiplicationDivision& node)
{
	rational_t result_r (1);
	data_t result_f (1);
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
