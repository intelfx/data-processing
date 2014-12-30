#include "visitor-simplify.h"

namespace {

/* holds a mapping from stripped nodes to their constants to aid constant folding */
typedef std::map<Node::TaggedChild<void>, rational_t> DecompositionMap;
typedef std::pair<Node::TaggedChild<void>, rational_t> StrippedNode;

StrippedNode power_strip_exponent (Node::Base::Ptr&& node, bool reciprocate);
Node::Base::Ptr power_add_exponent (StrippedNode&& node_data);

StrippedNode muldiv_strip_multiplier (Node::Base::Ptr&& node, bool negate);
Node::Base::Ptr muldiv_add_multiplier (StrippedNode&& node_data);

void generic_fold_single (DecompositionMap& result, StrippedNode&& term);

void muldiv_decompose_and_fold_nested (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::MultiplicationDivision& node, bool node_reciprocation);
void muldiv_decompose_and_fold_nested_single (rational_t& result_value, DecompositionMap& result, Node::Base::Ptr&& node, bool node_reciprocation);
void muldiv_decompose_into_common_denominator (DecompositionMap& result, Node::Base::Ptr&& node, bool node_reciprocation);
void muldiv_decompose_no_fold (DecompositionMap& result, Node::Base::Ptr&& node, bool node_reciprocation);
Node::Base::Ptr muldiv_reconstruct (const rational_t& value, DecompositionMap&& terms);

void addsub_decompose_and_fold_nested (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::AdditionSubtraction& node, bool node_negation);
void addsub_decompose_fold_nested_single (rational_t& result_value, DecompositionMap& result, Node::Base::Ptr&& node, bool node_negation);
void addsub_decompose_fold_nested_single_decomposed (rational_t& result_value, DecompositionMap& result, rational_t source_constant, DecompositionMap&& source); // source is muldiv-decomposed
Node::Base::Ptr addsub_reconstruct (const rational_t& value, DecompositionMap&& terms);
DecompositionMap addsub_multiply_by_common_denominator (rational_t& constant, DecompositionMap& fractions); // returns the computed common denominator

StrippedNode power_strip_exponent (Node::Base::Ptr&& node, bool reciprocate)
{
	StrippedNode result;

	Node::Power* node_power = dynamic_cast<Node::Power*> (node.get());
	if (node_power) {
		result.second = node_power->get_exponent_constant (true);
		result.first.node = node_power->decay_move (std::move (node));
	} else {
		result.second = rational_t (1);
		result.first.node = std::move (node);
	}

	if (reciprocate) {
		result.second = -result.second;
	}

	assert (result.second != 0);

	return result;
}

Node::Base::Ptr power_add_exponent (StrippedNode&& node_data)
{
	assert (node_data.second != 0);

	if (node_data.second == 1) {
		return std::move (node_data.first.node);
	} else {
		Node::Power* node_power = dynamic_cast<Node::Power*> (node_data.first.node.get());
		if (node_power) {
			node_power->insert_exponent_constant (node_data.second);
			return std::move (node_data.first.node);
		} else {
			Node::Power::Ptr result_power (new Node::Power);
			result_power->set_base (std::move (node_data.first.node));
			result_power->insert_exponent_constant (node_data.second);
			return std::move (result_power);
		}
	}
}

StrippedNode muldiv_strip_multiplier (Node::Base::Ptr&& node, bool negate)
{
	StrippedNode result;

	Node::MultiplicationDivision* node_muldiv = dynamic_cast<Node::MultiplicationDivision*> (node.get());
	if (node_muldiv) {
		result.second = node_muldiv->get_constant (true);
		result.first.node = node_muldiv->decay_move (std::move (node));
	} else {
		result.second = rational_t (1);
		result.first.node = std::move (node);
	}

	if (negate) {
		result.second = -result.second;
	}

	assert (result.second != 0);

	return result;
}

Node::Base::Ptr muldiv_add_multiplier (StrippedNode&& node_data)
{
	assert (node_data.second != 0);

	if (node_data.second == 1) {
		return std::move (node_data.first.node);
	} else {
		Node::MultiplicationDivision* node_muldiv = dynamic_cast<Node::MultiplicationDivision*> (node_data.first.node.get());
		if (node_muldiv) {
			node_muldiv->insert_constant (node_data.second);
			return std::move (node_data.first.node);
		} else {
			Node::MultiplicationDivision::Ptr result_muldiv (new Node::MultiplicationDivision);
			result_muldiv->insert_constant (node_data.second);
			result_muldiv->add_child (std::move (node_data.first.node), false);
			return std::move (result_muldiv);
		}
	}
}

void generic_fold_single (DecompositionMap& result, StrippedNode&& term)
{
	auto r = result.emplace (std::move (term.first), term.second);
	if (!r.second) {
		/* fold -- sum up exponents/multipliers */
		r.first->second += term.second;

		/* if exponent/multiplier is 0, erase the child */
		if (r.first->second == 0) {
			result.erase (r.first);
		}
	}
}

void muldiv_decompose_and_fold_nested (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::MultiplicationDivision& node, bool node_reciprocation)
{
	for (auto& child: node.children()) {
		Node::Base::Ptr simplified (child.node->accept_ptr (visitor));
		bool reciprocation = child.tag.reciprocated ^ node_reciprocation;
		Node::Value* simplified_value = dynamic_cast<Node::Value*> (simplified.get());
		Node::MultiplicationDivision* simplified_muldiv = dynamic_cast<Node::MultiplicationDivision*> (simplified.get());

		if (simplified_value) {
			if (reciprocation) {
				result_value /= simplified_value->value();
			} else {
				result_value *= simplified_value->value();
			}
		} else if (simplified_muldiv) {
			muldiv_decompose_and_fold_nested (visitor, result_value, result, *simplified_muldiv, reciprocation);
		} else {
			/* insert child into the destination map, attempting folding */
			StrippedNode term = power_strip_exponent (std::move (simplified), reciprocation);
			generic_fold_single (result, std::move (term));
		}

		/* shortcut: if constant somehow turns to 0, then just return. */
		if (result_value == 0) {
			return;
		}
	}
}

void muldiv_decompose_and_fold_nested_single (rational_t& result_value, DecompositionMap& result, Node::Base::Ptr&& node, bool node_reciprocation)
{
	Node::Value* node_value = dynamic_cast<Node::Value*> (node.get());
	Node::MultiplicationDivision* node_muldiv = dynamic_cast<Node::MultiplicationDivision*> (node.get());

	if (node_value) {
		if (node_reciprocation) {
			result_value /= node_value->value();
		} else {
			result_value *= node_value->value();
		}
	} else if (node_muldiv) {
		for (auto it = node_muldiv->children().begin(); it != node_muldiv->children().end(); ) {
			auto child = take_set (node_muldiv->children(), it++);
			muldiv_decompose_and_fold_nested_single (result_value, result, std::move (child.node), child.tag.reciprocated ^ node_reciprocation);
		}
	} else {
		/* insert child into the destination map, attempting folding */
		StrippedNode term = power_strip_exponent (std::move (node), node_reciprocation);
		generic_fold_single (result, std::move (term));
	}
}

void muldiv_decompose_into_common_denominator (DecompositionMap& result, Node::Base::Ptr&& node, bool node_reciprocation)
{
	assert (!dynamic_cast<Node::MultiplicationDivision*> (node.get()));
	assert (!dynamic_cast<Node::Value*> (node.get()));

	StrippedNode term = power_strip_exponent (std::move (node), node_reciprocation);
	if (term.second < 0) {
		auto r = result.emplace (std::move (term.first), term.second);
		if (!r.second) {
			/* term already exists, pick the largest (absolute value) exponent */
			assert (r.first->second < 0);
			r.first->second = std::min (r.first->second, term.second);
		}
	}
}

void muldiv_decompose_no_fold (DecompositionMap& result, Node::Base::Ptr&& node, bool node_reciprocation)
{
	assert (!dynamic_cast<Node::MultiplicationDivision*> (node.get()));
	assert (!dynamic_cast<Node::Value*> (node.get()));

	StrippedNode term = power_strip_exponent (std::move (node), node_reciprocation);
	auto r = result.emplace (std::move (term.first), term.second);
	assert (r.second);
}

Node::Base::Ptr muldiv_reconstruct (const rational_t& value, DecompositionMap&& terms)
{
	if ((value != 0) &&
	    !terms.empty()) {
		/* we have something to multiply: at least one non-constant node and a non-zero constant */

		/* if there is "neutral" constant and only one child node, return it directly.
		 * in this mode we push exponent sign into child, even if this leads to creation of a Power node */
		if ((value == 1) &
		    (std::next (terms.begin()) == terms.end())) {
			StrippedNode term = take_map (terms, terms.begin());
			return power_add_exponent (std::move (term));
		}

		/* otherwise we create a full-fledged MultiplicationDivision node. */
		Node::MultiplicationDivision::Ptr result (new Node::MultiplicationDivision);

		/* add the constant, if needed */
		if (value != 1) {
			result->add_child (Node::Base::Ptr (new Node::Value (value)), false);
		}

		/* add remaining child nodes.
		 * in this mode we extract sign from child, this allows to elide a Power if constant == -1 */
		for (auto it = terms.begin(); it != terms.end(); ) {
			StrippedNode term = take_map (terms, it++);
			if (term.second < 0) {
				term.second = -term.second;
				result->add_child (power_add_exponent (std::move (term)), true);
			} else {
				result->add_child (power_add_exponent (std::move (term)), false);
			}
		}

		return std::move (result);
	} else {
		/* we do not have any nodes besides the constant, return it directly */
		return Node::Base::Ptr (new Node::Value (value));
	}
}

void addsub_decompose_and_fold_nested (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::AdditionSubtraction& node, bool node_negation)
{
	for (const auto& child: node.children()) {
		bool negation = child.tag.negated ^ node_negation;
		Node::Base::Ptr simplified (child.node->accept_ptr (visitor));
		Node::Value* simplified_value = dynamic_cast<Node::Value*> (simplified.get());
		Node::AdditionSubtraction* simplified_addsub = dynamic_cast<Node::AdditionSubtraction*> (simplified.get());

		if (simplified_value) {
			if (negation) {
				result_value -= simplified_value->value();
			} else {
				result_value += simplified_value->value();
			}
		} else if (simplified_addsub) {
			addsub_decompose_and_fold_nested (visitor, result_value, result, *simplified_addsub, negation);
		} else {
			/* insert child into the destination map, attempting folding */
			StrippedNode term = muldiv_strip_multiplier (std::move (simplified), negation);
			generic_fold_single (result, std::move (term));
		}
	}
}

void addsub_decompose_fold_nested_single (rational_t& result_value, DecompositionMap& result, Node::Base::Ptr&& node, bool node_negation)
{
	Node::Value* node_value = dynamic_cast<Node::Value*> (node.get());
	Node::AdditionSubtraction* node_addsub = dynamic_cast<Node::AdditionSubtraction*> (node.get());

	if (node_value) {
		if (node_negation) {
			result_value -= node_value->value();
		} else {
			result_value += node_value->value();
		}
	} else if (node_addsub) {
		for (auto it = node_addsub->children().begin(); it != node_addsub->children().end(); ) {
			auto child = take_set (node_addsub->children(), it++);
			addsub_decompose_fold_nested_single (result_value, result, std::move (child.node), child.tag.negated ^ node_negation);
		}
	} else {
		/* insert child into the destination map, attempting folding */
		StrippedNode term = muldiv_strip_multiplier (std::move (node), node_negation);
		generic_fold_single (result, std::move (term));
	}
}

void addsub_decompose_fold_nested_single_decomposed (rational_t& result_value, DecompositionMap& result, rational_t source_constant, DecompositionMap&& source)
{
	if (source.empty()) {
		result_value += source_constant;
	} else if (abs (source_constant) == 1) {
		/* reconstruct node (as muldiv) and attempt to decompose it (into addsub) -- may succeed if there is only one term */
		Node::Base::Ptr node = muldiv_reconstruct (rational_t (1), std::move (source));
		bool node_negation = (source_constant < 0);
		addsub_decompose_fold_nested_single (result_value, result, std::move (node), node_negation);
	} else {
		/* constant is non-1, so decomposition would never succeed, so take a shortcut instead of creating+deleting a muldiv node */
		StrippedNode term;
		term.first.node = muldiv_reconstruct (rational_t (1), std::move (source));
		term.second = source_constant;
		generic_fold_single (result, std::move (term));
	}
}

Node::Base::Ptr addsub_reconstruct (const rational_t& value, DecompositionMap&& terms)
{
	if (!terms.empty()) {
		/* we have something to add up: at least one non-constant node */

		/* if there is "neutral" constant and only one child node, return it directly.
		 * in this mode we push sign into child, even if this leads to creation of a MultiplicationDivision node */
		if ((value == 0) &
		    (std::next (terms.begin()) == terms.end())) {
			StrippedNode term = take_map (terms, terms.begin());
			return muldiv_add_multiplier (std::move (term));
		}

		/* otherwise we create a full-fledged AdditionSubtraction node. */
		Node::AdditionSubtraction::Ptr result (new Node::AdditionSubtraction);

		/* add the constant, if needed */
		if (value != 0) {
			result->add_child (Node::Base::Ptr (new Node::Value (value)), false);
		}

		/* add remaining child nodes.
		 * in this mode we extract sign from child, this allows to elide a MultiplicationDivision if constant == -1 */
		for (auto it = terms.begin(); it != terms.end(); ) {
			StrippedNode term = take_map (terms, it++);
			if (term.second < 0) {
				term.second = -term.second;
				result->add_child (muldiv_add_multiplier (std::move (term)), true);
			} else {
				result->add_child (muldiv_add_multiplier (std::move (term)), false);
			}
		}

		return std::move (result);
	} else {
		/* we do not have any nodes besides the constant, return it directly */
		return Node::Base::Ptr (new Node::Value (value));
	}
}

void muldiv_divide_by_decomposed (DecompositionMap& target_terms, const DecompositionMap& divisor_terms)
{
	for (const auto& term: divisor_terms) {
		auto it = target_terms.find (term.first);
		if (it == target_terms.end()) {
			/* term does not exist in the fraction */
			target_terms.emplace (term.first.clone(), -term.second);
		} else {
			it->second -= term.second;
			if (it->second == 0) {
				target_terms.erase (it);
			}
		}
	}
}

DecompositionMap addsub_multiply_by_common_denominator (rational_t& constant, DecompositionMap& fractions)
{
	/* product: term -> exponent (all exponents < 0) */
	DecompositionMap denominator_terms;

	/* compute the common denominator of all fractions */
	for (auto it = fractions.begin(); it != fractions.end(); ++it) {
		const Node::MultiplicationDivision* fraction_muldiv = dynamic_cast<const Node::MultiplicationDivision*> (it->first.node.get());
		const Node::Power* fraction_power = dynamic_cast<const Node::Power*> (it->first.node.get());

		if (fraction_muldiv) {
			for (const auto& child: fraction_muldiv->children()) {
				if (child.tag.reciprocated) {
					/* candidate for common denominator */
					muldiv_decompose_into_common_denominator (denominator_terms, child.node->clone(), true);
				}
			}
		} else if (fraction_power) {
			/* candidate for common denominator */
			muldiv_decompose_into_common_denominator (denominator_terms, it->first.node->clone(), false);
		}
	}

	/* if common denominator is empty, then we have nothing to do */
	if (denominator_terms.empty()) {
		return denominator_terms;
	}

	/* multiply all fractions by the common denominator
	 * we move the source map into a temporary and write terms back to the source map */

	/* sum: term -> multiplier */
	std::vector<StrippedNode> fractions_source;
	fractions_source.reserve (fractions.size() + 1);
	for (auto it = fractions.begin(); it != fractions.end(); ) {
		StrippedNode fraction = take_map (fractions, it++);
		fractions_source.push_back (std::move (fraction));
	}
	fractions.clear();

	/* insert the constant as an empty fraction */
	if (constant != 0) {
		fractions_source.emplace_back (Node::TaggedChild<void>(), constant);
		constant = 0;
	}

	for (StrippedNode& fraction: fractions_source) {
		/* product: term -> exponent */
		DecompositionMap fraction_terms;

		/* decompose current fraction */
		if (fraction.first.node) {
			Node::MultiplicationDivision* fraction_muldiv = dynamic_cast<Node::MultiplicationDivision*> (fraction.first.node.get());

			if (fraction_muldiv) {
				for (auto it = fraction_muldiv->children().begin(); it != fraction_muldiv->children().end(); ) {
					auto child = take_set (fraction_muldiv->children(), it++);
					muldiv_decompose_no_fold (fraction_terms, std::move (child.node), child.tag.reciprocated);
				}
			} else {
				muldiv_decompose_no_fold (fraction_terms, std::move (fraction.first.node), false);
			}
		}

		/* divide decomposed fraction by common denominator */
		muldiv_divide_by_decomposed (fraction_terms, denominator_terms);

		/* store the modified fraction */
		addsub_decompose_fold_nested_single_decomposed (constant, fractions, fraction.second, std::move (fraction_terms));
	}

	return denominator_terms;
}

} // anonymous namespace

