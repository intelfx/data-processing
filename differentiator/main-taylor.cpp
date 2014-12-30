#include "util-tree.h"
#include "parser.h"
#include "visitor-print.h"
#include "visitor-calculate.h"

struct Expression
{
	Node::Base::Ptr tree;
	boost::any value;

	void compute (const char* explanation, bool quiet)
	{
		static Visitor::Calculate calculate;

		value = tree->accept (calculate);

		if (value.empty() && !quiet) {
			std::cerr << "Warning: could not compute " << explanation << std::endl
			          << std::endl;
		}
	}
};

int main()
{
	unsigned N;
	std::string expression_text;
	std::cin >> N >> std::ws;
	std::getline (std::cin, expression_text);
	expression_text = "1/(" + expression_text + ")";

	variables.insert (Variable::make<rational_t> ("x", rational_t (0), rational_t (0), false));

	Node::Base::Ptr expression = Parser (expression_text, variables).parse();
	expression = simplify_tree (expression.get());

	std::unique_ptr<rational_t[]> series_coefficients (new rational_t[N+1]);

	Visitor::Calculate calculator;
	Node::Base::Ptr derivative;
	integer_t denominator = 1;
	unsigned current_order = 0;

	derivative = expression->clone();

	for (;;) {

		/*
		 * Add the next term to the Taylor series, if it is non-zero.
		 */

		boost::any derivative_value = derivative->accept (calculator);

		if (!any_isa<rational_t> (derivative_value)) {
			std::ostringstream reason;
			reason << "Cannot build the Taylor series: derivative of order " << current_order << " is not rational or cannot be computed";
			throw std::runtime_error (reason.str());
		}

		rational_t multiplier = any_to_rational (derivative_value) / denominator;
		series_coefficients[current_order] = multiplier;

		if (multiplier.numerator() != 0) {
			if (current_order == 0) {
				rational_to_ostream (std::cout, multiplier);
			} else {
				if (multiplier == 1) {
					std::cout << " +";
				} else if (multiplier == -1) {
					std::cout << " -";
				} else if (multiplier > 0) {
					std::cout << " + "; rational_to_ostream (std::cout, multiplier);
				} else if (multiplier < 0) {
					std::cout << " - "; rational_to_ostream (std::cout, -multiplier);
				} else {
					abort();
				}
				std::cout << " x";
				if (current_order > 1) {
					std::cout << "^" << current_order;
				}
			}
		}

		/*
		 * Check if the series has enough length.
		 */

		if (current_order == N) {
			break;
		}

		/*
		 * Build the next derivative.
		 */

		derivative = differentiate (derivative.get(), "x");

		++current_order;
		denominator *= current_order;
	}

	std::cout << std::endl;

	return 0;
}
