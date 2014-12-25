#include "util-tree.h"
#include "parser.h"
#include "visitor-print.h"
#include "visitor-calculate.h"
#include "visitor-latex.h"

#include <getopt.h>
#include <cassert>

#include <iomanip>

enum {
	ARG_MACHINE_OUTPUT          = 'm',
	ARG_LATEX_OUTPUT            = 'l',
	ARG_GLOBAL_VAR_NAME         = 'n',
	ARG_TERSE_OUTPUT            = 'q',
	ARG_ADD_VARIABLE            = 'v',
	ARG_ADD_VARIABLE_FRAC       = 'r',
	ARG_ADD_VARIABLE_NO_VALUE   = 'b',
	ARG_ADD_VARIABLE_FROM_FILE  = 'f',
	ARG_DERIVATIVE_ORDER        = 'o',
	ARG_MODE_DIFFERENTIATE      = 'D',
	ARG_MODE_FIND_ERROR         = 'E',
	ARG_MODE_SIMPLIFY           = 'S',
	ARG_MACHINE_OUTPUT_VAR_NAME = 0x100,
	ARG_LATEX_OUTPUT_VAR_NAME,
};

namespace {

const option option_array[] = {
	{ "machine",       no_argument,       nullptr, ARG_MACHINE_OUTPUT },
	{ "latex",         required_argument, nullptr, ARG_LATEX_OUTPUT },
	{ "name",          required_argument, nullptr, ARG_GLOBAL_VAR_NAME },
	{ "name-machine",  required_argument, nullptr, ARG_MACHINE_OUTPUT_VAR_NAME },
	{ "name-latex",    required_argument, nullptr, ARG_LATEX_OUTPUT_VAR_NAME },
	{ "terse",         no_argument,       nullptr, ARG_TERSE_OUTPUT },
	{ "var",           required_argument, nullptr, ARG_ADD_VARIABLE },
	{ "var-frac",      required_argument, nullptr, ARG_ADD_VARIABLE_FRAC },
	{ "var-bare",      required_argument, nullptr, ARG_ADD_VARIABLE_NO_VALUE },
	{ "var-file",      required_argument, nullptr, ARG_ADD_VARIABLE_FROM_FILE },
	{ "deriv-order",   required_argument, nullptr, ARG_DERIVATIVE_ORDER },
	{ "differentiate", required_argument, nullptr, ARG_MODE_DIFFERENTIATE },
	{ "error",         no_argument,       nullptr, ARG_MODE_FIND_ERROR },
	{ "simplify",      optional_argument, nullptr, ARG_MODE_SIMPLIFY },
	{ }
};

void usage (const char* name) {
	std::cerr << "Usage: " << name << " [-m|--machine] [-l|--latex FILE] [-q|--terse]" << std::endl
	          << "       [-n|--name NAME] [--name-machine NAME] [--name-latex NAME]" << std::endl
	          << "       [-v|--var VARIABLE ...] [-r|--var-frac VARIABLE ...] [-b|--var-bare VARIABLE ...] [-f|--var-file FILE ...]" << std::endl
	          << "       [-o|--deriv-order ORDER]" << std::endl
	          << "       [-D|--differentiate VARIABLE] [-E|--error] [-S|--simplify[=VARIABLE]] <EXPRESSION>" << std::endl;
	exit (EXIT_FAILURE);
}

void print_expression_aligned (std::ostream& out, const std::string& name, Node::Base* tree, Node::Base* simplified, const boost::any& value)
{
	static Visitor::Print print_symbolic (out, false),
	                      print_substitute (out, true);

	auto align = std::setw (name.length());

	out << name << " = "; tree->accept (print_symbolic); out << " =" << std::endl;
	if (simplified) {
		out << align << "" << " = "; simplified->accept (print_symbolic); out << " =" << std::endl;
	}
	out << align << "" << " = "; (simplified ? simplified : tree)->accept (print_substitute); out << " =" << std::endl;

	out << align << "" << " = ";
	if (!value.empty()) {
		any_to_ostream (out, value);
	} else {
		out << "(could not compute)";
	}
	out << std::endl;
}

void print_expression_terse (std::ostream& out, Node::Base* tree)
{
	static Visitor::Print print_symbolic (out, false);

	tree->accept (print_symbolic);
}

void print_value_terse (std::ostream& out, const boost::any& value)
{
	if (!value.empty()) {
		any_to_ostream (out, value);
	} else {
		out << "(could not compute)";
	}
}

struct Expression
{
	Node::Base::Ptr tree;
	boost::any value;

	void compute (const char* explanation)
	{
		static Visitor::Calculate calculate;

		value = tree->accept (calculate);

		if (value.empty()) {
			std::cerr << "Warning: could not compute " << explanation << std::endl
			          << std::endl;
		}
	}
};

struct Differential
{
	std::string variable;
	unsigned order;
	mutable Expression expression;

