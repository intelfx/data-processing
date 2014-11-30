#include "visitor-latex.h"
#include "visitor-calculate.h"

#include <sstream>
#include <regex>

namespace {

const std::string border_width = "2pt";
const std::string extra_left_border_width = "0pt";
const std::string varwidth_limit = "30cm";

} // anonymous namespace

namespace Visitor
{

LaTeX::Document::Document (const char* file)
: first_in_document_ (true)
{
	open (stream_, file);

	std::ostringstream latex_render_command_builder;
	latex_render_command_builder << "pdflatex " << "\"" << file << "\" >/dev/null";
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
		err << "LaTeX renderer exited with non-zero exit code: " << err;
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
: Print (stream, substitute)
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

void LaTeX::Document::print_expression (Node::Base::Ptr& tree, Base& visitor)
{
	write_linebreaks();

	stream_ << " & ="; tree->accept (visitor);
}

void LaTeX::Document::print_value (data_t value)
{
	write_linebreaks();

	stream_ << " & =" << prepare_value (value);
}

void LaTeX::Document::write_equation_footer()
{
	write_linebreaks();

	stream_ << "\\intertext{" << std::endl;
}

void LaTeX::Document::print (const std::string& name, Node::Base::Ptr& tree, bool substitute, bool calculate)
{
	LaTeX printer_literal (stream_, false),
	      printer_substituting (stream_, true);

	write_equation_header (name);

	print_expression (tree, printer_literal);

	if (substitute) {
		print_expression (tree, printer_substituting);
	}

	if (calculate) {
		Visitor::Calculate calculator;
		print_value (tree->accept_value (calculator));
	}

	write_equation_footer();
}

void LaTeX::Document::print (const std::string& name, Node::Base::Ptr& tree, Node::Base::Ptr& simplified)
{
	LaTeX printer_literal (stream_, false);

	write_equation_header (name);

	print_expression (tree, printer_literal);
	print_expression (simplified, printer_literal);

	write_equation_footer();
}

boost::any LaTeX::visit (Node::Value& node)
{
	stream_ << prepare_value (node.value());

	return boost::any();
}

boost::any LaTeX::visit (Node::Variable& node)
{
	if (substitute_ && need_substitution (node.name())) {
		stream_ << prepare_value (node.value());
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
	    fp_cmp (exponent_value->value(), 0.5)) {
		stream_ << "\\sqrt {";
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
	for (auto& child: node.children()) {
		if (child.tag.reciprocated) {
			stream_ << "\\frac {";
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
				/* elide multiplication sign only we're outputting symbolic variable names */
				if (substitute_ ||
				    dynamic_cast<Node::Value*> (child.node.get())) {
					stream_ << " * ";
				} else {
					stream_ << " ";
				}
			}
		}

		/* skip parenthesizing anything because { .. } are effectively parentheses */
		child.node->accept (*this);

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

	size_t underscore = name.find ('_');
	if (underscore == std::string::npos) {
		return name;
	} else {
		std::string result;
		result.append (name, 0, underscore + 1);
		result.append ("{");
		result.append (name, underscore + 1, std::string::npos);
		result.append ("}");
		return result;
	}
}

std::string LaTeX::prepare_value (data_t value)
{
	std::ostringstream ss;
	ss << value;

	std::string str = ss.str();

	if (str.find ('e') != std::string::npos) {
		std::regex exponent_replace_re ("e(-)?0*([0-9]*)", std::regex::extended);
		return "(" + std::regex_replace (str, exponent_replace_re, " * 10^{\\1\\2}", std::regex_constants::format_sed) + ")";
	} else {
		return str;
	}

}

} // namespace Visitor
