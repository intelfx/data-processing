#include "visitor-simplify.h"

namespace {

/* holds a mapping from stripped nodes to their constants to aid constant folding */
typedef std::map<Node::TaggedChild<void>, rational_t> DecompositionMap;
typedef std::pair<Node::TaggedChild<void>, rational_t> StrippedNode;

StrippedNode power_strip_exponent (Node::Base::Ptr&& node, rational_t node_exponent);
Node::Base::Ptr power_add_exponent (StrippedNode&& node_data);

StrippedNode muldiv_strip_multiplier (Node::Base::Ptr&& node, rational_t premultiplier);
Node::Base::Ptr muldiv_add_multiplier (StrippedNode&& node_data);

void generic_fold_single (DecompositionMap& result, StrippedNode&& term);

void muldiv_decompose_fold_nested_single (rational_t& result_value, DecompositionMap& result, Node::Base::Ptr&& node, rational_t node_exponent);
void muldiv_decompose_fold_nested_muldiv (rational_t& result_value, DecompositionMap& result, Node::MultiplicationDivision* node, rational_t node_exponent);
void muldiv_decompose_fold_nested_single_simplify (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::Base& node, rational_t node_exponent);
void muldiv_decompose_fold_nested_muldiv_simplify (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::MultiplicationDivision& node, rational_t node_exponent);

void muldiv_decompose_into_common_denominator (DecompositionMap& result, Node::Base::Ptr&& node, rational_t node_exponent);
void muldiv_decompose_no_fold (DecompositionMap& result, Node::Base::Ptr&& node, rational_t node_exponent);

Node::Base::Ptr muldiv_reconstruct (const rational_t& value, DecompositionMap&& terms);

void addsub_decompose_fold_nested_single (rational_t& result_value, DecompositionMap& result, Node::Base::Ptr&& node, rational_t node_multiplier);
void addsub_decompose_fold_nested_addsub (rational_t& result_value, DecompositionMap& result, Node::AdditionSubtraction* node, rational_t node_multiplier);
void addsub_decompose_fold_nested_muldiv_decomposed (rational_t& result_value, DecompositionMap& result, DecompositionMap&& source, rational_t source_constant); // source is muldiv-decomposed
void addsub_decompose_fold_nested_single_simplify (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::Base& node, rational_t node_multiplier);
void addsub_decompose_fold_nested_addsub_simplify (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::AdditionSubtraction& node, rational_t node_multiplier);

Node::Base::Ptr addsub_reconstruct (const rational_t& value, DecompositionMap&& terms);
DecompositionMap addsub_multiply_by_common_denominator (rational_t& constant, DecompositionMap& fractions); // returns the computed common denominator
Node::Base::Ptr addsub_reconstruct_common_multiplier (rational_t value, DecompositionMap&& terms);

StrippedNode power_strip_exponent (Node::Base::Ptr&& node, rational_t node_exponent)
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

	result.second *= node_exponent;

	ASSERT (result.second != 0, "Zero exponent encountered in tree");

	return result;
}

Node::Base::Ptr power_add_exponent (StrippedNode&& node_data)
{
	ASSERT (node_data.second != 0, "Zero exponent when building node");

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

StrippedNode muldiv_strip_multiplier (Node::Base::Ptr&& node, rational_t premultiplier)
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

	result.second *= premultiplier;

	ASSERT (result.second != 0, "Zero multiplier encountered in tree");

	return result;
}

