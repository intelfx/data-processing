#pragma once

#include <util.h>
#include "expr.h"

#include <memory>
#include <boost/any.hpp>

class Visitor;

#define DECLARE_ACCEPTOR virtual boost::any visit (Visitor& visitor)
#define IMPLEMENT_ACCEPTOR(type) boost::any type::visit (Visitor& visitor) { return visitor.visit (*this); }

namespace Node
{

class Base
{
public:
	typedef std::unique_ptr<Base> Ptr;

	Base() = default;
	virtual ~Base();

	void add_child (Ptr&& node);

	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR = 0;

protected:
	std::vector<Ptr> children_;
};

class Value : public Base
{
public:
	typedef std::unique_ptr<Value> Ptr;

	Value (data_t value);

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

	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

private:
	std::string name_;
};

class Power : public Base
{
public:
	typedef std::unique_ptr<Power> Ptr;

	Power (Base::Ptr&& base, Base::Ptr&& exponent);

	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;
};

class AdditionSubtraction : public Base
{
public:
	typedef std::unique_ptr<AdditionSubtraction> Ptr;

	void add_child (Base::Ptr&& node, bool negated);

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

	virtual void Dump (std::ostream& str);

	DECLARE_ACCEPTOR;

private:
	std::vector<bool> reciprocation_;
};

} // namespace Node
