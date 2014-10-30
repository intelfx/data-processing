#pragma once

#include <util.h>
#include <cctype>

class LexerIterator
{
public:
	typedef std::string string;
	typedef string::value_type character;
	string::const_iterator begin_, current_, current_end_, end_;
	string cache_;

private:
	enum class Classification
	{
		Nothing,
		Whitespace,
		Numeric,
		Alphabetical,
		PunctuationSingle,
		PunctuationMultiple
	};

	static bool same_lexem (Classification first, Classification next);

	Classification classify (string::const_iterator it);
	void next();
	bool is_end() const;

public:
	LexerIterator() = default;
	~LexerIterator() = default;
	LexerIterator (string::const_iterator b, string::const_iterator e);

	LexerIterator (const string& s)
	: LexerIterator (s.begin(), s.end())
	{
	}

	bool operator== (const LexerIterator& rhs) const;
	bool operator!= (const LexerIterator& rhs) const;

	LexerIterator& operator++();
	LexerIterator operator++(int);
	const string& operator*();
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
