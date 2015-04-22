#include <util/util.h>
#include <cstring>

#include <getopt.h>

enum {
	ARG_MACHINE_OUTPUT          = 'm',
	ARG_TERSE_OUTPUT            = 'q',
	ARG_FILE_X                  = 'x',
	ARG_FILE_Y                  = 'y',
	ARG_MODE_K                  = 0x100,
	ARG_MODE_AB
};

namespace {

const option option_array[] = {
	{ "machine",       no_argument,       nullptr, ARG_MACHINE_OUTPUT },
	{ "terse",         no_argument,       nullptr, ARG_TERSE_OUTPUT },
	{ "k",             no_argument,       nullptr, ARG_MODE_K },
	{ "ab",            no_argument,       nullptr, ARG_MODE_AB },
	{ "file-x",        required_argument, nullptr, ARG_FILE_X },
	{ "file-y",        required_argument, nullptr, ARG_FILE_Y },
	{ }
};

void usage (const char* name) {
	std::cerr << "Usage: " << name << " [-m|--machine] [-q|--terse]" << std::endl
	          << "       -x|--file-x=<X data file> -y|--file-y=<Y data file> [--k] [--ab]" << std::endl;
	exit (EXIT_FAILURE);
}

}

int main (int argc, char** argv)
{
	enum class Mode
	{
		Unknown = 0,
		K,
		AB
	};

	struct {
		struct {
			struct {
				bool enabled;
			} machine;

			struct {
				bool terse;
			} common;
		} output;

		struct {
			std::ifstream x_file, y_file;
		} input;

		Mode mode = Mode::Unknown;
	} parameters = { };

	/*
	 * Parse the command-line arguments.
	 */

	int option;
	while ((option = getopt_long (argc, argv, "mqx:y:", option_array, nullptr)) != -1) {
		switch (option) {
		case ARG_MACHINE_OUTPUT:
			parameters.output.machine.enabled = true;
			break;

		case ARG_TERSE_OUTPUT:
			parameters.output.common.terse = true;
			break;

		case ARG_MODE_AB:
			ASSERT (parameters.mode == Mode::Unknown, "Mode set twice");
			parameters.mode = Mode::AB;
			break;

		case ARG_MODE_K:
			ASSERT (parameters.mode == Mode::Unknown, "Mode set twice");
			parameters.mode = Mode::K;
			break;

		case ARG_FILE_X:
			open (parameters.input.x_file, optarg);
			break;

		case ARG_FILE_Y:
			open (parameters.input.y_file, optarg);
			break;

		default:
			usage (argv[0]);
		}
	}

	/*
	 * Verify arguments' consistency.
	 */

	if (optind < argc) {
		std::cerr << "Stray arguments detected." << std::endl;
		usage (argv[0]);
	}

	if (parameters.mode == Mode::Unknown) {
		std::cerr << "One of operation modes ('--k' or '--ab') is expected." << std::endl;
		usage (argv[0]);
	}

	auto x_data = read_into_vector<double> (parameters.input.x_file),
	     y_data = read_into_vector<double> (parameters.input.y_file);

	if (x_data.size() != y_data.size()) {
		std::cerr << "Measurement counts differ" << std::endl;
		exit (EXIT_FAILURE);
	}

	if (!parameters.output.common.terse) {
		std::cerr << x_data.size() << " measurements for X" << std::endl
		          << y_data.size() << " measurements for Y" << std::endl;
	}

	double x_avg = avg (x_data), y_avg = avg (y_data),
	       xy_avg = avg_product (x_data, y_data),
	       x_sq_avg = avg_product (x_data, x_data),
	       y_sq_avg = avg_product (y_data, y_data),
	       a, b, a_err, b_err;

	switch (parameters.mode) {
	case Mode::K:
		a = xy_avg / x_sq_avg;
		b = 0;

		a_err = sqrt ((y_sq_avg / x_sq_avg - sq (a)) / x_data.size());
		b_err = 0;

		if (parameters.output.common.terse) {
			std::cerr << "Y = (" << a << " ± " << a_err << ") * X" << std::endl;
		} else {
			std::cerr << "Y = k * X" << std::endl
			          << "k = " << a << " ± " << a_err << std::endl;
		}

		if (parameters.output.machine.enabled) {
			std::cout << "k " << a << " " << a_err << std::endl;
		}

		break;

	case Mode::AB:
		a = (xy_avg - x_avg * y_avg) / (x_sq_avg - sq (x_avg));
		b = y_avg - a * x_avg;

		a_err = sqrt (((y_sq_avg - sq (y_avg)) / (x_sq_avg - sq (x_avg)) - sq (a)) / x_data.size());
		b_err = a_err * sqrt (x_sq_avg - sq (x_avg));

		if (parameters.output.common.terse) {
			std::cerr << "Y = (" << a << " ± " << a_err << ") * X + (" << b << " ± " << b_err << ")" << std::endl;
		} else {
			std::cerr << "Y = a * X + b" << std::endl
			          << "a = " << a << " ± " << a_err << std::endl
			          << "b = " << b << " ± " << b_err << std::endl;
		}

		if (parameters.output.machine.enabled) {
			std::cout << "A " << a << " " << a_err << std::endl;
			std::cout << "B " << b << " " << b_err << std::endl;
		}

		break;

	HANDLE_DEFAULT_CASE
	}
}