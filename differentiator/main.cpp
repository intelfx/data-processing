#include "util.h"
#include "expr.h"
#include "lexer.h"

#include <sstream>

/*
 * Main data
 */

namespace {

// all variables being considered
std::vector<Variable> variables;

// expression evaluation results
data_t result_nominal;

} // anonymous namespace

/*
 * Bopulates the variable definition list by reading a file.
 */

void read_variables (const char* path)
{
	std::ifstream variable_file;
	open (variable_file, path);

	while (!(variable_file >> std::ws, variable_file.eof())) {
		parse_variable (variables, variable_file);
	}
}

int main (int argc, char** argv)
{
	if (argc < 3) {
		std::cerr << "This program takes two or more arguments." << std::endl
		          << "Usage: " << argv[0] << " <VARIABLE DEFINITION FILE> <C-FORMATTED EXPRESSION> [VARIABLE DEFINITION...]" << std::endl;
		exit (EXIT_FAILURE);
	}

	read_variables (argv[1]);

	for (int i = 3; i < argc; ++i) {
		std::string s (argv[i]);
		std::istringstream ss (s);
		ss.exceptions (std::istream::failbit | std::istream::badbit);
		parse_variable (variables, ss);
	}

	std::string expression (argv[2]);

	std::cout << "Parameters:" << std::endl;

	std::cout << " F(...) = " << argv[2] << std::endl;

	for (const Variable& v: variables) {
		std::cout << " " << v.name << " = " << v.value << " ± " << v.error << std::endl;
	}

	std::cout << std::endl
	          << "Results:" << std::endl
	          << " F    = " << result_nominal << std::endl
	          << std::endl;
}
