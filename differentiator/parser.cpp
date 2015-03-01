#include "parser.h"

Parser::Parser (const Lexer::string& s, const Variable::Map& v)
: current_ (LexerIterator (s))
, variables_ (v)
{
}

Node::Base::Ptr Parser::parse()
{
	Node::Base::Ptr result = get_toplevel();
	if (current_) {
		ERROR (std::runtime_error, "Parse error: " << current_ << ": expected end of input");
	}
	return result;
}

Node::Base::Ptr Parser::get_toplevel()
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
		return child;
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
		result->set_base (std::move (base));
		result->set_exponent (get_sub_expr());
		return std::move (result);
	} else {
		return base;
	}
}

Node::Base::Ptr Parser::get_sub_expr()
{
	/* end of expression */
	if (!current_) {
		ERROR (std::runtime_error, "Parse error: " << current_ << ": expected expression");
	}

	/* sub-expression */
	if (current_.check_and_advance ("(")) {
		Node::Base::Ptr sub_expr = get_toplevel();
		if (current_.check_and_advance (")")) {
			return sub_expr;
		} else {
			ERROR (std::runtime_error, "Parse error: " << current_ << ": expected closing parenthesis");
		}
	}

	/* numeric literal */
	if (current_.get_class() == LexerIterator::Classification::Numeric) {
		integer_t value = current_.get_numeric();
		++current_;
		return Node::Value::Ptr (new Node::Value (value));
	}

	/* function or variable */
	if (current_.get_class() != LexerIterator::Classification::Alphabetical) {
			ERROR (std::runtime_error, "Parse error: " << current_ << ": symbol expected");
	}

	std::string name = *current_;

	if (std::next (current_).check ("(")) {
		std::advance (current_, 2);

		Node::Function::Ptr node (new Node::Function (name));

		if (current_.check_and_advance (")")) {
			return std::move (node);
		}

		do {
			node->add_child (get_toplevel());
		} while (current_.check_and_advance (","));

		if (current_.check_and_advance (")")) {
			return std::move (node);
		} else {
			ERROR (std::runtime_error, "Parse error: " << current_ << ": expected closing parenthesis");
		}
	}

	/* variable */
	auto it = variables_.find (name);
	if (it != variables_.end()) {
		++current_;
		return Node::Variable::Ptr (new Node::Variable (it->first, it->second, false));
	} else {
		ERROR (std::runtime_error, "Parse error: unknown variable: '" << *current_ << "'");
	}
}
