#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>

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

bool update_error (double* error, double new_error) {
	if (new_error <= *error + 1e-3) {
		*error = new_error;
		return true;
	} else {
		return false;
	}
}

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
	       ylimit = parse_double (argv[6]),
		   result_x = 0,
		   result_y,
		   result_yR,
		   error;

	auto f = [a, b] (double x) { return a * x + b; };

	for (;;) {
		error = +INFINITY;

		for (double x = result_x + xstep, y = f (x); x <= xlimit && y <= ylimit; x += xstep, y = f (x)) {
			double floored = ystep * floor (y / ystep),
				   ceiled = ystep * ceil (y / ystep);

			if (update_error (&error, fabs (y - floored))) {
#ifdef DEBUG
				std::cerr << "DBG: choice updated: x = " << x << " y = " << y << " (floored to " << floored << ") error = " << error << std::endl;
#endif
				result_x = x;
				result_y = y;
				result_yR = floored;
			}

			if (update_error (&error, fabs (y - ceiled))) {
#ifdef DEBUG
				std::cerr << "DBG: choice updated: x = " << x << " y = " << y << " (ceiled to " << ceiled << ") error = " << error << std::endl;
#endif
				result_x = x;
				result_y = y;
				result_yR = ceiled;
			}
		}

		if (error < ystep) {
			std::cout << "x = " << result_x << " y = " << result_yR << " (" << result_y << ") error = " << error << std::endl;
		} else {
			break;
		}
	}
}
