#pragma once

#include <util.h>
#include "expr.h"

#include <memory>
#include <boost/any.hpp>

namespace Visitor {

class Base;

} // namespace Visitor

#define DECLARE_ACCEPTOR virtual boost::any accept (Visitor::Base& visitor)
#define IMPLEMENT_ACCEPTOR(type) boost::any type::accept (Visitor::Base& visitor) { return visitor.visit (*this); }

namespace Node
{

class Base
{
public:
	typedef std::unique_ptr<Base> Ptr;

	Base() = default;
	virtual ~Base();

	void add_child (Ptr&& node);

	Ptr clone() const;
	std::vector<Ptr>& children() { return children_; }

	virtual int priority() const = 0;
	virtual void Dump (std::ostream& str) = 0;

	DECLARE_ACCEPTOR = 0;

	Node::Base::Ptr accept_ptr (Visitor::Base& visitor) { return Node::Base::Ptr (boost::any_cast<Node::Base*> (accept (visitor))); }
	data_t accept_value (Visitor::Base& visitor) { return boost::any_cast<data_t> (accept (visitor)); }

protected:
	virtual Ptr clone_bare() const = 0;

	std::vector<Ptr> children_;
};

class Value : public Base
{
public:
	typedef std::unique_ptr<Value> Ptr;

	Value (data_t value);

	data_t value() const { return value_; }

	virtual int priority() const;
	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

protected:
    virtual Base::Ptr clone_bare() const;

private:
	data_t value_;
};

class Variable : public Base
{
public:
	typedef std::unique_ptr<Variable> Ptr;

    Variable (const std::string& name, const ::Variable& variable, bool is_error);

	const std::string& name() const { return name_; }
	std::string pretty_name() const { return is_error_ ? "Δ" + name_ : name_;  }
	data_t value() const { return is_error_ ? variable_.error : variable_.value; }

	bool is_error() const { return is_error_; }
	bool is_target_variable (const std::string& desired) const { return !is_error_ && (name_ == desired); }

	virtual int priority() const;
	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

protected:
    virtual Base::Ptr clone_bare() const;

private:
	const std::string& name_;
	const ::Variable& variable_;
	bool is_error_;
};

class Function : public Base
{
public:
	typedef std::unique_ptr<Function> Ptr;

	Function (const std::string& name);

	const std::string& name() const { return name_; }

	virtual int priority() const;
	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

protected:
    virtual Base::Ptr clone_bare() const;

private:
	std::string name_;
};

class Power : public Base
{
public:
	typedef std::unique_ptr<Power> Ptr;

	virtual int priority() const;
	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

protected:
    virtual Base::Ptr clone_bare() const;
};

class AdditionSubtraction : public Base
{
public:
	typedef std::unique_ptr<AdditionSubtraction> Ptr;

	void add_child (Base::Ptr&& node, bool negated);

	std::vector<bool>& negation() { return negation_; }

	virtual int priority() const;
	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

protected:
    virtual Base::Ptr clone_bare() const;

private:
	std::vector<bool> negation_;
};

class MultiplicationDivision : public Base
{
public:
	typedef std::unique_ptr<MultiplicationDivision> Ptr;

	void add_child (Base::Ptr&& node, bool reciprocated);

	std::vector<bool>& reciprocation() { return reciprocation_; }

	virtual int priority() const;
	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

protected:
    virtual Base::Ptr clone_bare() const;

private:
	std::vector<bool> reciprocation_;
};

} // namespace Node
