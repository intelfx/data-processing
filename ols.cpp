#include "util.h"
#include <cstring>

int main (int argc, char** argv)
{
	if (argc != 4) {
		std::cerr << "This program takes three arguments." << std::endl
		          << "Usage: " << argv[0] << " [X data file] [Y data file] K|AB" << std::endl;
		exit (EXIT_FAILURE);
	}

	std::ifstream x_file, y_file;
	open (x_file, argv[1]);
	open (y_file, argv[2]);

	auto x_data = read_into_vector<double> (x_file),
	     y_data = read_into_vector<double> (y_file);

	std::cerr << x_data.size() << " measurements for X" << std::endl
	          << y_data.size() << " measurements for Y" << std::endl;

	if (x_data.size() != y_data.size()) {
		std::cerr << "Measurement counts differ" << std::endl;
		exit (EXIT_FAILURE);
	}

	double x_avg = avg (x_data), y_avg = avg (y_data),
	       xy_avg = avg_product (x_data, y_data),
	       x_sq_avg = avg_product (x_data, x_data),
	       y_sq_avg = avg_product (y_data, y_data),
	       a, b, a_err, b_err;

	if (!strcmp (argv[3], "k")) {
	    a = xy_avg / x_sq_avg;
		b = 0;

	    a_err = sqrt ((y_sq_avg / x_sq_avg - sq (a)) / x_data.size());
		b_err = 0;

		std::cout << "Y = k * X" << std::endl
		          << "k = " << a << " ± " << a_err << std::endl;
	} else if (!strcmp (argv[3], "ab")) {
		b = (xy_avg - sq (x_avg)) / (x_sq_avg - sq (x_avg));
		a = y_avg - b * x_avg;

		b_err = sqrt (((y_sq_avg - sq (y_avg)) / (x_sq_avg - sq (x_avg)) - sq (b)) / x_data.size());
		a_err = b_err * sqrt (x_sq_avg - sq (x_avg));

		std::cout << "Y = a * X + b" << std::endl
		          << "a = " << a << " ± " << a_err << std::endl
		          << "b = " << b << " ± " << b_err << std::endl;
	} else {
		std::cerr << "Wrong mode: " << argv[3] << std::endl;
		exit (EXIT_FAILURE);
	}
}