Node::Base::Ptr muldiv_add_multiplier (StrippedNode&& node_data)
{
	ASSERT (node_data.second != 0, "Zero multiplier while building node");

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
	ASSERT (term.second != 0, "Zero multiplier/exponent when folding");

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

void muldiv_decompose_fold_nested_single_simplify (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::Base& node, rational_t node_exponent)
{
	const Node::Value* node_value = dynamic_cast<const Node::Value*> (&node);
	const Node::MultiplicationDivision* node_muldiv = dynamic_cast<const Node::MultiplicationDivision*> (&node);

	if (node_value && (node_exponent.denominator() == 1)) {
		result_value *= pow_frac (node_value->value(), node_exponent.numerator());
	} else if (node_muldiv) {
		/* this is an optimization to go without simplifying while we can go deeper */
		muldiv_decompose_fold_nested_muldiv_simplify (visitor, result_value, result, *node_muldiv, node_exponent);
	} else {
		/* here we actually call the simplifier (which duplicates the subtree) and pass control to the non-const version */
		muldiv_decompose_fold_nested_single (result_value, result, node.accept_ptr (visitor), node_exponent);
	}
}

void muldiv_decompose_fold_nested_muldiv_simplify (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::MultiplicationDivision& node, rational_t node_exponent)
{
	for (const auto& child: node.children()) {
		muldiv_decompose_fold_nested_single_simplify (visitor, result_value, result, *child.node, child.tag.reciprocated ? -node_exponent : node_exponent);
	}
}

void muldiv_decompose_fold_nested_single (rational_t& result_value, DecompositionMap& result, Node::Base::Ptr&& node, rational_t node_exponent)
{
	Node::Value* node_value = dynamic_cast<Node::Value*> (node.get());

	if (node_value && (node_exponent.denominator() == 1)) {
		result_value *= pow_frac (node_value->value(), node_exponent.numerator());
	} else {
		/* insert child into the destination map, attempting folding */
		StrippedNode term = power_strip_exponent (std::move (node), node_exponent);
		Node::MultiplicationDivision* term_muldiv = dynamic_cast<Node::MultiplicationDivision*> (term.first.node.get());

		if (term_muldiv) {
			muldiv_decompose_fold_nested_muldiv (result_value, result, term_muldiv, term.second);
		} else {
			generic_fold_single (result, std::move (term));
		}
	}
}

void muldiv_decompose_fold_nested_muldiv (rational_t& result_value, DecompositionMap& result, Node::MultiplicationDivision* node, rational_t node_exponent)
{
	for (auto it = node->children().begin(); it != node->children().end(); ) {
		auto child = take_set (node->children(), it++);
		muldiv_decompose_fold_nested_single (result_value, result, std::move (child.node), child.tag.reciprocated ? -node_exponent : node_exponent);
	}
}

void muldiv_decompose_into_common_denominator (DecompositionMap& result, Node::Base::Ptr&& node, rational_t node_exponent)
{
	ASSERT (!dynamic_cast<Node::MultiplicationDivision*> (node.get()), "MultiplicationDivision node encountered as a child of a MultiplicationDivision node");
	ASSERT (!dynamic_cast<Node::Value*> (node.get()), "Value node encountered as a child of a MultiplicationDivision node");

	StrippedNode term = power_strip_exponent (std::move (node), node_exponent);
	if (term.second < 0) {
		auto r = result.emplace (std::move (term.first), term.second);
		if (!r.second) {
			/* term already exists, pick the largest (absolute value) exponent */
			assert (r.first->second < 0);
			r.first->second = std::min (r.first->second, term.second);
		}
	}
}

void muldiv_decompose_no_fold (DecompositionMap& result, Node::Base::Ptr&& node, rational_t node_exponent)
{
	ASSERT (!dynamic_cast<Node::MultiplicationDivision*> (node.get()), "MultiplicationDivision node encountered as a child of a MultiplicationDivision node");
	ASSERT (!dynamic_cast<Node::Value*> (node.get()), "Value node encountered as a child of a MultiplicationDivision node");

	StrippedNode term = power_strip_exponent (std::move (node), node_exponent);
	auto r = result.emplace (std::move (term.first), term.second);
	ASSERT (r.second, "Term already exists in the decomposed map -- should be doing decomposition with folding instead");
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

void addsub_decompose_fold_nested_single (rational_t& result_value, DecompositionMap& result, Node::Base::Ptr&& node, rational_t node_multiplier)
{
	Node::Value* node_value = dynamic_cast<Node::Value*> (node.get());

	if (node_value) {
		result_value += node_value->value() * node_multiplier;
	} else {
		/* insert child into the destination map, attempting folding */
		StrippedNode term = muldiv_strip_multiplier (std::move (node), node_multiplier);
		Node::AdditionSubtraction* term_addsub = dynamic_cast<Node::AdditionSubtraction*> (term.first.node.get());

		if (term_addsub) {
			addsub_decompose_fold_nested_addsub (result_value, result, term_addsub, term.second);
		} else {
			generic_fold_single (result, std::move (term));
		}
	}
}

void addsub_decompose_fold_nested_addsub (rational_t& result_value, DecompositionMap& result, Node::AdditionSubtraction* node, rational_t node_multiplier)
{
	for (auto it = node->children().begin(); it != node->children().end(); ) {
		auto child = take_set (node->children(), it++);
		addsub_decompose_fold_nested_single (result_value, result, std::move (child.node), child.tag.negated ? -node_multiplier : node_multiplier);
	}
}

void addsub_decompose_fold_nested_single_simplify (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::Base& node, rational_t node_multiplier)
{
	const Node::Value* node_value = dynamic_cast<const Node::Value*> (&node);
	const Node::AdditionSubtraction* node_addsub = dynamic_cast<const Node::AdditionSubtraction*> (&node);

	if (node_value) {
		result_value += node_value->value() * node_multiplier;
	} else if (node_addsub) {
		/* this is an optimization to go without simplifying while we can go deeper */
		addsub_decompose_fold_nested_addsub_simplify (visitor, result_value, result, *node_addsub, node_multiplier);
	} else {
		/* here we actually call the simplifier (which duplicates the subtree) and pass control to the non-const version */
		addsub_decompose_fold_nested_single (result_value, result, node.accept_ptr (visitor), node_multiplier);
	}
}

void addsub_decompose_fold_nested_addsub_simplify (Visitor::Simplify& visitor, rational_t& result_value, DecompositionMap& result, const Node::AdditionSubtraction& node, rational_t node_multiplier)
{
	for (const auto& child: node.children()) {
		addsub_decompose_fold_nested_single_simplify (visitor, result_value, result, *child.node, child.tag.negated ? -node_multiplier : node_multiplier);
	}
}

void addsub_decompose_fold_nested_muldiv_decomposed (rational_t& result_value, DecompositionMap& result, DecompositionMap&& source, rational_t source_constant)
{
	if (source.empty()) {
		result_value += source_constant;
	} else {
		addsub_decompose_fold_nested_single (result_value, result, muldiv_reconstruct (rational_t (1), std::move (source)), source_constant);
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
					muldiv_decompose_into_common_denominator (denominator_terms, child.node->clone(), rational_t (-1));
				}
			}
		} else if (fraction_power) {
			/* candidate for common denominator */
			muldiv_decompose_into_common_denominator (denominator_terms, it->first.node->clone(), rational_t (1));
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
					muldiv_decompose_no_fold (fraction_terms, std::move (child.node), rational_t (child.tag.reciprocated ? -1 : 1));
				}
			} else {
				muldiv_decompose_no_fold (fraction_terms, std::move (fraction.first.node), rational_t (1));
			}
		}

		/* divide decomposed fraction by common denominator */
		muldiv_divide_by_decomposed (fraction_terms, denominator_terms);

		/* store the modified fraction */
		addsub_decompose_fold_nested_muldiv_decomposed (constant, fractions, std::move (fraction_terms), fraction.second);
	}

	return denominator_terms;
}

