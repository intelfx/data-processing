#include "util.h"
#include "expr.h"
#include "lexer.h"
#include "parser.h"
#include "visitor.h"
#include "visitor-print.h"
#include "visitor-calculate.h"
#include "visitor-simplify.h"
#include "visitor-optimize.h"
#include "visitor-differentiate.h"

#include <sstream>

/*
 * Main data
 */

namespace {

// all variables being considered
VariableMap variables;

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

	for (auto it = variables.begin(); it != variables.end(); ++it) {
		std::cout << " " << it->first << " = " << it->second.value << " ± " << it->second.error << std::endl;
	}

	std::cout << "string: '" << expression << "'" << std::endl;
	std::cout << "lexems:" << std::endl;

	for (std::string lexem: Lexer (expression))
	{
		std::cout << " '" << lexem << "'" << std::endl;
	}

	Node::Base::Ptr tree = Parser (expression, variables).parse();
	std::cout << "tree: ";
	tree->Dump (std::cout);
	std::cout << std::endl;

	Node::Base::Ptr tree2 = tree->clone();
	std::cout << "tree-cloned: ";
	tree2->Dump (std::cout);
	std::cout << std::endl;

	tree = std::move (tree2);
	std::cout << "tree-moved: ";
	tree->Dump (std::cout);
	std::cout << std::endl;

	SimplifyVisitor simplifier ("x");
	Node::Base::Ptr simplified (boost::any_cast<Node::Base*> (tree->accept (simplifier)));
	std::cout << "simpl: ";
	simplified->Dump (std::cout);
	std::cout << std::endl;

	OptimizeVisitor optimizer;
	Node::Base::Ptr optimized (boost::any_cast<Node::Base*> (simplified->accept (optimizer)));
	std::cout << "opt: ";
	optimized->Dump (std::cout);
	std::cout << std::endl;

	DifferentiateVisitor differentiator ("x");
	Node::Base::Ptr differential (boost::any_cast<Node::Base*> (optimized->accept (differentiator)));
	std::cout << "diff: ";
	differential->Dump (std::cout);
	std::cout << std::endl;

	Node::Base::Ptr diff_simpl (boost::any_cast<Node::Base*> (differential->accept (simplifier)));
	std::cout << "d.simpl: ";
	diff_simpl->Dump (std::cout);
	std::cout << std::endl;

	PrintVisitor printer (std::cout);
	std::cout << "tree: ";
	tree->accept (printer);
	std::cout << std::endl;

	std::cout << "simpl: ";
	simplified->accept (printer);
	std::cout << std::endl;

	std::cout << "opt: ";
	optimized->accept (printer);
	std::cout << std::endl;

	std::cout << "diff: ";
	differential->accept (printer);
	std::cout << std::endl;

	std::cout << "d.simpl: ";
	diff_simpl->accept (printer);
	std::cout << std::endl;

	CalculateVisitor calculator;
	result_nominal = boost::any_cast<data_t> (optimized->accept (calculator));

	std::cout << std::endl
	          << "Results:" << std::endl
	          << " F    = " << result_nominal << std::endl
	          << std::endl;
}
