#pragma once

#include <util/util.h>

#include <initializer_list>
#include <cctype>

class LexerIterator : public std::iterator<std::forward_iterator_tag, std::string>
{
public:
	typedef value_type string;
	typedef string::value_type character;

	enum class Classification
	{
		Nothing,
		Whitespace,
		Numeric,
		Alphabetical,
		OpParenthesisOpening,
		OpParenthesisClosing,
		OpArithmetic,
		OpPowerOperator
	};

	struct CachedLexem {
		string text;
		integer_t numeric;
		Classification classification = Classification::Nothing;
		bool is_valid = false;

		CachedLexem() = default;

		CachedLexem (integer_t value)
		: numeric (value)
		, classification (Classification::Numeric)
		, is_valid (true)
		{ }

		CachedLexem (const char* value, Classification c)
		: text (value)
		, classification (c)
		, is_valid (true)
		{ }
	};

private:
	string::const_iterator begin_, current_, current_end_, end_;

	const CachedLexem* fake_sequence_ = nullptr;
	CachedLexem cache_;

	bool classify_check_next (string::const_iterator it, const char* pattern);
	Classification classify (string::const_iterator it);
	void next();
	const CachedLexem* get_cache();
	const CachedLexem* get_cache_no_fill() const;
	bool is_end() const;

public:
	LexerIterator() = default;
	~LexerIterator() = default;
	LexerIterator (string::const_iterator b, string::const_iterator e);

	LexerIterator (const string& s)
	: LexerIterator (s.begin(), s.end())
	{
	}

	LexerIterator& operator++();
	LexerIterator operator++(int);
	const string& operator*();
	const string* operator->();
	operator bool() const;

	Classification get_class() const;
	integer_t get_numeric() const;

	bool check (std::initializer_list<string> list, size_t* idx = nullptr);
	bool check (const string& s);
	bool check_and_advance (std::initializer_list<string> list, size_t* idx = nullptr);
	bool check_and_advance (const string& s);

	friend std::ostream& operator<< (std::ostream& out, LexerIterator& lexem);
};

class Lexer
{
public:
	typedef LexerIterator::string string;

private:
	const string& s_;

public:
	Lexer (const string& s);

	LexerIterator begin() const;
	LexerIterator end() const;
};