Node::Base::Ptr addsub_reconstruct_common_multiplier (rational_t value, DecompositionMap&& terms)
{
	if (Visitor::Simplify::options.sum_fractions) {
		rational_t denominator_multiplier (1);
		DecompositionMap denominator = addsub_multiply_by_common_denominator (value, terms);

		if (denominator.empty()) {
			return addsub_reconstruct (value, std::move (terms));
		} else {
			/* do that once again to get rid of nested fractions */
			for (;;) {
				DecompositionMap next_denominator = addsub_multiply_by_common_denominator (value, terms);
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
			Node::Base::Ptr numerator = addsub_reconstruct (value, std::move (terms));

			/* numerator can be a constant or a muldiv, so fold it properly */
			muldiv_decompose_fold_nested_single (denominator_multiplier, denominator, std::move (numerator), rational_t (1));

			return muldiv_reconstruct (denominator_multiplier, std::move (denominator));
		}
	} else {
		return addsub_reconstruct (value, std::move (terms));
	}
}

} // anonymous namespace

namespace Visitor {

Simplify::Options Simplify::options;

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
	std::cerr << "Simplify warning: unknown function: '" << node.name() << "'" << std::endl;

	Node::Function::Ptr result (new Node::Function (node.name()));

	for (const auto& child: node.children()) {
		result->add_child (child.node->accept_ptr (*this));
	}

	return static_cast<Node::Base*> (result.release());
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

	muldiv_decompose_fold_nested_muldiv_simplify (*this, result_value, node_terms, node, rational_t (1));

	return muldiv_reconstruct (result_value, std::move (node_terms)).release();
}

boost::any Simplify::visit (const Node::AdditionSubtraction& node)
{
	rational_t result_value (0);

	/* sum: term -> multiplier */
	DecompositionMap node_terms;

	addsub_decompose_fold_nested_addsub_simplify (*this, result_value, node_terms, node, rational_t (1));

	return addsub_reconstruct_common_multiplier (result_value, std::move (node_terms)).release();
}

} // namespace Visitor
