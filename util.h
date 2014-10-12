#include <iostream>
#include <fstream>
#include <iterator>
#include <utility>
#include <algorithm>
#include <numeric>
#include <vector>
#include <cmath>
#include <cstdlib>

/*
 * Opens a file for reading and configures ifstream's exceptions.
 */

void open (std::ifstream& stream, const char* name)
{
	stream.exceptions (std::ifstream::badbit | std::ifstream::failbit);
	stream.open (name);
}

/*
 * Opens a file for writing and configures ofstream's exceptions.
 */

void open (std::ofstream& stream, const char* name)
{
	stream.exceptions (std::ofstream::badbit | std::ofstream::failbit);
	stream.open (name);
}

/*
 * Reads a file into a vector.
 */

template <typename T>
std::vector<T> read_into_vector (std::istream& in)
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

/*
 * Numeric: returns average value of the vector.
 */
template <typename T>
double avg (const std::vector<T>& vec)
{
	return (double) std::accumulate (vec.begin(), vec.end(), (T) 0) / vec.size();
}

/*
 * Numeric: returns average value of pairwise per-component products of two vectors.
 * (That is, an inner product of two vectors divided by their length.)
 */

template <typename T>
double avg_product (const std::vector<T>& vec1, const std::vector<T>& vec2)
{
	return (double) std::inner_product (vec1.begin(), vec1.end(), vec2.begin(), (T) 0) / vec1.size();
}

/*
 * Numeric: a shorthand for X * X.
 */

template <typename T>
T sq (T val)
{
	return val * val;
}