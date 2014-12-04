#pragma once

#include <util.h>
#include "lexer.h"
#include "node.h"

class Parser
{
	LexerIterator current_;
	const Variable::Map& variables_;

	template <typename T, typename ChildInserter>
	Node::Base::Ptr get_arithm (Node::Base::Ptr(Parser::*next)(),
	                            std::initializer_list<LexerIterator::string> tokens,
	                            ChildInserter inserter);

	Node::Base::Ptr get_add_sub();
	Node::Base::Ptr get_mul_div();
	Node::Base::Ptr get_power();
	Node::Base::Ptr get_sub_expr();

public:
	Parser (const Lexer::string& s, const Variable::Map& v);
	~Parser() = default;

	Node::Base::Ptr parse();
};
