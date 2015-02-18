#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <utility>

#include <initializer_list>
#include <functional>
#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include <cassert>

#ifndef IN_KDEVELOP_PARSER
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#endif // IN_KDEVELOP_PARSER

#include <boost/any.hpp>

/*
 * Numeric: data types
 */

typedef long double data_t;
typedef boost::multiprecision::cpp_int integer_t;
typedef boost::rational<integer_t> rational_t;

/*
 * Numeric: constants
 */

namespace {

const data_t eps = 1e-9;

} // anonymous namespace

inline void configure_exceptions (std::istream& stream)
{
	stream.exceptions (std::istream::badbit | std::istream::failbit);
}

inline void configure_exceptions (std::ostream& stream)
{
	stream.exceptions (std::ostream::badbit | std::ostream::failbit);
}

/*
 * Opens a file for reading and configures ifstream's exceptions.
 */

inline void open (std::ifstream& stream, const char* name)
{
	configure_exceptions (stream);
	stream.open (name);
}

/*
 * Opens a file for writing and configures ofstream's exceptions.
 */

inline void open (std::ofstream& stream, const char* name)
{
	configure_exceptions (stream);
	stream.open (name);
}

/*
 * Reads a file into a vector.
 */

template <typename T = data_t>
inline std::vector<T> read_into_vector (std::istream& in)
{
	std::vector<T> ret;
	try {
		std::copy (std::istream_iterator<T> (in),
		           std::istream_iterator<T>(),
		           std::back_inserter (ret));
	} catch (std::ios_base::failure& e) {
		if (!in.eof()) throw;
	}
	return std::move (ret);
}

template <typename T = data_t>
inline std::pair<std::vector<T>, T> read_into_vector_errors (std::istream& in)
{
	std::vector<T> ret;
	T ret_error = 0;

	while (!in.eof()) {
		T value, error;
		std::cin >> value >> error >> std::ws;

		ret_error = std::max (ret_error, error);
		ret.push_back (std::move (value));
	}

	return std::pair<std::vector<T>, T> (std::move (ret), std::move (ret_error));
}

/*
 * Util: take (erase+return) an object from an associative container which prohibits modification of keys.
 * FIXME: UB, to be fixed in N3586, N3645 or follow-ups (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3645.pdf)
 */

template <typename T>
using map_value_type = std::pair<typename T::key_type, typename T::mapped_type>;

template <typename T>
typename T::value_type take_set (T& container, typename T::iterator iterator)
{
	try {
		typename T::value_type result = std::move (const_cast<typename T::value_type&> (*iterator));
		container.erase (iterator);
		return result;
	} catch (...) {
		container.erase (iterator);
		throw;
	}
}

template <typename T>
map_value_type<T> take_map (T& container, typename T::iterator iterator)
{
	try {
		map_value_type<T> result {
			std::move (const_cast<typename T::key_type&> (iterator->first)),
			std::move (iterator->second)
		};
		container.erase (iterator);
		return result;
	} catch (...) {
		container.erase (iterator);
		throw;
	}
}

/*
 * Numeric: converts a rational value to floating-point representation.
 */

inline data_t to_fp (const rational_t& arg)
{
    return boost::rational_cast<data_t> (arg);
}

/*
 * Numeric: compares two floating-point values.
 */

inline bool fp_cmp (data_t _1, data_t _2)
{
    return fabsl (_1 - _2) < eps;
}

/*
 * Numeric: returns average value of the vector.
 */

template <typename T>
inline data_t avg (const std::vector<T>& vec)
{
	return (data_t) std::accumulate (vec.begin(), vec.end(), (T) 0) / vec.size();
}

/*
 * Numeric: returns average value of pairwise per-component products of two vectors.
 * (That is, an inner product of two vectors divided by their length.)
 */

template <typename T>
inline data_t avg_product (const std::vector<T>& vec1, const std::vector<T>& vec2)
{
	return (data_t) std::inner_product (vec1.begin(), vec1.end(), vec2.begin(), (T) 0) / vec1.size();
}

/*
 * Numeric: a shorthand for X * X.
 */

template <typename T>
inline T sq (T val)
{
	return val * val;
}

/*
 * Numeric: an integer power operator.
 */

inline integer_t pow_int (integer_t base, integer_t exponent)
{
	integer_t result = 1;
	for (integer_t i = 0; i < exponent; ++i) {
		result *= base;
	}
	return result;
}

inline rational_t pow_frac (rational_t base, integer_t exponent)
{
	if (exponent >= 0) {
		return rational_t (pow_int (base.numerator(), exponent),
		                   pow_int (base.denominator(), exponent));
	} else {
		return rational_t (pow_int (base.denominator(), -exponent),
		                   pow_int (base.numerator(), -exponent));
	}
}

template <typename T>
inline bool any_isa (const boost::any& obj)
{
	return obj.type() == typeid (T);
}

inline rational_t any_to_rational (const boost::any& obj)
{
	return boost::any_cast<rational_t> (obj);
}

inline data_t any_to_fp (const boost::any& obj)
{
	if (const rational_t* val = boost::any_cast<rational_t> (&obj)) {
		return to_fp (*val);
	} else {
		return boost::any_cast<data_t> (obj);
	}
}

inline void rational_to_ostream (std::ostream& out, const rational_t& obj)
{
	if (obj.denominator() == 1) {
		out << obj.numerator();
	} else {
		out << obj;
	}
}

inline void any_to_ostream (std::ostream& out, const boost::any& obj)
{
	if (const rational_t* val = boost::any_cast<rational_t> (&obj)) {
		rational_to_ostream (out, *val);
	} else {
		out << boost::any_cast<data_t> (obj);
	}
}

inline void any_to_ostream_to_fp (std::ostream& out, const boost::any& obj)
{
	if (const rational_t* val = boost::any_cast<rational_t> (&obj)) {
		rational_to_ostream (out, *val);
		out << " ≈ " << to_fp (*val);
	} else {
		out << boost::any_cast<data_t> (obj);
	}
}
