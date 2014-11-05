#include "parser.h"

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
	Node::Base::Ptr child = (this->*next)();
	typename T::Ptr node;
	size_t idx;

	while (current_.check_and_advance (tokens, &idx)) {
		if (!node) {
			node = typename T::Ptr (new T);
			if (child) {
				inserter (node, std::move (child), 0);
			}
		}

		child = (this->*next)();
		if (child) {
			inserter (node, std::move (child), idx);
		} else {
			return nullptr;
		}
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
		return nullptr;
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
				return nullptr;
			}
			++current_;
			return Node::Value::Ptr (new Node::Value (value));
		} catch (std::invalid_argument& e) {
			/* clearly not a value -- continue */
		}

		auto it = variables_.find (*current_);
		if (it != variables_.end()) {
			++current_;
			return Node::Variable::Ptr (new Node::Variable (&it->second));
		}
	}

	return nullptr;
}
