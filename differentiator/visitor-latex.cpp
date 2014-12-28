#include "visitor-latex.h"
#include "visitor-calculate.h"

#include <regex>

namespace {

const std::string border_width = "2pt";
const std::string extra_left_border_width = "0pt";
const std::string varwidth_limit = "30cm";

void rational_to_latex (std::ostream& out, const rational_t& obj)
{
	if (obj.denominator() == 1) {
		out << obj.numerator();
	} else {
		out << "\\frac {" << obj.numerator() << "} {" << obj.denominator() << "}";
	}
}

void fp_to_latex (std::ostream& out, data_t value)
{
	std::ostringstream ss;
	ss << value;

	std::string str = ss.str();

	if (str.find ('e') != std::string::npos) {
		std::regex exponent_replace_re ("e(-)?0*([0-9]*)", std::regex::extended);
		out << "(" << std::regex_replace (str, exponent_replace_re, " * 10^{\\1\\2}", std::regex_constants::format_sed) << ")";
	} else {
		out << str;
	}
}

void any_to_latex (std::ostream& out, const boost::any& obj)
{
	if (const rational_t* val = boost::any_cast<rational_t> (&obj)) {
		rational_to_latex (out, *val);
	} else {
		fp_to_latex (out, any_to_fp (obj));
	}
}

void any_to_latex_to_fp (std::ostream& out, const boost::any& obj)
{
	if (const rational_t* val = boost::any_cast<rational_t> (&obj)) {
		rational_to_latex (out, *val);
		out << " \\approx ";
		fp_to_latex (out, to_fp (*val));
	} else {
		fp_to_latex (out, any_to_fp (obj));
	}
}

} // anonymous namespace

