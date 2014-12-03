#include "util-tree.h"
#include "expr.h"
#include "parser.h"
#include "visitor-print.h"
#include "visitor-calculate.h"
#include "visitor-latex.h"

#include <iomanip>
#include <sstream>

int main (int argc, char** argv)
{
	if (argc < 3) {
		std::cerr << "This program takes two or more arguments." << std::endl
		          << "Usage: " << argv[0] << " <C-FORMATTED EXPRESSION> [DIFFERENTIATION VARIABLE] [OTHER VARIABLES...]" << std::endl;
		exit (EXIT_FAILURE);
	}

	insert_constants();

	for (int i = 2; i < argc; ++i) {
		variables.insert (Variable::make (argv[i], 0, 0));
	}

	bool latex_output = false;
	std::string output_name;
	std::string latex_file;

	if (char* file = getenv ("LATEX_FILE")) {
		if (file[0] != '\0') {
			latex_file = std::string (file);
			latex_output = true;
		} else {
			std::cerr << "WARNING: LATEX_FILE= variable is set but empty; set it to desired file name" << std::endl;
		}
	}

	if (char* name = getenv ("OUTPUT")) {
		if (name[0] != '\0') {
			output_name = std::string (name);
		} else {
			std::cerr << "WARNING: OUTPUT= variable is set but empty; set it to desired output variable name" << std::endl;
		}
	}

	std::string expression (argv[1]),
	            differentiation_variable (argv[2]);

	std::cout << "Parameters:" << std::endl;

	std::cout << " F(...) = " << expression << std::endl;

	for (auto it = variables.begin(); it != variables.end(); ++it) {
		std::cout << " " << it->first << std::endl;
	}

	Node::Base::Ptr tree = Parser (expression, variables).parse(),
	                simplified = simplify_tree (tree.get(), differentiation_variable);
	Visitor::Print print_symbolic (std::cout, false);

	std::cout << std::endl
	          << "Value:" << std::endl;

	std::cout << "F = "; tree->accept (print_symbolic); std::cout << " = " << std::endl;
	std::cout << "  = "; simplified->accept (print_symbolic); std::cout << std::endl;

	std::cout << std::endl
	          << "Partial derivatives:" << std::endl;

	Node::Base::Ptr derivative = differentiate (simplified.get(), differentiation_variable);
	std::cout << "dF/d" << differentiation_variable << " = "; derivative->accept (print_symbolic); std::cout << std::endl;

	if (latex_output) {
		if (output_name.empty()) {
			output_name = "F";
		}

		Visitor::LaTeX::Document document (latex_file.c_str());

		document.print (output_name, tree.get(), false, false);
		document.print (output_name, tree.get(), simplified.get());
		std::ostringstream derivative_name;
		derivative_name << "\\frac {\\partial " << output_name << "} {\\partial " << differentiation_variable << "}";
		document.print (derivative_name.str(), derivative.get(), false, false);
	}
}
