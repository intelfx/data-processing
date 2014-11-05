#pragma once

#include <util.h>

/*
 * Ancillary data types
 */

typedef long double data_t;

/*
 * Constants
 */

namespace {

const data_t eps = 1e-9;

} // anonymous namespace

/*
 * Defines a single variable with a value and an error.
 */

struct Variable
{
	data_t value, error;

	bool no_error() const { return fabsl (error) < eps; }
};
typedef std::map<std::string, Variable> VariableMap;

/*
 * Defines a single variable with name, value and error.
 */

struct NamedVariable
{
	std::string name;
	data_t value, error;
	enum Mode
	{
		NONE = 0,
		MINUS_ERROR,
		PLUS_ERROR
	} mode;

	bool no_error() const { return fabsl (error) < eps; }
};
typedef std::vector<NamedVariable> VariableVec;

/*
 * Adds a single named variable definition.
 */

inline void parse_variable (VariableVec& vec, std::istream& stream)
{
	NamedVariable v;
	v.mode = NamedVariable::NONE;
	stream >> v.name >> v.value >> v.error;
	vec.push_back (std::move (v));
}

/*
 * Adds a single variable definition.
 */

inline void parse_variable (VariableMap& map, std::istream& stream)
{
	std::string name;
	Variable v;
	stream >> name >> v.value >> v.error;
	map.insert (std::make_pair (std::move (name), std::move (v)));
}

/*
 * Compares two floating-point values.
 */

inline bool fp_cmp (data_t _1, data_t _2)
{
	return fabsl (_1 - _2) < eps;
}
