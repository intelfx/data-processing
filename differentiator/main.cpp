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

#include <iomanip>
#include <sstream>

/*
 * Main data
 */

namespace {

// all variables being considered
VariableMap variables;

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

void testcase_visitors (Node::Base::Ptr tree, const std::string& partial_variable)
{
	std::cerr << "VISITORS TESTCASE" << std::endl;
	Node::Base::Ptr simplified = Visitor::Util::apply_transformation (tree, Visitor::Simplify (partial_variable));
	Node::Base::Ptr optimized = Visitor::Util::apply_transformation (simplified, Visitor::Optimize());
	Node::Base::Ptr differential = Visitor::Util::apply_transformation (optimized, Visitor::Differentiate (partial_variable));
	Node::Base::Ptr diff_simplified = Visitor::Util::apply_transformation (differential, Visitor::Simplify (partial_variable));
	Node::Base::Ptr diff_optimized = Visitor::Util::apply_transformation (diff_simplified, Visitor::Optimize());

	Visitor::Print printer (std::cerr, false);

	std::cerr << "initial:    "; tree->accept (printer); std::cerr << std::endl;
	std::cerr << "simplified: "; simplified->accept (printer); std::cerr << std::endl;
	std::cerr << "optimized:  "; optimized->accept (printer); std::cerr << std::endl;
	std::cerr << "diff(" << partial_variable << "): "; differential->accept (printer); std::cerr << std::endl;
	std::cerr << "diff.smpl.: "; diff_simplified->accept (printer); std::cerr << std::endl;
	std::cerr << "diff.opt.:  "; diff_optimized->accept (printer); std::cerr << std::endl;
}

void simplify_tree (Node::Base::Ptr& tree, const std::string& partial_variable)
{
	if (getenv ("SIMPLIFY")) {
		tree = Visitor::Util::apply_transformation (tree, Visitor::Simplify (partial_variable));
	} else {
		tree = Visitor::Util::apply_transformation (tree, Visitor::Simplify());
	}
	tree = Visitor::Util::apply_transformation (tree, Visitor::Optimize());
}

Node::Base::Ptr differentiate (Node::Base::Ptr tree, const std::string& partial_variable)
{
	simplify_tree (tree, partial_variable);
	tree = Visitor::Util::apply_transformation (tree, Visitor::Differentiate (partial_variable));
	simplify_tree (tree, partial_variable);
	return std::move (tree);
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

	std::string expression (argv[2]);

	std::cout << "Parameters:" << std::endl;

	std::cout << " F(...) = " << argv[2] << std::endl;

	for (auto it = variables.begin(); it != variables.end(); ++it) {
		std::cout << " " << it->first << " = " << it->second.value << " ± " << it->second.error << std::endl;
	}

	Node::Base::Ptr tree = Parser (expression, variables).parse();
	Visitor::Print print_symbolic (std::cout, false),
	               print_substitute (std::cout, true);
	Visitor::Calculate calculate;

	std::cout << std::endl
	          << "Value:" << std::endl;

	std::cout << "F = "; tree->accept (print_symbolic); std::cout << " =" << std::endl;
	std::cout << "  = "; tree->accept (print_substitute); std::cout << " =" << std::endl;
	std::cout << "  = " << boost::any_cast<data_t> (tree->accept (calculate)) << std::endl;

	std::cout << std::endl
	          << "Partial derivatives:" << std::endl;

	Node::AdditionSubtraction::Ptr full_squared_error (new Node::AdditionSubtraction);

	for (auto it = variables.begin(); it != variables.end(); ++it) {
		Node::Variable::Ptr error (new Node::Variable (it->first, it->second, true));
		Node::Base::Ptr derivative = differentiate (tree->clone(), it->first);

		std::cout << "   " << error->pretty_name() << " = " << error->value() << std::endl;
		std::cout << "dF/d" << it->first << " = "; derivative->accept (print_symbolic); std::cout << std::endl;


		if (fp_cmp (error->value(), 0) ||
		    fp_cmp (boost::any_cast<data_t> (derivative->accept (calculate)), 0)) {
			continue;
		}

		Node::Power::Ptr squared_derivative (new Node::Power);
		squared_derivative->add_child (std::move (derivative));
		squared_derivative->add_child (Node::Base::Ptr (new Node::Value (2)));

		Node::Power::Ptr squared_error (new Node::Power);
		squared_error->add_child (std::move (error));
		squared_error->add_child (Node::Base::Ptr (new Node::Value (2)));

		Node::MultiplicationDivision::Ptr partial_error (new Node::MultiplicationDivision);
		partial_error->add_child (std::move (squared_derivative), false);
		partial_error->add_child (std::move (squared_error), false);

		full_squared_error->add_child (std::move (partial_error), false);
	}

	Node::Power::Ptr full_error (new Node::Power);
	full_error->add_child (std::move (full_squared_error));
	full_error->add_child (Node::Base::Ptr (new Node::Value (0.5)));

	std::cout << std::endl
	          << "Total error:" << std::endl;
	std::cout << "ΔF = "; full_error->accept (print_symbolic); std::cout << " =" << std::endl;
	std::cout << "   = "; full_error->accept (print_substitute); std::cout << " =" << std::endl;
	std::cout << "   = " << boost::any_cast<data_t> (full_error->accept (calculate)) << std::endl;
}
