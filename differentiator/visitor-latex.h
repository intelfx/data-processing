#pragma once

#include "visitor-print.h"

namespace Visitor
{

class LaTeX : public Print
{
public:
	class Document
	{
		std::ofstream stream_;
		std::string latex_render_command_;
		bool first_in_document_;

		void write_header();
		void write_footer();

		void write_linebreaks();
		void write_equation_header (const std::string& name);
		void write_equation_footer();

		void print_expression (Node::Base* tree, Visitor::Base& visitor);
		void print_value (const boost::any& value);

	public:
		Document (const char* file);
		~Document();

		void print ( const std::string& name, Node::Base* tree, bool substitute, const boost::any& value );
		void print (const std::string& name, Node::Base* tree, Node::Base* simplified);
	};

private:
	LaTeX (std::ostream& stream, bool substitute);

public:
    virtual boost::any visit (Node::Value& node);
    virtual boost::any visit (Node::Variable& node);
    virtual boost::any visit (Node::Power& node);
    virtual boost::any visit (Node::MultiplicationDivision& node);

	static std::string prepare_name (const std::string& name);
};

} // namespace Visitor
