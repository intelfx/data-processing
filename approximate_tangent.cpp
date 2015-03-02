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
	double x, y;
	double rounded_x, rounded_y;
	int steps_x, steps_y;
};

int main (int argc, char** argv)
{
	if (argc != 9) {
		std::cerr << "This program takes eight arguments." << std::endl
		          << "Usage (Y = a * X + b): " << argv[0] << " [a] [b] [X step] [Y step] [X min] [Y min] [X max] [Y max]" << std::endl;
		exit (EXIT_FAILURE);
	}

	double a = parse_double (argv[1]),
	       b = parse_double (argv[2]),
	       step_x = parse_double (argv[3]),
	       step_y = parse_double (argv[4]),
	       min_x = parse_double (argv[5]),
	       min_y = parse_double (argv[6]),
	       max_x = parse_double (argv[7]),
	       max_y = parse_double (argv[8]);

	std::map<double, Result> results;

	auto f = [a, b] (double x) { return a * x + b; };

	for (double x = min_x, y = f (x); x <= max_x && y <= max_y; x += step_x, y = f (x)) {
		if (y < min_y) {
			continue;
		}

		double charted_x = x - min_x,
		       charted_y = y - min_y;

		int steps_y_floor = floor (charted_y / step_y),
		    steps_y_ceil = ceil (charted_y / step_y),
		    steps_x = round (x / step_x);

		double rounded_y_floored = step_y * steps_y_floor,
		       rounded_y_ceiled = step_y * steps_y_ceil,
			   rounded_x = step_x * steps_x;

		double dist_to_floor = charted_y - rounded_y_floored,
		       dist_to_ceil = rounded_y_ceiled - charted_y;

		if (dist_to_floor < dist_to_ceil) {
			if (dist_to_floor < step_y) {
				results.emplace (dist_to_floor, Result { x, y, rounded_x + min_x, rounded_y_floored + min_y, steps_x, steps_y_floor });
			}
		} else {
			if (dist_to_ceil < step_y) {
				results.emplace (rounded_y_ceiled - y, Result { x, y, rounded_x + min_x, rounded_y_ceiled + min_y, steps_x, steps_y_ceil });
			}
		}

		if (results.size() > MAX_RESULTS) {
			results.erase (--results.end());
		}
	}

	for (const auto& res: results) {
		std::cout << "x = " << res.second.x << " y = " << res.second.y
		          << " (rounded: x = " << res.second.rounded_x << " y = " << res.second.rounded_y << ")"
		          << " (steps: x = " << res.second.steps_x << " y = " << res.second.steps_y << ")" << std::endl;
	}
}
