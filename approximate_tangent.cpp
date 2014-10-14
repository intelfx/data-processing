#include <iostream>
#include <algorithm>
#include <map>
#include <cmath>
#include <cstdlib>

namespace {

static const int MAX_RESULTS = 5;

} // anonymous namespace

double parse_double (const char* ptr)
{
	char* endptr;
	double ret = std::strtod (ptr, &endptr);
	if (*endptr != '\0') {
		std::cerr << "Wrong floating-point value: '" << ptr << "'" << std::endl;
		exit (EXIT_FAILURE);
	}
	return ret;
}

struct Result
{
	double x, y, yR;	
};

int main (int argc, char** argv)
{
	if (argc != 7) {
		std::cerr << "This program takes six arguments." << std::endl
		          << "Usage (Y = a * X + b): " << argv[0] << " [a] [b] [X step] [Y step] [X limit] [Y limit]" << std::endl;
		exit (EXIT_FAILURE);
	}

	double a = parse_double (argv[1]),
	       b = parse_double (argv[2]),
	       xstep = parse_double (argv[3]),
	       ystep = parse_double (argv[4]),
	       xlimit = parse_double (argv[5]),
	       ylimit = parse_double (argv[6]);

	std::map<double, Result> results;

	auto f = [a, b] (double x) { return a * x + b; };

	for (double x = xstep, y = f (x); x <= xlimit && y <= ylimit; x += xstep, y = f (x)) {
		double floored = ystep * floor (y / ystep),
		       ceiled = ystep * ceil (y / ystep);

		if (y - floored < ceiled - y) {
			if (y - floored < ystep) {
				results.emplace (y - floored, Result { x, y, floored });
			}
		} else {
			if (ceiled - y < ystep) {
				results.emplace (ceiled - y, Result { x, y, ceiled });
			}
		}

		if (results.size() > MAX_RESULTS) {
			results.erase (--results.end());
		}
	}

	for (auto it = results.begin(); it != results.end(); ++it) {
		std::cout << "x = " << it->second.x << " y = " << it->second.yR << " (" << it->second.y << ") error = " << it->first << std::endl;
	}
}
