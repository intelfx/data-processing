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
 * Defines a single variable with name, value and error.
 */

struct Variable
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

/*
 * Adds a single variable definition.
 */

void parse_variable (std::vector<Variable>& vec, std::istream& stream)
{
	Variable v;
	v.mode = Variable::NONE;
	stream >> v.name >> v.value >> v.error;
	vec.push_back (std::move (v));
}
