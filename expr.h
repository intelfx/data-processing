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
	bool do_not_substitute;

	bool no_error() const { return fabsl (error) < eps; }

	typedef std::map<std::string, Variable> Map;

	static Map::value_type make (std::string name, data_t value, data_t error, bool do_not_substitute)
	{
		return {std::move (name), Variable { value, error, do_not_substitute }};
	}

	static Map::value_type read (std::istream& in)
	{
		std::string name;
		data_t value, error;
		in >> name >> value >> error;
		return Map::value_type { name, Variable { value, error, false } };
	}
};

/*
 * Adds a single variable definition.
 */

inline void parse_variable (Variable::Map& map, std::istream& stream)
{
	map.insert (Variable::read (stream));
}

/*
 * Compares two floating-point values.
 */

inline bool fp_cmp (data_t _1, data_t _2)
{
	return fabsl (_1 - _2) < eps;
}
