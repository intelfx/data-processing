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
	return get_arithm<Node::MultiplicationDivision> (&Parser::get_sub_expr,
	                                                 { "*", "/" },
	                                                 [] (Node::MultiplicationDivision::Ptr& node, Node::Base::Ptr&& child, size_t idx)
	                                                    { node->add_child (std::move (child), (idx == 1)); });
}

Node::Base::Ptr Parser::get_sub_expr()
{
	if (!current_) {
		throw std::runtime_error ("Parse error: unexpected end of expression");
	} else if (current_.check_and_advance ("(")) {
		Node::Base::Ptr sub_expr = parse();
		if (current_.check_and_advance (")")) {
			return std::move (sub_expr);
		}
	} else {
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

		auto it = variables_.find (*current_);
		if (it != variables_.end()) {
			++current_;
			return Node::Variable::Ptr (new Node::Variable (it->first, it->second));
		} else {
			std::ostringstream reason;
			reason << "Parse error: unknown variable: '" << *current_ << "'";
			throw std::runtime_error (reason.str());
		}
	}
}