	Differential (std::string v, unsigned o)
	: variable (std::move (v))
	, order (o)
	{
	}

	bool operator< (const Differential& rhs) const
	{
		return (variable < rhs.variable) || (order < rhs.order);
	}

	bool operator== (const Differential& rhs) const
	{
		return (variable == rhs.variable) && (order == rhs.order);
	}
};

} // anonymous namespace

int main (int argc, char** argv)
{
	enum class Task {
		None = 0,
		Differentiate,
		CalculateError,
		Simplify
	};

	struct {
		struct {
			struct {
				bool enabled;
				std::string name;
			} machine;

			struct {
				bool enabled;
				std::string name;
				std::string file;
			} latex;

			struct {
				bool terse;
				std::string name;
			} common;
		} output;

		struct {
			Task type;

			struct {
				std::string variable;
				unsigned order = 1;
			} differentiate;

			struct {
				std::string variable;
			} simplify;
		} task;

		std::string expression;
	} parameters = { };

	insert_constants();

	/*
	 * Parse the command-line arguments.
	 */

	int option;
	while ((option = getopt_long (argc, argv, "ml:n:qv:r:b:f:o:D:ES:", option_array, nullptr)) != -1) {
		switch (option) {
		case ARG_MACHINE_OUTPUT:
			parameters.output.machine.enabled = true;
			break;

		case ARG_LATEX_OUTPUT:
			parameters.output.latex.enabled = true;
			parameters.output.latex.file = optarg;
			break;

		case ARG_GLOBAL_VAR_NAME:
			parameters.output.common.name = optarg;
			break;

		case ARG_MACHINE_OUTPUT_VAR_NAME:
			parameters.output.machine.name = optarg;
			break;

		case ARG_LATEX_OUTPUT_VAR_NAME:
			parameters.output.latex.name = optarg;
			break;

		case ARG_TERSE_OUTPUT:
			parameters.output.common.terse = true;
			break;

		case ARG_ADD_VARIABLE: {
			std::istringstream ss (optarg);
			parse_variable<data_t> (variables, ss);
			ss >> std::ws;

			if (ss.fail() || !ss.eof()) {
				std::ostringstream reason;
				reason << "Wrong variable specifier: '" << optarg << "'";
				throw std::runtime_error (reason.str());
			}

			break;
		}

		case ARG_ADD_VARIABLE_FRAC: {
			std::istringstream ss (optarg);
			parse_variable<rational_t> (variables, ss);
			ss >> std::ws;

			if (ss.fail() || !ss.eof()) {
				std::ostringstream reason;
				reason << "Wrong rational variable specifier: '" << optarg << "'";
				throw std::runtime_error (reason.str());
			}

			break;
		}

		case ARG_ADD_VARIABLE_NO_VALUE: {
			std::istringstream ss (optarg);
			parse_variable<void> (variables, ss);
			ss >> std::ws;

			if (ss.fail() || !ss.eof()) {
				std::ostringstream reason;
				reason << "Wrong bare variable specifier: '" << optarg << "'";
				throw std::runtime_error (reason.str());
			}

			break;
		}

		case ARG_ADD_VARIABLE_FROM_FILE:
			parse_variables_from_file<data_t> (variables, optarg);
			break;

		case ARG_DERIVATIVE_ORDER: {
			std::istringstream ss (optarg);
			ss >> parameters.task.differentiate.order >> std::ws;

			if (ss.fail() || !ss.eof()) {
				std::ostringstream reason;
				reason << "Could not parse the derivative order: '" << optarg << "'";
				throw std::runtime_error (reason.str());
			}

			break;
		}

		case ARG_MODE_DIFFERENTIATE:
			assert (parameters.task.type == Task::None);
			parameters.task.type = Task::Differentiate;
			parameters.task.differentiate.variable = optarg;
			break;

		case ARG_MODE_FIND_ERROR:
			assert (parameters.task.type == Task::None);
			parameters.task.type = Task::CalculateError;
			break;

		case ARG_MODE_SIMPLIFY:
			assert (parameters.task.type == Task::None);
			parameters.task.type = Task::Simplify;
			if (optarg && optarg[0]) {
				parameters.task.simplify.variable = optarg;
			}
			break;

		default:
			usage (argv[0]);
		}
	}

	/*
	 * Verify arguments' consistency.
	 */

	if (optind >= argc) {
		std::cerr << "Expression expected." << std::endl;
		usage (argv[0]);
	} else if (optind + 1 < argc) {
		std::cerr << "Only one expression expected." << std::endl;
		usage (argv[0]);
	}

	if (parameters.task.type == Task::None) {
		std::cerr << "One of operation modes ('-D', '-E' or '-S') is expected." << std::endl;
		usage (argv[0]);
	}

	/*
	 * Init remaining parameters and configure default values.
	 */

	parameters.expression = argv[optind];

	if (parameters.output.common.name.empty()) {
		parameters.output.common.name = "F";
	}

	if (parameters.output.latex.name.empty()) {
		parameters.output.latex.name = parameters.output.common.name;
	}

	if (parameters.output.machine.name.empty()) {
		parameters.output.machine.name = parameters.output.common.name;
	}

	/*
	 * Dump the input data to stderr (if not quiet).
	 */

	if (!parameters.output.common.terse) {
		std::cerr << "Input data:" << std::endl;

		std::string name = parameters.output.common.name + "(...)";
		auto align = std::setw (name.length());

		std::cerr << name << " = " << parameters.expression << std::endl;

		for (Variable::Map::value_type& var: variables) {
			std::cerr << align << var.first;
			if (!var.second.value.empty()) {
				std::cerr << " = "; any_to_ostream (std::cerr, var.second.value);
			}
			if (!var.second.no_error()) {
				std::cerr << " ± "; any_to_ostream (std::cerr, var.second.error);
			}
			std::cerr << std::endl;
		}

		std::cerr << std::endl;
	}

	/*
	 * Parse the expression, simplify it and compute.
	 * We store original (parsed) and simplified trees separately and check if there was anything to simplify.
	 */

	Node::Base::Ptr expression_raw = Parser (parameters.expression, variables).parse();
	Expression expression;
	bool expression_simplified;

	if (parameters.task.simplify.variable.empty()) {
		if (!parameters.output.common.terse) {
			std::cerr << "Will simplify the expression generally." << std::endl
			          << std::endl;
		}
		expression.tree = simplify_tree (expression_raw.get());
	} else {
		if (!parameters.output.common.terse) {
			std::cerr << "Will simplify the expression for variable '" << parameters.task.simplify.variable << "'." << std::endl
			          << std::endl;
		}
		expression.tree = simplify_tree (expression_raw.get(), parameters.task.simplify.variable);
	}

	expression_simplified = !expression.tree->compare (expression_raw);

	expression.compute ("expression value");

	/*
	 * Calculate the differentials and error (if requested).
	 */
	std::set<Differential> differentials;

	/*
	 * Collect all differentiation variables alongside with placeholders for the differential expressions.
	 */

	switch (parameters.task.type) {
	case Task::Simplify:
		break;

	case Task::Differentiate:
		differentials.insert (Differential (parameters.task.differentiate.variable, parameters.task.differentiate.order));
		break;

	case Task::CalculateError:
		for (Variable::Map::value_type& var: variables) {
			differentials.insert (Differential (var.first, 1));
		}
		break;

	default:
		throw std::logic_error ("Wrong task type selected.");
	}

	/*
	 * Calculate the differentials, erasing entries for constant-zero results.
	 * A constant-zero result means that the source expression does not contain the differentiation variable.
	 * (FIXME: really? it'd be better to collect usage flags for all variables during parsing.)
	 */

	for (auto it = differentials.begin(); it != differentials.end(); ) {
		const Differential& d = *it;

		d.expression.tree = differentiate (expression.tree.get(), d.variable, d.order);
		d.expression.compute (static_cast<std::ostringstream&&> (std::ostringstream() << "differential of order " << d.order << " for variable '" << d.variable << "'").str().c_str());

		Node::Value* differential_value = dynamic_cast<Node::Value*> (d.expression.tree.get());
		if (differential_value && (differential_value->value() == 0)) {
			differentials.erase (it++);
		} else {
			++it;
		}
	}

	/*
	 * Build and compute the error (if requested).
	 */

	Expression error;

	if (parameters.task.type == Task::CalculateError) {
		Node::AdditionSubtraction::Ptr error_sq_sum (new Node::AdditionSubtraction);

		for (const Differential& d: differentials) {
			if (d.order != 1) {
				std::ostringstream reason;
				reason << "Differential for variable '" << d.variable << "' has unexpected order (" << d.order << ") while computing error";
				throw std::logic_error (reason.str());
			}

			auto var = variables.find (d.variable);
			if (var == variables.end()) {
				std::ostringstream reason;
				reason << "Cannot find variable '" << d.variable << "' while computing error, despite differential exists";
				throw std::logic_error (reason.str());
			}

			if (var->second.no_error()) {
				continue;
			}

			Node::Variable::Ptr error (new Node::Variable (var->first, var->second, true));

			// dF/dx * error(x)
			Node::MultiplicationDivision::Ptr partial (new Node::MultiplicationDivision);
			partial->add_child (d.expression.tree->clone(), false);
			partial->add_child (std::move (error), false);

			// (dF/dx * error(x))^2
			Node::Power::Ptr partial_sq (new Node::Power);
			partial_sq->add_child (std::move (partial));
			partial_sq->add_child (Node::Base::Ptr (new Node::Value (2)));

			error_sq_sum->add_child (std::move (partial_sq), false);
		}

		Node::Power::Ptr error_sqrt (new Node::Power);
		error_sqrt->add_child (std::move (error_sq_sum));
		error_sqrt->add_child (Node::Base::Ptr (new Node::Value (rational_t (1, 2))));

		error.tree = simplify_tree (error_sqrt.get());

		error.compute ("expression error value");
	}

	/*
	 * Init various printers and output engines.
	 */

	typedef std::unique_ptr<Visitor::LaTeX::Document> LaTeXDocumentPtr;
	LaTeXDocumentPtr latex_document;

	if (parameters.output.latex.enabled) {
		latex_document = LaTeXDocumentPtr (new Visitor::LaTeX::Document (parameters.output.latex.file.c_str()));
	}

	/*
	 * Write everything to stderr (if not quiet).
	 */

	if (!parameters.output.common.terse) {
		std::cerr << "Calculation:" << std::endl;

		print_expression_aligned (std::cerr,
		                          parameters.output.common.name,
		                          expression_raw.get(),
		                          expression_simplified ? expression.tree.get()
		                                                : nullptr,
		                          expression.value);

		std::cerr << std::endl;

		if (!differentials.empty()) {
			std::cerr << "Partial derivatives:" << std::endl;
		}

		for (const Differential& d: differentials) {
			std::string name;

			if (d.order == 1) {
				name = "d" + parameters.output.common.name + "/d" + d.variable;
			} else {
				std::ostringstream builder;
				builder << "d^" << d.order << parameters.output.common.name << "/d" << d.variable << "^" << d.order;
				name = builder.str();
			}

			print_expression_aligned (std::cerr,
			                          name,
			                          d.expression.tree.get(),
			                          nullptr,
			                          d.expression.value);

			std::cerr << std::endl;
		}

		if (parameters.task.type == Task::CalculateError) {
			std::cerr << "Error:" << std::endl;

			std::string name = "Δ" + parameters.output.common.name;
			print_expression_aligned (std::cerr,
			                          name,
			                          error.tree.get(),
			                          nullptr,
			                          error.value);

			std::cerr << std::endl;
		}
	} else {

		std::cerr << parameters.output.common.name << "(...) = ";

		print_expression_terse (std::cerr,
		                        expression_raw.get());

		std::cerr << " = ";

		print_value_terse (std::cerr,
		                   expression.value);

		if (parameters.task.type == Task::CalculateError) {
			std::cerr << " ± ";

			print_value_terse (std::cerr,
			                   error.value);
		}

		std::cerr << std::endl;
	}

	/*
	 * Write everything to the LaTeX document (if enabled).
	 */

	if (parameters.output.latex.enabled) {
		if (expression_simplified) {
			latex_document->print (parameters.output.latex.name,
			                       expression_raw.get(),
			                       expression.tree.get());
		}

		latex_document->print (parameters.output.latex.name,
		                       expression.tree.get(),
		                       true,
		                       expression.value);

		for (const Differential& d: differentials) {
			std::string name;

			if (d.order == 1) {
				name = "\\frac {\\partial " + Visitor::LaTeX::prepare_name (parameters.output.latex.name) + "}"
			                 " {\\partial " + Visitor::LaTeX::prepare_name (d.variable) + "}";
			} else {
				std::ostringstream builder;
				builder << "\\frac {\\partial ^ {" << d.order << "} " << Visitor::LaTeX::prepare_name (parameters.output.latex.name) << "}"
				        <<        "{\\partial " << d.variable << " ^ {" << d.order << "} }";
				name = builder.str();
			}

			latex_document->print (name,
			                       d.expression.tree.get(),
			                       true,
			                       d.expression.value);
		}

		if (parameters.task.type == Task::CalculateError) {
			latex_document->print ("\\sigma " + parameters.output.latex.name,
			                       error.tree.get(),
			                       true,
			                       error.value);
		}
	}

	/*
	 * Finally, write the machine-readable output (if enabled).
	 */

	if (parameters.output.machine.enabled) {
		std::cout << parameters.output.machine.name << " " << (expression.value.empty() ? any_to_fp (expression.value) : NAN);

		if (parameters.task.type == Task::CalculateError) {
			std::cout << " " << (error.value.empty() ? any_to_fp (error.value) : NAN);
		}

		std::cout << std::endl;
	}
}
