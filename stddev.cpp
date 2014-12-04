#include "util.h"

static const size_t MAX_SIGMA = 2;

int main (int argc, char** argv)
{
	double systematic_error = 0;
	std::string variable_name;

	if (argc > 1) {
		std::istringstream ss (argv[1]);
		ss.exceptions (std::istream::badbit | std::istream::failbit);
		ss >> systematic_error;
	}

	bool verbose = !getenv ("TERSE");
	bool data_with_errors = getenv ("INPUT_ERRORS");
	bool machine_output = false;

	if (char* output_variable = getenv ("OUTPUT")) {
		if (output_variable[0] != '\0') {
			variable_name = std::string (output_variable);
			machine_output = true;
		} else {
			std::cerr << "WARNING: OUTPUT= variable is set but empty; set it to desired variable name" << std::endl;
		}
	}

	std::cin.exceptions (std::istream::badbit | std::istream::failbit);

	std::vector<double> data;
	if (data_with_errors) {
		std::tie (data, systematic_error) = read_into_vector_errors<double> (std::cin);
	} else {
		data = read_into_vector<double> (std::cin);
	}

	double average = avg (data);
	double squared_difference_sum = std::accumulate (data.begin(), data.end(), (double) 0,
	                                                 [average] (double acc, double value) { return acc + (value - average) * (value - average); });

	double stddev = sqrt (squared_difference_sum / (data.size() - 1));
	double stderror = sqrt (squared_difference_sum) / data.size();
	double total_error;

	if (!variable_name.empty()) {
		std::cerr << "variable: " << variable_name << std::endl;
	}
	std::cerr << "average: " << average << std::endl;
	if (verbose) {
		std::cerr << "standard deviation of the sample: " << stddev << std::endl;
	}
	std::cerr << "standard error of the average: " << stderror << std::endl;

	if (systematic_error) {
		total_error = sqrt (sq (stderror) + sq (systematic_error));
		std::cerr << "systematic error: " << systematic_error << std::endl;
		std::cerr << "total error of the average: " << total_error << std::endl;
	} else {
		total_error = stderror;
	}

	if (verbose) {
		size_t measurements_in_range[MAX_SIGMA] = { };

		for (size_t sigma = 1; sigma <= MAX_SIGMA; ++sigma) {
			measurements_in_range[sigma - 1] = std::count_if (data.begin(), data.end(), [average, stddev, sigma] (double value) { return fabs (value - average) <= stddev * sigma; });
		}

		for (size_t sigma = 1; sigma <= MAX_SIGMA; ++sigma) {
			size_t count = measurements_in_range[sigma - 1];
			std::cerr << "error = " << sigma << " sigma (" << stddev * sigma << "): " << count << " measurements (" << (double) 100 * count / data.size() << "%)" << std::endl;
		}
	}

	if (machine_output) {
		std::cout << variable_name << " " << average << " " << total_error << std::endl;
	}
}
