#include "util-tree.h"
#include "expr.h"
#include "parser.h"
#include "visitor-print.h"
#include "visitor-calculate.h"

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

	std::string expression (argv[1]),
	            differentiation_variable (argv[2]);

	std::cout << "Parameters:" << std::endl;

	std::cout << " F(...) = " << expression << std::endl;

	for (auto it = variables.begin(); it != variables.end(); ++it) {
		std::cout << " " << it->first << std::endl;
	}

	Node::Base::Ptr tree = Parser (expression, variables).parse();
	Visitor::Print print_symbolic (std::cout, false);

	std::cout << std::endl
	          << "Value:" << std::endl;

	std::cout << "F = "; tree->accept (print_symbolic); std::cout << std::endl;

	std::cout << std::endl
	          << "Partial derivatives:" << std::endl;

	Node::Base::Ptr derivative = differentiate (tree, differentiation_variable);
	std::cout << "dF/d" << differentiation_variable << " = "; derivative->accept (print_symbolic); std::cout << std::endl;
}
