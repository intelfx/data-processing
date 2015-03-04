#include "lexer.h"

#include <cctype>

namespace {

LexerIterator::CachedLexem implicit_multiplication[] = {
	LexerIterator::CachedLexem ("*", LexerIterator::Classification::OpArithmetic),
	LexerIterator::CachedLexem()
};

bool check_implicit_multiplication (LexerIterator::Classification _1, LexerIterator::Classification _2)
{
	// '2x', '2(' (not vice versa!)
	if ((_1 == LexerIterator::Classification::Numeric) &&
	    ((_2 == LexerIterator::Classification::Alphabetical) ||
	     (_2 == LexerIterator::Classification::OpParenthesisOpening))) {
		return true;
	}

	return false;
}

} // anonymous namespace

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

	if (isdigit (*it)) {
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
			return Classification::OpParenthesisOpening;

		case ')':
			return Classification::OpParenthesisClosing;
		}
	}


	ERROR (std::runtime_error, "Parse error: unknown character: '" << *it << "'");
}

void LexerIterator::next()
{
	// first, try to advance the fake sequence
	if (fake_sequence_) {
		++fake_sequence_;

		// check if the sequence has ended; then the actual value (cache_) comes into effect
		if (fake_sequence_->classification == Classification::Nothing) {
			fake_sequence_ = nullptr;
		}

		return;
	}

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
	case Classification::OpParenthesisOpening:
	case Classification::OpParenthesisClosing:
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

		VERIFY (end_char > current_char, std::logic_error, "Lexer error: could not parse a number at '" << *current_char << "' (classification error)");

		current_end_ += (end_char - current_char);
		cache_.is_valid = true;
		break;
	}

	default:
	case Classification::Nothing:
		cache_.is_valid = true;
		break;
	}

	// try to match one of the fake sequences (before we overwrite the previous lexem's class)
	if (check_implicit_multiplication (cache_.classification, cl)) {
		fake_sequence_ = implicit_multiplication;
	}

	// set class
	cache_.classification = cl;
}

const LexerIterator::CachedLexem* LexerIterator::get_cache()
{
	if (fake_sequence_) {
		if (!fake_sequence_->is_valid) {
			throw std::logic_error ("Fake sequence lexem has invalid cached value");
		}
		return fake_sequence_;
	}

	if (!cache_.is_valid) {
		cache_.text = std::string (current_, current_end_);
		cache_.is_valid = true;
	}
	return &cache_;
}

const LexerIterator::CachedLexem* LexerIterator::get_cache_no_fill() const
{
	if (fake_sequence_) {
		return fake_sequence_;
	}

	return &cache_;
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
	return get_cache()->text;
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
	return get_cache_no_fill()->classification;
}

integer_t LexerIterator::get_numeric() const
{
	const CachedLexem* lexem = get_cache_no_fill();

	if (lexem->classification != Classification::Numeric) {
		throw std::runtime_error ("Requested numeric value of a non-numeric lexem");
	}
	if (!lexem->is_valid) {
		throw std::runtime_error ("Numerlc lexem has invalid cached value");
	}
	return lexem->numeric;
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
