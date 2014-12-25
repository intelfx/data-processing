#include <differentiator/node.h>
#include <differentiator/visitor-calculate.h>
#include <differentiator/visitor-print.h>
#include <differentiator/visitor-latex.h>

#include "sstream"

int main (int argc, char** argv)
{
	if (argc < 3 || argc > 4) {
		std::cerr << "This program takes two or three arguments." << std::endl
		          << "Usage: " << argv[0] << " <N> <K1> [K2]" << std::endl;
	}

	rational_t N;
	int Ka[2];
	std::istringstream (argv[1]) >> N;
	std::istringstream (argv[2]) >> Ka[0];
	if (argc == 4) {
		std::istringstream (argv[3]) >> Ka[1];
	} else {
		Ka[1] = Ka[0];
	}

	Visitor::Print printer (std::cout, false);
	Visitor::Calculate calculate;
	Visitor::LaTeX::Document doc ("out.tex");

	for (int K = Ka[0]; K <= Ka[1]; ++K) {
		Node::MultiplicationDivision::Ptr top (new Node::MultiplicationDivision),
		                                  bottom (new Node::MultiplicationDivision),
		                                  fraction;

		top->add_child (Node::Base::Ptr (new Node::Value (N)), false);

		for (int i = 1; i < K; ++i) {
			Node::AdditionSubtraction::Ptr top_term (new Node::AdditionSubtraction);
			top_term->add_child (Node::Base::Ptr (new Node::Value (N)), false);
			top_term->add_child (Node::Base::Ptr (new Node::Value (i)), true);
			top->add_child (std::move (top_term), false);

			bottom->add_child (Node::Base::Ptr (new Node::Value (i + 1)), false);
		}

		if (bottom->children().empty()) {
			fraction = std::move (top);
		} else {
			fraction = Node::MultiplicationDivision::Ptr (new Node::MultiplicationDivision);
			fraction->add_child (std::move (top), false);
			fraction->add_child (std::move (bottom), true);
		}

		std::cout << "(" << N << " " << K << ") = "; fraction->accept (printer); std::cout << std::endl;

		boost::any value = fraction->accept (calculate);

		doc.print (static_cast<std::ostringstream&&> (std::ostringstream() << "\\dbinom {" << N << "} {" << K << "}").str(), fraction.get(), true, value);
	}
}
