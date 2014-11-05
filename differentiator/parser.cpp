#include "parser.h"

#include <sstream>

Parser::Parser (const Lexer::string& s, const VariableMap& v)
: current_ (LexerIterator (s))
, variables_ (v)
{
}

Node::Base::Ptr Parser::parse()
{
	return get_add_sub();
}

template <typename T, typename ChildInserter>
Node::Base::Ptr Parser::get_arithm (Node::Base::Ptr(Parser::*next)(),
                                    std::initializer_list<LexerIterator::string> tokens,
                                    ChildInserter inserter)
{
	Node::Base::Ptr child;
	typename T::Ptr node;
	size_t idx;

	if (current_.check_and_advance (tokens, &idx)) {
		node = typename T::Ptr (new T);
		inserter (node, (this->*next)(), idx);
	} else {
		child = (this->*next)();
	}

	while (current_.check_and_advance (tokens, &idx)) {
		if (!node) {
			node = typename T::Ptr (new T);
			inserter (node, std::move (child), 0);
		}

		inserter (node, (this->*next)(), idx);
	}

	if (child) {
		return std::move (child);
	} else {
		return std::move (node);
	}
}

Node::Base::Ptr Parser::get_add_sub()
{
	return get_arithm<Node::AdditionSubtraction> (&Parser::get_mul_div,
	                                              { "+", "-" },
	                                              [] (Node::AdditionSubtraction::Ptr& node, Node::Base::Ptr&& child, size_t idx)
	                                                 { node->add_child (std::move (child), (idx == 1)); });
}

Node::Base::Ptr Parser::get_mul_div()
{
	return get_arithm<Node::MultiplicationDivision> (&Parser::get_power,
	                                                 { "*", "/" },
	                                                 [] (Node::MultiplicationDivision::Ptr& node, Node::Base::Ptr&& child, size_t idx)
	                                                    { node->add_child (std::move (child), (idx == 1)); });
}

Node::Base::Ptr Parser::get_power()
{
	Node::Base::Ptr base = get_sub_expr();

	if (current_.check_and_advance ({ "^", "**" })) {
		Node::Power::Ptr result (new Node::Power);
		result->add_child (std::move (base));
		result->add_child (get_sub_expr());
		return std::move (result);
	} else {
		return std::move (base);
	}
}

Node::Base::Ptr Parser::get_sub_expr()
{
	/* end of expression */
	if (!current_) {
		throw std::runtime_error ("Parse error: unexpected end of expression");
	}

	/* sub-expression */
	if (current_.check_and_advance ("(")) {
		Node::Base::Ptr sub_expr = parse();
		if (current_.check_and_advance (")")) {
			return std::move (sub_expr);
		} else {
			throw std::runtime_error ("Parse error: mismatched parentheses");
		}
	}

	/* numeric literal */
	try {
		size_t pos;
		data_t value = std::stold (*current_, &pos);
		if (pos != current_->size()) {
			std::ostringstream reason;
			reason << "Parse error: wrong numeric literal: '" << *current_ << "'";
			throw std::runtime_error (reason.str());
		}
		++current_;
		return Node::Value::Ptr (new Node::Value (value));
	} catch (std::invalid_argument& e) {
		/* clearly not a value -- continue */
	}

	/* function or variable */
	std::string name = *current_;

	if (std::next (current_).check ("(")) {
		std::advance (current_, 2);

		Node::Function::Ptr node (new Node::Function (name));

		if (current_.check_and_advance (")")) {
			return std::move (node);
		}

		do {
			node->add_child (parse());
		} while (current_.check_and_advance (","));

		if (current_.check_and_advance (")")) {
			return std::move (node);
		} else {
			throw std::runtime_error ("Parse error: mismatched parentheses");
		}
	}

	/* variable */
	auto it = variables_.find (name);
	if (it != variables_.end()) {
		++current_;
		return Node::Variable::Ptr (new Node::Variable (it->first, it->second));
	} else {
		std::ostringstream reason;
		reason << "Parse error: unknown variable: '" << *current_ << "'";
		throw std::runtime_error (reason.str());
	}
}