namespace Visitor {

Simplify::Simplify (const std::string& variable)
: simplification_variable_ (variable)
{
}

Simplify::Simplify()
{
}

boost::any Simplify::visit (const Node::Value& node)
{
	return static_cast<Node::Base*> (node.clone().release());
}

boost::any Simplify::visit (const Node::Variable& node)
{
	// we substitute if both 1) variable is eligible for substitution and 2) it holds a rational value
	if (!simplification_variable_.empty() && !node.is_target_variable (simplification_variable_)) {
		boost::any value = node.value();
		if (any_isa<rational_t> (value)) {
			return static_cast<Node::Base*> (new Node::Value (any_to_rational (value)));
		}
	}

	return static_cast<Node::Base*> (node.clone().release());
}

boost::any Simplify::visit (const Node::Function& node)
{
	std::ostringstream reason;
	reason << "Simplify error: unknown function: '" << node.name() << "'";
	throw std::runtime_error (reason.str());
}

boost::any Simplify::visit (const Node::Power& node)
{
	Node::Base::Ptr base (node.get_base()->accept_ptr (*this)),
	                exponent (node.get_exponent()->accept_ptr (*this));

	Node::Value *base_value = dynamic_cast<Node::Value*> (base.get()),
	            *exponent_value = dynamic_cast<Node::Value*> (exponent.get());
	Node::MultiplicationDivision* base_muldiv = dynamic_cast<Node::MultiplicationDivision*> (base.get());
	Node::Power* base_power = dynamic_cast<Node::Power*> (base.get());

	if (base_value && exponent_value && (exponent_value->value().denominator() == 1)) {
		return static_cast<Node::Base*> (new Node::Value (pow_frac (base_value->value(), exponent_value->value().numerator())));
	} else if (base_value && (base_value->value() == 0)) {
		return static_cast<Node::Base*> (new Node::Value (0));
	} else if (base_value && (base_value->value() == 1)) {
		return static_cast<Node::Base*> (new Node::Value (1));
	} else if (exponent_value && (exponent_value->value() == 0)) {
		return static_cast<Node::Base*> (new Node::Value (1));
	} else if (exponent_value && (exponent_value->value() == 1)) {
		return static_cast<Node::Base*> (base.release());
	} else if (base_muldiv) {
		Node::MultiplicationDivision::Ptr result (new Node::MultiplicationDivision);

		for (auto it = base_muldiv->children().begin(); it != base_muldiv->children().end(); ) {
			auto base_term = take_set (base_muldiv->children(), it++);

			Node::Power::Ptr base_term_pwr (new Node::Power);
			base_term_pwr->set_base (std::move (base_term.node));
			base_term_pwr->set_exponent (exponent->clone());
			result->add_child (std::move (base_term_pwr), base_term.tag.reciprocated);
		}

		return result->accept_ptr (*this).release();
	} else if (base_power) {
		Node::Base::Ptr base_base (std::move (base_power->get_base())),
		                base_exponent (std::move (base_power->get_exponent()));

		Node::MultiplicationDivision::Ptr result_exponent (new Node::MultiplicationDivision);
		result_exponent->add_child (std::move (base_exponent), false);
		result_exponent->add_child (std::move (exponent), false);

		Node::Power::Ptr result (new Node::Power);
		result->set_base (std::move (base_base));
		result->set_exponent (std::move (result_exponent));
		return result->accept_ptr (*this).release();
	} else {
		Node::Power::Ptr result (new Node::Power);
		result->set_base (std::move (base));
		result->set_exponent (std::move (exponent));
		return static_cast<Node::Base*> (result.release());
	}
}

boost::any Simplify::visit (const Node::MultiplicationDivision& node)
{
	rational_t result_value (1);

	/* product: term -> exponent */
	DecompositionMap node_terms;

	muldiv_decompose_and_fold_nested (*this, result_value, node_terms, node, false);

	return muldiv_reconstruct (result_value, std::move (node_terms)).release();
}

boost::any Simplify::visit (const Node::AdditionSubtraction& node)
{
	rational_t result_value (0);

	/* sum: term -> multiplier */
	DecompositionMap node_terms;

	addsub_decompose_and_fold_nested (*this, result_value, node_terms, node, false);

	DecompositionMap denominator = addsub_multiply_by_common_denominator (result_value, node_terms);

	if (denominator.empty()) {
		return addsub_reconstruct (result_value, std::move (node_terms)).release();
	} else {
		/* do that once again to get rid of nested fractions */
		for (;;) {
			DecompositionMap next_denominator = addsub_multiply_by_common_denominator (result_value, node_terms);
			if (next_denominator.empty()) {
				break;
			}
			/* terms of denominator can never be constants or muldiv nodes */
			for (auto it = next_denominator.begin(); it != next_denominator.end(); ) {
				auto term = take_map (next_denominator, it++);
				generic_fold_single (denominator, std::move (term));
			}
		}

		/* build numerator */
		Node::Base::Ptr numerator = addsub_reconstruct (result_value, std::move (node_terms));

		/* now result_value is the multiplier */
		result_value = rational_t (1);

		/* numerator can be a constant or a muldiv, so fold it properly */
		muldiv_decompose_and_fold_nested_single (result_value, denominator, std::move (numerator), false);

		return muldiv_reconstruct (result_value, std::move (denominator)).release();
	}
}

} // namespace Visitor
