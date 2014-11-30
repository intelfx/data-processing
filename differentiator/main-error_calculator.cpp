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
		          << "Usage: " << argv[0] << " <VARIABLE DEFINITION FILE> <C-FORMATTED EXPRESSION> [VARIABLE DEFINITION...]" << std::endl;
		exit (EXIT_FAILURE);
	}

	std::string variable_name;
	bool verbose = !getenv ("TERSE");
	bool machine_output = false;

	if (char* output_variable = getenv ("OUTPUT")) {
		if (output_variable[0] != '\0') {
			variable_name = std::string (output_variable);
			machine_output = true;
		} else {
			std::cerr << "WARNING: OUTPUT= variable is set but empty; set it to desired variable name" << std::endl;
		}
	} else if (getenv ("OUTPUT_BARE")) {
		machine_output = true;
	}

	insert_constants();
	read_variables (argv[1]);

	for (int i = 3; i < argc; ++i) {
		std::istringstream ss (argv[i]);
		ss.exceptions (std::istream::failbit | std::istream::badbit);
		parse_variable (variables, ss);
	}

	std::string expression (argv[2]);

	if (verbose) {
		std::cerr << "Parameters:" << std::endl;

		std::cerr << " F(...) = " << expression << std::endl;

		for (auto it = variables.begin(); it != variables.end(); ++it) {
			std::cerr << " " << it->first << " = " << it->second.value << " ± " << it->second.error << std::endl;
		}
	}

	Node::Base::Ptr tree = Parser (expression, variables).parse();
	Visitor::Print print_symbolic (std::cerr, false),
	               print_substitute (std::cerr, true);
	Visitor::Calculate calculate;

	if (verbose) {
		std::cerr << std::endl
				<< "Value:" << std::endl;

		std::cerr << "F = "; tree->accept (print_symbolic); std::cerr << " =" << std::endl;
		tree = simplify_tree (tree);
		std::cerr << "  = "; tree->accept (print_symbolic); std::cerr << " =" << std::endl;
		std::cerr << "  = "; tree->accept (print_substitute); std::cerr << " =" << std::endl;
		std::cerr << "  = " << tree->accept_value (calculate) << std::endl;

		std::cerr << std::endl
		          << "Partial derivatives:" << std::endl;
	}

	Node::AdditionSubtraction::Ptr full_squared_error (new Node::AdditionSubtraction);

	for (auto it = variables.begin(); it != variables.end(); ++it) {
		Node::Variable::Ptr error (new Node::Variable (it->first, it->second, true));
		Node::Base::Ptr derivative = differentiate (tree, it->first);

		if (fp_cmp (error->value(), 0) ||
		    fp_cmp (derivative->accept_value (calculate), 0)) {
			continue;
		}

		if (verbose) {
			std::cerr << "   " << error->pretty_name() << " = " << error->value() << std::endl;
			std::cerr << "dF/d" << it->first << " = "; derivative->accept (print_symbolic); std::cerr << std::endl;
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

	Node::Base::Ptr error (std::move (full_error));
	error = simplify_tree (error);

	if (verbose) {
		std::cerr << std::endl
		          << "Total error:" << std::endl;
		std::cerr << "ΔF = "; error->accept (print_symbolic); std::cerr << " =" << std::endl;
		std::cerr << "   = "; error->accept (print_substitute); std::cerr << " =" << std::endl;
		std::cerr << "   = " << error->accept_value (calculate) << std::endl;
	} else {
		std::cerr << " F(...) = " << expression
		          << " F = " << tree->accept_value (calculate)
		          << " ΔF = " << error->accept_value (calculate) << std::endl;
	}

	if (machine_output) {
		if (!variable_name.empty()) {
			std::cout << variable_name << " ";
		}
		std::cout << tree->accept_value (calculate) << " " << error->accept_value (calculate) << std::endl;
	}
}
