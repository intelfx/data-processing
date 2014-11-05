#include "lexer.h"

bool LexerIterator::same_lexem (Classification first, Classification next)
{
	return ((first != Classification::PunctuationSingle) && (first == next))
	    || ((first == Classification::Alphabetical) && (next == Classification::Numeric));
}

LexerIterator::Classification LexerIterator::classify (string::const_iterator it)
{
	if (it == end_) {
		return Classification::Nothing;
	}

	if (isspace (*it)) {
		return Classification::Whitespace;
	}

	if (isalpha (*it)) {
		return Classification::Alphabetical;
	}

	if (isdigit (*it)) {
		return Classification::Numeric;
	}

	if (ispunct (*it)) {
		switch (*it) {
		// These chars may be used consequently with others as a part of single lexem
		case '=':
		case '>':
		case '<':
		case '+':
		case '-':
		case '!':
			return Classification::PunctuationMultiple;

		default:
			return Classification::PunctuationSingle;
		}
	}

	return Classification::Nothing;
}

void LexerIterator::next()
{
	Classification cl, next;

	cache_.clear();

	// seek to end of whitespace (or end of input)
	while ((cl = classify (current_end_)) == Classification::Whitespace) {
		++current_end_;
	}
	current_ = current_end_;

	// if this is end of input, return
	if (cl == Classification::Nothing) {
		return;
	}

	do {
		next = classify (++current_end_);
	} while (same_lexem (cl, next));
}

bool LexerIterator::is_end() const
{
	return current_ == end_; // also true for default-constructed Lexer
}

LexerIterator::LexerIterator (string::const_iterator b, string::const_iterator e)
: begin_ (b)
, current_ (b)
, current_end_ (b)
, end_ (e)
{
	next();
}

bool LexerIterator::operator== (const LexerIterator& rhs) const
{
	if (is_end()) {
		return rhs.is_end();
	} else {
		return begin_ == rhs.begin_
		    && current_ == rhs.current_
		    && end_ == rhs.end_;
	}
}

bool LexerIterator::operator!= (const LexerIterator& rhs) const
{
	return !(*this == rhs);
}

LexerIterator& LexerIterator::operator++()
{
	next();
	return *this;
}

LexerIterator LexerIterator::operator++(int)
{
	LexerIterator l = *this;
	next();
	return l;
}

const LexerIterator::string& LexerIterator::operator*()
{
	if ((current_ != current_end_) && cache_.empty()) {
		cache_ = string (current_, current_end_);
	}

	return cache_;
}

const LexerIterator::string* LexerIterator::operator->()
{
	return &operator*();
}

LexerIterator::operator bool() const
{
	return !is_end();
}

bool LexerIterator::check (std::initializer_list<string> list, size_t* idx)
{
	size_t matched = 0;

	for (const string& s: list) {
		if (operator*() == s) {
			if (idx) {
				*idx = matched;
			}
			return true;
		}

		++matched;
	}

	return false;
}

bool LexerIterator::check_and_advance (std::initializer_list<string> list, size_t* idx)
{
	if (check (list, idx)) {
		operator++();
		return true;
	}

	return false;
}

bool LexerIterator::check_and_advance(const LexerIterator::string& s)
{
	if (operator*() == s) {
		operator++();
		return true;
	}

	return false;
}

Lexer::Lexer (const string& s)
: s_ (s)
{
}

LexerIterator Lexer::begin() const
{
	return LexerIterator (s_.begin(), s_.end());
}

LexerIterator Lexer::end() const
{
	return LexerIterator();
}
