#include "util.h"
#include "expr.h"
#include <cctype>
#include <dlfcn.h>

/*
 * Ancillary data types
 */

typedef data_t(*Func)();

/*
 * Defines a single variable with name, value, error and mode of error application.
 */

struct AugmentedVariable : Variable
{
	std::string name;
	enum Mode
	{
		NONE = 0,
		MINUS_ERROR,
		PLUS_ERROR
	} mode;

	typedef std::vector<AugmentedVariable> Vec;

	static Vec::value_type read (std::istream& in)
	{
		AugmentedVariable v;
		in >> v.name >> v.value >> v.error;
		v.do_not_substitute = false;
		v.mode = NONE;
		return v;
	}
};

/*
 * Adds a single named variable definition.
 */

inline void parse_variable (AugmentedVariable::Vec& vec, std::istream& in)
{
	vec.push_back (AugmentedVariable::read (in));
}

/*
 * Constants
 */

namespace {

const char* compiler = "${CXX:-clang++} ${CXXFLAGS:--O3 -Wall -Wextra -Werror} -shared -fPIC";

} // anonymous namespace

/*
 * Main data
 */

namespace {

// all variables being considered
AugmentedVariable::Vec variables;

// expression evaluation results 
data_t result_nominal, result_min, result_max;

// expression function
Func F;

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

/*
 * Generates a C++ source evaluating given expression,
 * compiles it into a dynamic library, loads it and
 * returns the function's address.
 */

Func compile_and_load_expression (const char* expression)
{
	/*
	 * Generate an "unique" name for temporary source and binary files.
	 * We use the expression itself, with non-alphanumeric characters
	 * replaced with underscores.
	 */

	std::string file_name_prefix (expression);

	for (char& c: file_name_prefix) {
		if (!std::isalnum (c)) {
			c = '_';
		}
	}

	/*
	 * Generate complete names for said temporary files.
	 * The binary must have an explicit "current directory" relative
	 * path.
	 */

	std::string src_file_name ("module_"),
	            binary_file_name ("./");

	src_file_name.append (file_name_prefix); // module_<name>
	binary_file_name.append (src_file_name); // ./module_<name>
	src_file_name.append (".cpp");           // module_<name>.cpp
	binary_file_name.append (".so");         // ./module_<name>.so

	/*
	 * Generate the source.
	 */

	std::ofstream src_file;
	open (src_file, src_file_name.c_str());

	/*
	 * Boilerplate...
	 */

	src_file << "#include <cstddef>\n"
	         << "#include <cmath>\n"
	         << "\n"
	         << "typedef long double data_t;\n"
	         << "\n"
	         << "extern data_t get_variable (size_t idx);\n"
	         << "\n"
	         << "template <typename T> T sq (T arg) { return arg * arg; }\n"
	         << "\n";

	/*
	 * ...defines for variables...
	 * (each variable's name is replaced with a call to the getter function)
	 */

	for (size_t i = 0; i < variables.size(); ++i) {
		src_file << "#define " << variables[i].name << " get_variable(" << i << ")\n";
	}

	/*
	 * ...and the evaluation function itself.
	 */

	src_file << "\n"
	         << "extern \"C\" data_t F() { return " << expression << "; }\n";

	src_file.close();

	/*
	 * Build the compiler command line.
	 */

	std::string compiler_call_string (compiler);
	compiler_call_string.append (" '");
	compiler_call_string.append (src_file_name);
	compiler_call_string.append ("' -o '");
	compiler_call_string.append (binary_file_name);
	compiler_call_string.append ("'");

	/*
	 * Call the compiler.
	 */

	void* ret = nullptr;
	int compiler_exit_code = system (compiler_call_string.c_str());

	if (compiler_exit_code != 0) {
		std::cerr << "Compilation failed: " << compiler_exit_code << std::endl;
	} else {
		/* Compiled OK, open the library. */
		void* module = dlopen (binary_file_name.c_str(), RTLD_NOW);
		if (!module) {
			std::cerr << "Could not dlopen(): " << dlerror() << std::endl;
		} else {
			/* Opened OK, get the evaluation function's address. */
			ret = dlsym (module, "F");
			if (!ret) {
				std::cerr << "Could not dlsym(): " << dlerror() << std::endl;
			}
		}

		/*
		 * FIXME: we do not close the library handle.
		 */
	}

	return (Func) ret;
}

/*
 * Main recursive worker function. For each variable X, two values are
 * substituted in order: "X + ΔX" and "X - ΔX". The expression is evaluated
 * for all combinations of such substitutions.
 */

void process (size_t idx)
{
	if (idx >= variables.size()) {
		// end of recursion -- evaluate the expression
		data_t result = F();
		result_min = std::min (result_min, result);
		result_max = std::max (result_max, result);
	} else {
		// perform substitutions for current variable and continue recursion
		if (variables[idx].no_error()) {
			variables[idx].mode = AugmentedVariable::NONE;
			process (idx + 1);
		} else {
			variables[idx].mode = AugmentedVariable::MINUS_ERROR;
			process (idx + 1);
			variables[idx].mode = AugmentedVariable::PLUS_ERROR;
			process (idx + 1);
		}
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
		std::istringstream ss (argv[i]);
		ss.exceptions (std::istream::failbit | std::istream::badbit);
		parse_variable (variables, ss);
	}

	F = compile_and_load_expression (argv[2]);

	if (!F) {
		std::cerr << "Failed to compile expression" << std::endl;
		exit (EXIT_FAILURE);
	}

	result_nominal = F();
	result_min = +INFINITY;
	result_max = -INFINITY;

	process (0);

	std::cout << "Parameters:" << std::endl;

	std::cout << " F(...) = " << argv[2] << std::endl;

	for (const AugmentedVariable& v: variables) {
		std::cout << " " << v.name << " = " << v.value << " ± " << v.error << std::endl;
	}

	std::cout << std::endl
	          << "Results:" << std::endl
	          << " F    = " << result_nominal << std::endl
	          << " Fmin = " << result_min << std::endl
	          << " Fmax = " << result_max << std::endl
	          << " ΔF-  = " << result_nominal - result_min << std::endl
	          << " ΔF+  = " << result_max - result_nominal << std::endl
	          << std::endl;
}

/*
 * Returns the value substituted for i-th variable.
 * The evaluation function calls this via per-variable defines.
 */

data_t get_variable (size_t idx)
{
	AugmentedVariable& v = variables.at (idx);
	switch (v.mode) {
	case AugmentedVariable::NONE:        return v.value;
	case AugmentedVariable::MINUS_ERROR: return v.value - v.error;
	case AugmentedVariable::PLUS_ERROR:  return v.value + v.error;
	default: std::cerr << "Incorrect variable mode" << std::endl; abort();
	}
}