namespace Visitor
{

LaTeX::Document::Document (const char* file)
: first_in_document_ (true)
{
	open (stream_, file);

	std::ostringstream latex_render_command_builder;
	latex_render_command_builder << "pdflatex -halt-on-error " << "\"" << file << "\" >/dev/null";
	latex_render_command_ = latex_render_command_builder.str();

	write_header();
}

LaTeX::Document::~Document()
{
	write_footer();
	stream_.close();

	int exitcode = system (latex_render_command_.c_str());
	if (exitcode) {
		std::ostringstream err;
		err << "LaTeX renderer exited with non-zero exit code: " << exitcode;
		throw std::runtime_error (err.str());
	}
}

void LaTeX::Document::write_header()
{
	stream_ << "\\documentclass[border=" << border_width << ",varwidth=" << varwidth_limit << "]{standalone}" << std::endl
	        << "\\usepackage[fleqn]{amsmath}" << std::endl
	        << "\\setlength {\\mathindent} {" << extra_left_border_width << "}" << std::endl
	        << "\\begin{document}" << std::endl;

	stream_ << "\\begin{align*}" << std::endl;

	first_in_document_ = true;
}

void LaTeX::Document::write_footer()
{
	stream_ << "}" << std::endl
	        << "\\end{align*}" << std::endl
	        << "\\end{document}" << std::endl;
}

LaTeX::LaTeX (std::ostream& stream, bool substitute)
: Print (stream, substitute, "\\left(", "\\right)")
{
}

void LaTeX::Document::write_linebreaks()
{
	if (!first_in_document_) {
		stream_ << " \\\\" << std::endl;
	} else {
		first_in_document_ = false;
	}
}

void LaTeX::Document::write_equation_header (const std::string& name)
{
	if (!first_in_document_) {
		stream_ << "}" << std::endl;
	}

	stream_ << prepare_name (name);

	first_in_document_ = true;
}

void LaTeX::Document::print_expression (Node::Base* tree, Base& visitor)
{
	write_linebreaks();

	stream_ << " & ="; tree->accept (visitor);
}

void LaTeX::Document::print_value (const boost::any& value)
{
	write_linebreaks();

	stream_ << " & =";
	any_to_latex_to_fp (stream_, value);
}

void LaTeX::Document::write_equation_footer()
{
	write_linebreaks();

	stream_ << "\\intertext{" << std::endl;
}

void LaTeX::Document::print (const std::string& name, Node::Base* tree, bool substitute, const boost::any& value)
{
	LaTeX printer_literal (stream_, false),
	      printer_substituting (stream_, true);

	write_equation_header (name);

	print_expression (tree, printer_literal);

	if (substitute) {
		print_expression (tree, printer_substituting);
	}

	if (!value.empty()) {
		print_value (value);
	}

	write_equation_footer();
}

void LaTeX::Document::print (const std::string& name, Node::Base* tree, Node::Base* simplified)
{
	LaTeX printer_literal (stream_, false);

	write_equation_header (name);

	print_expression (tree, printer_literal);
	print_expression (simplified, printer_literal);

	write_equation_footer();
}

boost::any LaTeX::visit (Node::Value& node)
{
	rational_to_latex (stream_, node.value());

	return boost::any();
}

boost::any LaTeX::visit (Node::Variable& node)
{
	if (substitute_ && node.can_be_substituted() && !node.value().empty()) {
		any_to_latex (stream_, node.value());
	} else {
		if (node.is_error()) {
			stream_ << "\\sigma ";
		}
		stream_ << prepare_name (node.name());
	}

	return boost::any();
}

boost::any LaTeX::visit (Node::Power& node)
{
	auto child = node.children().begin();

	Node::Base::Ptr &base = *child++,
	                &exponent = *child++;

	Node::Value* exponent_value = dynamic_cast<Node::Value*> (exponent.get());

	if (exponent_value &&
	    exponent_value->value().numerator() == 1) {
		integer_t denominator = exponent_value->value().denominator();
		if (denominator == 2) {
			stream_ << "\\sqrt {";
		} else {
			stream_ << "\\sqrt[" << denominator << "] {";
		}
		/* skip parenthesizing anything because { .. } are effectively parentheses */
		base->accept (*this);
		stream_ << "}";
	} else {
		parenthesized_visit (node, base);
		stream_ << "^{";
		/* skip parenthesizing anything because { .. } are effectively parentheses */
		exponent->accept (*this);
		stream_ << "}";
	}

	return boost::any();
}

boost::any LaTeX::visit (Node::MultiplicationDivision& node)
{
	size_t mul_count = 0, div_count = 0;

	for (auto& child: node.children()) {
		if (child.tag.reciprocated) {
			stream_ << "\\frac {";
			++div_count;
		} else {
			++mul_count;
		}
	}

	bool first = true;

	for (auto& child: node.children()) {
		if (first) {
			if (child.tag.reciprocated) {
				stream_ << "1} {";
			}
		} else {
			if (child.tag.reciprocated) {
				stream_ << "} {";
			} else {
				maybe_print_multiplication (child.node);
			}
		}

		if (dynamic_cast<Node::MultiplicationDivision*> (child.node.get()) ||
		    child.tag.reciprocated ||
		    (mul_count == 1)) {
			/* skip parenthesizing if either:
			 * - the child is mul-div node (\\frac is a parenthesizing construction by itself)
			 * - the child is reciprocated (it is enclosed in { .. })
			 * - we are the single multiplier (same) */
			child.node->accept (*this);
		} else {
			parenthesized_visit (node, child.node);
		}

		if (child.tag.reciprocated) {
			stream_ << "}";
		}

		first = false;
	}

	return boost::any();
}

std::string LaTeX::prepare_name (const std::string& name)
{
	if (name == "pi") {
		return "\\pi";
	}

	size_t underscore = 0, underscore_prev = 0, underscore_count = 0;
	std::string result;

	// Don't touch names which already contain braces.
	// For now we expect that the callers will escape required parts of the name themselves.
	// TODO: handle this automatically.
	if (name.find_first_of ("{}") != std::string::npos) {
		return name;
	}

	while ((underscore = name.find ('_', underscore)) != std::string::npos) {
		++underscore;
		result.append (name, underscore_prev, underscore - underscore_prev);
		result.append ("{");
		underscore_prev = underscore;
		++underscore_count;
	}
	result.append (name, underscore_prev, std::string::npos);
	for (size_t i = 0; i < underscore_count; ++i) {
		result += '}';
	}

	return result;
}

} // namespace Visitor
