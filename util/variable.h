#pragma once

#include "util.h"

/*
 * Defines a single variable with a value and an error.
 */

struct Variable
{
	boost::any value, error;
	bool do_not_substitute;

	bool no_error() const { return error.empty() || fabsl (any_to_fp (error)) < eps; }

	typedef std::map<std::string, Variable> Map;

	template <typename T>
	static Map::value_type make (std::string name, T value, T error, bool do_not_substitute)
	{
		return {std::move (name), Variable { value, error, do_not_substitute }};
	}

	template <typename T>
	static Map::value_type read (std::istream& in);
};

template <typename T>
inline Variable::Map::value_type Variable::read (std::istream& in)
{
	std::string name;
	T value, error;
	in >> name >> std::ws >> value >> std::ws >> error >> std::ws; // work around bugs in boost::multiprecision or boost::rational or clang
	return Map::value_type { name, Variable { value, error, false } };
}

template <>
inline Variable::Map::value_type Variable::read<void> (std::istream& in)
{
	std::string name;
	in >> name;
	return Map::value_type { name, Variable { boost::any(), boost::any(), false } };
}

/*
 * Adds a single variable definition.
 */

template <typename T>
inline void parse_variable (Variable::Map& map, std::istream& stream)
{
	map.insert (Variable::read<T> (stream));
}

/*
 * Populates the variable definition list by reading a file.
 */

template <typename T>
void parse_variables_from_file (Variable::Map& map, const char* path)
{
	std::ifstream variable_file;
	open (variable_file, path);

	while (!(variable_file >> std::ws, variable_file.eof())) {
		parse_variable<T> (map, variable_file);
	}
}
