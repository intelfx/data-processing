#include <util/util.h>

#include <getopt.h>

enum {
	ARG_MACHINE_OUTPUT          = 'm',
	ARG_GLOBAL_VAR_NAME         = 'n',
	ARG_TERSE_OUTPUT            = 'q',
	ARG_ADDITIONAL_ERROR        = 'E',
	ARG_INPUT_HAS_ERRORS        = 'e',
	ARG_MACHINE_OUTPUT_VAR_NAME = 0x100,
};

namespace {

const option option_array[] = {
	{ "machine",       no_argument,       nullptr, ARG_MACHINE_OUTPUT },
	{ "terse",         no_argument,       nullptr, ARG_TERSE_OUTPUT },
	{ "name",          required_argument, nullptr, ARG_GLOBAL_VAR_NAME },
	{ "name-machine",  required_argument, nullptr, ARG_MACHINE_OUTPUT_VAR_NAME },
	{ "input-errors",  no_argument,       nullptr, ARG_INPUT_HAS_ERRORS },
	{ "error",         required_argument, nullptr, ARG_ADDITIONAL_ERROR },
	{ }
};

void usage (const char* name) {
	std::cerr << "Usage: " << name << " [-m|--machine] [-q|--terse]" << std::endl
	          << "       [-n|--name NAME] [--name-machine NAME]" << std::endl
	          << "       [-e|--input-errors] [-E|--error SYSTEMATIC-ERROR]" << std::endl;
	exit (EXIT_FAILURE);
}

const size_t MAX_SIGMA = 2;

} // anonymous namespace

void process_dataset (const std::string& name, const std::string& name_machine, const std::vector<data_t>& data, data_t systematic_error,
                      bool output_machine, bool output_terse);

int main (int argc, char** argv)
{
	struct {
		struct {
			struct {
				bool enabled;
				std::string name;
			} machine;

			struct {
				bool terse;
				std::string name;
			} common;
		} output;

		struct {
			bool has_errors;
			data_t additional_error;
		} input;
	} parameters = { };

	/*
	 * Parse the command-line arguments.
	 */

	int option;
	while ((option = getopt_long (argc, argv, "mn:qeE:", option_array, nullptr)) != -1) {
		switch (option) {
		case ARG_MACHINE_OUTPUT:
			parameters.output.machine.enabled = true;
			break;

		case ARG_GLOBAL_VAR_NAME:
			parameters.output.common.name = optarg;
			break;

		case ARG_MACHINE_OUTPUT_VAR_NAME:
			parameters.output.machine.name = optarg;
			break;

		case ARG_TERSE_OUTPUT:
			parameters.output.common.terse = true;
			break;

		case ARG_INPUT_HAS_ERRORS:
			parameters.input.has_errors = true;
			break;

		case ARG_ADDITIONAL_ERROR: {
			std::istringstream ss (optarg);
			configure_exceptions (ss);
			ss >> parameters.input.additional_error;
			break;
		}

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

	/*
	 * Read the input data.
	 */

	configure_exceptions (std::cin);

	if (parameters.input.has_errors) {
		if (!parameters.output.common.name.empty()) {
			std::cerr << "Warning: configured dataset name '" << parameters.output.common.name << "' will be ignored." << std::endl
			          << std::endl;
		}

		if (!parameters.output.machine.name.empty()) {
			std::cerr << "Warning: configured machine output name '" << parameters.output.machine.name << "' will be ignored." << std::endl
			          << std::endl;
		}

		if (!fp_cmp (parameters.input.additional_error, 0)) {
			std::cerr << "Warning: configured dataset systematic error == " << parameters.input.additional_error << " will be ignored." << std::endl
			          << std::endl;
		}

		Dataset<data_t>::Map datasets = read_into_vector_errors (std::cin);
		for (Dataset<data_t>::Map::iterator it = datasets.begin(); it != datasets.end(); ++it) {
			process_dataset (it->first, it->first, it->second.data, it->second.error,
			                 parameters.output.machine.enabled, parameters.output.common.terse);
		}
	} else {
		/*
		 * Configure the output name.
		 */

		if (parameters.output.common.name.empty()) {
			parameters.output.common.name = "F";
		}

		if (parameters.output.machine.name.empty()) {
			parameters.output.machine.name = parameters.output.common.name;
		}

		std::vector<data_t> data = read_into_vector (std::cin);
		process_dataset (parameters.output.common.name, parameters.output.machine.name, data, parameters.input.additional_error,
		                 parameters.output.machine.enabled, parameters.output.common.terse);
	}
}


void process_dataset (const std::string& name, const std::string& name_machine, const std::vector<data_t>& data, data_t systematic_error,
                      bool output_machine, bool output_terse)
{
	data_t average = avg (data);
	data_t squared_difference_sum = std::accumulate (data.begin(), data.end(), (double) 0,
	                                                 [average] (double acc, double value) { return acc + (value - average) * (value - average); });

	data_t stddev = sqrt (squared_difference_sum / (data.size() - 1));
	data_t stderror = sqrt (squared_difference_sum) / data.size();
	data_t total_error;

	if (fp_cmp (systematic_error, 0)) {
		total_error = stderror;
	} else {
		total_error = sqrt (sq (stderror) + sq (systematic_error));
	}

	if (!output_terse) {
		std::cerr << "Dataset averaging:" << std::endl
		          << name << " < N=" << data.size() << " entries > = " << average << std::endl
		          << std::endl
		          << "Standard deviation (of the sample) = " << stddev << std::endl
		          << "Standard error (of the average) = " << stderror << std::endl
		          << std::endl;

		if (!fp_cmp (systematic_error, 0)) {
			std::cerr << "Systematic error = " << systematic_error << std::endl
			          << "Total error (of the average) = " << total_error << std::endl
			          << std::endl;
		}

		std::cerr << "Deviation range analysis:" << std::endl;

		size_t measurements_in_range[MAX_SIGMA] = { };

		for (size_t sigma = 1; sigma <= MAX_SIGMA; ++sigma) {
			measurements_in_range[sigma - 1] = std::count_if (data.begin(), data.end(), [average, stddev, sigma] (data_t value) { return fabsl (value - average) <= stddev * sigma; });
		}

		for (size_t sigma = 1; sigma <= MAX_SIGMA; ++sigma) {
			size_t count = measurements_in_range[sigma - 1];
			std::cerr << " " << sigma << " sigma (" << stddev * sigma << "): " << count << " measurements (" << (data_t) 100 * count / data.size() << "%)" << std::endl;
		}

		std::cerr << std::endl;
	} else {
		std::cerr << name << " <N=" << data.size() << "> = " << average << " ± " << total_error;

		if (!fp_cmp (systematic_error, 0)) {
			std::cerr << " (std. = " << stderror << "; syst. = " << systematic_error << ")";
		}

		std::cerr << std::endl;
	}

	if (output_machine) {
		std::cout << name_machine << " " << average << " " << total_error << std::endl;
	}
}
