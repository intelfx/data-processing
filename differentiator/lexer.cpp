#include "lexer.h"

bool LexerIterator::classify_check_next (string::const_iterator it, const char* pattern)
{
	while (*pattern) {
		if ((it == end_) || (*it != *pattern)) {
			return false;
		}
		++it;
		++pattern;
	}
	return true;
}

LexerIterator::Classification LexerIterator::classify (string::const_iterator it)
{
	if (it == end_) {
		return Classification::Nothing;
	}

	if (isspace (*it)) {
		return Classification::Whitespace;
	}

	if (isalpha (*it) || *it == '_') {
		return Classification::Alphabetical;
	}

	if (isdigit (*it) || *it == '.') {
		return Classification::Numeric;
	}

	if (ispunct (*it)) {
		switch (*it) {
		case '*':
			if (classify_check_next (it + 1, "*")) {
				return Classification::OpPowerOperator;
			}
			return Classification::OpArithmetic;

		case '+':
		case '-':
		case '/':
		case '^':
			return Classification::OpArithmetic;

		case '(':
		case ')':
			return Classification::OpParentheses;
		}
	}

	std::ostringstream reason;
	reason << "Parse error: unknown character: '" << *it << "'";
	throw std::runtime_error (reason.str());
}

void LexerIterator::next()
{
	Classification cl;

	cache_.text.clear();
	cache_.is_valid = false;

	// seek to end of whitespace (or end of input)
	while ((cl = classify (current_end_)) == Classification::Whitespace) {
		++current_end_;
	}
	current_ = current_end_;

	// determine length of current lexem and also optionally fill the cache
	switch (cl) {
	case Classification::OpParentheses:
	case Classification::OpArithmetic:
		current_end_ += 1;
		break;

	case Classification::OpPowerOperator:
		current_end_ += 2;
		break;

	case Classification::Alphabetical:
		// first character is already classified
		do {
			++current_end_;
		} while (isalpha (*current_end_) ||
		         isdigit (*current_end_) ||
		         (*current_end_ == '_'));
		break;

	case Classification::Numeric: {
		/*
		 * FIXME: this assumes string elements are stored contiguously, i. e.
		 * *(iter + n) == *(&*iter + n), and that the string is NULL-terminated.
		 */
		const character* current_char = &*current_;
		character* end_char;
		cache_.numeric = std::strtol (current_char, &end_char, 0);
		current_end_ += (end_char - current_char);
		cache_.is_valid = true;
		break;
	}

	default:
	case Classification::Nothing:
		cache_.is_valid = true;
		break;
	}

	// set class
	cache_.classification = cl;
}

void LexerIterator::fill_cache()
{
	if (!cache_.is_valid) {
		cache_.text = std::string (current_, current_end_);
		cache_.is_valid = true;
	}
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
	fill_cache();
	return cache_.text;
}

const LexerIterator::string* LexerIterator::operator->()
{
	return &operator*();
}

LexerIterator::operator bool() const
{
	return !is_end();
}

LexerIterator::Classification LexerIterator::get_class() const
{
	return cache_.classification;
}

integer_t LexerIterator::get_numeric() const
{
	if (cache_.classification != Classification::Numeric) {
		throw std::runtime_error ("Requested numeric value of a non-numeric lexem");
	}
	if (!cache_.is_valid) {
		throw std::runtime_error ("Numerlc lexem has invalid cached value");
	}
	return cache_.numeric;
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

bool LexerIterator::check (const LexerIterator::string& s)
{
	return (operator*() == s);
}

bool LexerIterator::check_and_advance (std::initializer_list<string> list, size_t* idx)
{
	if (check (list, idx)) {
		operator++();
		return true;
	}

	return false;
}

bool LexerIterator::check_and_advance (const LexerIterator::string& s)
{
	if (check (s)) {
		operator++();
		return true;
	}

	return false;
}

std::ostream& operator<< (std::ostream& out, LexerIterator& lexem)
{
	switch (lexem.get_class()) {
	case LexerIterator::Classification::Numeric:
		out << lexem.get_numeric();
		break;

	case LexerIterator::Classification::Nothing:
		out << "<end of input>";
		break;

	default:
		out << "'" << *lexem << "'";
		break;
	}

	return out;
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
