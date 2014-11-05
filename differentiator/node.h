#pragma once

#include <util.h>
#include "expr.h"

#include <memory>
#include <boost/any.hpp>

class Visitor;

#define DECLARE_ACCEPTOR virtual boost::any accept (Visitor& visitor)
#define IMPLEMENT_ACCEPTOR(type) boost::any type::accept (Visitor& visitor) { return visitor.visit (*this); }

namespace Node
{

class Base
{
public:
	typedef std::unique_ptr<Base> Ptr;

	Base() = default;
	virtual ~Base();

	void add_child (Ptr&& node);

	std::vector<Ptr>& children() { return children_; }

	virtual int priority() const = 0;
	virtual void Dump (std::ostream& str) = 0;

	DECLARE_ACCEPTOR = 0;

protected:
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

private:
	data_t value_;
};

class Variable : public Base
{
public:
	typedef std::unique_ptr<Variable> Ptr;

    Variable (const std::string& name, const ::Variable& variable);

	const std::string& name() const { return name_; }
	data_t value() const { return variable_.value; }

	virtual int priority() const;
	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

private:
	const std::string& name_;
	const ::Variable& variable_;
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

private:
	std::vector<bool> reciprocation_;
};

} // namespace Node
