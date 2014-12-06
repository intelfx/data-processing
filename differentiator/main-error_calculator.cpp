#include "util-tree.h"
#include "expr.h"
#include "parser.h"
#include "visitor-print.h"
#include "visitor-calculate.h"
#include "visitor-latex.h"

#include <iomanip>

int main (int argc, char** argv)
{
	if (argc < 3) {
		std::cerr << "This program takes two or more arguments." << std::endl
		          << "Usage: " << argv[0] << " <VARIABLE DEFINITION FILE> <C-FORMATTED EXPRESSION> [VARIABLE DEFINITION...]" << std::endl;
		exit (EXIT_FAILURE);
	}

	std::string variable_name;
	std::string latex_file;
	std::string latex_name;
	bool verbose = !getenv ("TERSE");
	bool machine_output = false;
	bool latex_output = false;

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

	if (char* file = getenv ("LATEX_FILE")) {
		if (file[0] != '\0') {
			latex_file = std::string (file);
			latex_output = true;
		} else {
			std::cerr << "WARNING: LATEX_FILE= variable is set but empty; set it to desired file name" << std::endl;
		}
	}

	if (char* name = getenv ("LATEX_NAME")) {
		if (name[0] != '\0') {
			latex_name = std::string (name);
		} else {
			std::cerr << "WARNING: LATEX_NAME= variable is set but empty; set it to desired variable name" << std::endl;
		}
	} else {
		latex_name = variable_name;
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

	Node::Base::Ptr tree = Parser (expression, variables).parse(),
	                simplified = simplify_tree (tree.get());
	Visitor::Print print_symbolic (std::cerr, false),
	               print_substitute (std::cerr, true);
	Visitor::Calculate calculate;

	if (verbose) {
		std::cerr << std::endl
				<< "Value:" << std::endl;

		std::cerr << "F = "; tree->accept (print_symbolic); std::cerr << " =" << std::endl;
		std::cerr << "  = "; simplified->accept (print_symbolic); std::cerr << " =" << std::endl;
		std::cerr << "  = "; simplified->accept (print_substitute); std::cerr << " =" << std::endl;
		std::cerr << "  = " << simplified->accept_value (calculate) << std::endl;

		std::cerr << std::endl
		          << "Partial derivatives:" << std::endl;
	}

	Node::AdditionSubtraction::Ptr full_squared_error (new Node::AdditionSubtraction);

	for (auto it = variables.begin(); it != variables.end(); ++it) {
		Node::Variable::Ptr error (new Node::Variable (it->first, it->second, true));
		Node::Base::Ptr derivative = differentiate (simplified.get(), it->first);

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

	Node::Power::Ptr error (new Node::Power);
	error->add_child (std::move (full_squared_error));
	error->add_child (Node::Base::Ptr (new Node::Value (0.5)));

	Node::Base::Ptr error_simplified = simplify_tree (error.get());

	if (verbose) {
		std::cerr << std::endl
		          << "Total error:" << std::endl;
		std::cerr << "ΔF = "; error_simplified->accept (print_symbolic); std::cerr << " =" << std::endl;
		std::cerr << "   = "; error_simplified->accept (print_substitute); std::cerr << " =" << std::endl;
		std::cerr << "   = " << error_simplified->accept_value (calculate) << std::endl;
	} else {
		std::cerr << " F(...) = " << expression
		          << " F = " << simplified->accept_value (calculate)
		          << " ΔF = " << error_simplified->accept_value (calculate) << std::endl;
	}

	if (machine_output) {
		if (!variable_name.empty()) {
			std::cout << variable_name << " ";
		}
		std::cout << simplified->accept_value (calculate) << " " << error_simplified->accept_value (calculate) << std::endl;
	}

	if (latex_output) {
		if (variable_name.empty()) {
			variable_name = "F";
		}

		Visitor::LaTeX::Document document (latex_file.c_str());

		data_t value = simplified->accept_value (calculate),
		       error_value = error_simplified->accept_value (calculate);

		document.print (latex_name, tree.get(), true, &value);
		document.print (latex_name, tree.get(), simplified.get());
		document.print (std::string ("\\sigma ") + latex_name, error_simplified.get(), true, &error_value);
	}
}
