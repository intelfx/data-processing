#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <utility>

#include <algorithm>
#include <numeric>
#include <vector>
#include <map>
#include <list>
#include <cmath>
#include <cstdlib>

#ifndef IN_KDEVELOP_PARSER
#include <boost/rational.hpp>
#endif // IN_KDEVELOP_PARSER

/*
 * Numeric: data types
 */

typedef long double data_t;
typedef long integer_t;
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
