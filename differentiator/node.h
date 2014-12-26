#pragma once

#include <util/util.h>
#include <util/variable.h>

#include <memory>

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

	virtual int priority() const = 0;
	virtual void Dump (std::ostream& str) const = 0;

	DECLARE_ACCEPTOR = 0;

	virtual Ptr clone() const = 0;
	virtual bool compare (const Ptr& rhs) const = 0;

	Node::Base::Ptr accept_ptr (Visitor::Base& visitor) { return Node::Base::Ptr (boost::any_cast<Node::Base*> (accept (visitor))); }

	friend std::ostream& operator<< (std::ostream& out, const Base& node);
};

template <typename Tag>
class TaggedChildList : public Base
{
	struct Child {
		Base::Ptr node;
		Tag tag;

		Child (const Child&) = delete;
		Child (Child&&) = default;
	};

public:
	typedef std::unique_ptr<TaggedChildList<Tag>> Ptr;

    TaggedChildList() = default;

	std::list<Child>& children() { return children_; }
	const std::list<Child>& children() const { return children_; }

	void add_child (Base::Ptr&& node, const Tag& tag);
	void add_child_front (Base::Ptr&& node, const Tag& tag);
	void add_children_from (const TaggedChildList<Tag>& rhs);

	static bool compare_child (const Child& _1, const Child& _2);

protected:
	std::list<Child> children_;
};

template <>
class TaggedChildList<void> : public Base
{
public:
	typedef std::unique_ptr<TaggedChildList<void>> Ptr;

    TaggedChildList() = default;

	std::list<Base::Ptr>& children() { return children_; }
	const std::list<Base::Ptr>& children() const { return children_; }

	void add_child (Base::Ptr&& node);
	void add_child_front (Base::Ptr&& node);
	void add_children_from (const TaggedChildList<void>& rhs);

	static bool compare_child (const Base::Ptr& _1, const Base::Ptr& _2);

protected:
	std::list<Base::Ptr> children_;
};

class Value : public Base
{
public:
	typedef std::unique_ptr<Value> Ptr;

	Value (rational_t value);
	Value (integer_t value);

	rational_t value() const { return value_; }
	void set_value (rational_t value) { value_ = value; }

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;
    virtual bool compare (const Base::Ptr& rhs) const;

private:
	rational_t value_;
};

class Variable : public Base
{
public:
	typedef std::unique_ptr<Variable> Ptr;

    Variable (const std::string& name, const ::Variable& variable, bool is_error);

	const std::string& name() const { return name_; }
	std::string pretty_name() const { return is_error_ ? "Δ" + name_ : name_;  }
	boost::any value() const { return is_error_ ? variable_.error : variable_.value; }

	bool is_error() const { return is_error_; }
	bool is_target_variable (const std::string& desired) const { return !is_error_ && (name_ == desired); }
	bool can_be_substituted() const { return !variable_.do_not_substitute; }

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;
    virtual bool compare (const Base::Ptr& rhs) const;

private:
	const std::string& name_;
	const ::Variable& variable_;
	bool is_error_;
};

class Function : public TaggedChildList<void>
{
public:
	typedef std::unique_ptr<Function> Ptr;

	Function (const std::string& name);

	const std::string& name() const { return name_; }

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;
    virtual bool compare (const Base::Ptr& rhs) const;

private:
	std::string name_;
};

class Power : public TaggedChildList<void>
{
public:
	typedef std::unique_ptr<Power> Ptr;

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;
    virtual bool compare (const Base::Ptr& rhs) const;
};

struct AdditionSubtractionTag
{
	bool negated;

	AdditionSubtractionTag (bool n) : negated (n) { }
	bool operator== (const AdditionSubtractionTag& rhs) const { return negated == rhs.negated; }
};

class AdditionSubtraction : public TaggedChildList<AdditionSubtractionTag>
{
public:
	typedef std::unique_ptr<AdditionSubtraction> Ptr;

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;
    virtual bool compare (const Base::Ptr& rhs) const;

private:
	std::list<bool> negation_;
};

struct MultiplicationDivisionTag
{
	bool reciprocated;

	MultiplicationDivisionTag (bool r) : reciprocated (r) { }
	bool operator== (const MultiplicationDivisionTag& rhs) const { return reciprocated == rhs.reciprocated; }
};

class MultiplicationDivision : public TaggedChildList<MultiplicationDivisionTag>
{
public:
	typedef std::unique_ptr<MultiplicationDivision> Ptr;

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

	std::pair<Value*, MultiplicationDivisionTag*> get_constant();
	std::pair<Value*, MultiplicationDivisionTag*> get_or_create_constant();
	void release_constant_if_exists();
	rational_t get_constant_value();
	rational_t get_constant_value_and_release();
	void insert_constant (rational_t value);

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;
    virtual bool compare (const Base::Ptr& rhs) const;

private:
	std::pair<Value*, MultiplicationDivisionTag*> create_constant();
	rational_t calculate_constant_value (const std::pair<Value*, MultiplicationDivisionTag*> constant);
	void release_constant();
};

template <typename Tag>
void TaggedChildList<Tag>::add_child (Base::Ptr&& node, const Tag& tag)
{
	children_.push_back (Child { std::move (node), tag });
}

template <typename Tag>
void TaggedChildList<Tag>::add_child_front (Base::Ptr&& node, const Tag& tag)
{
	children_.push_front (Child { std::move (node), tag });
}

template <typename Tag>
void TaggedChildList<Tag>::add_children_from (const TaggedChildList<Tag>& rhs)
{
	for (const Child& child: rhs.children_) {
		children_.push_back (Child { child.node->clone(), child.tag });
	}
}

template <typename Tag>
bool TaggedChildList<Tag>::compare_child (const Child& _1, const Child& _2)
{
	return (_1.tag == _2.tag) &&
	       (_1.node->compare (_2.node));
}

inline void TaggedChildList<void>::add_child (Base::Ptr&& node)
{
	children_.push_back (std::move (node));
}

inline void TaggedChildList<void>::add_child_front (Base::Ptr&& node)
{
	children_.push_front (std::move (node));
}

inline void TaggedChildList<void>::add_children_from (const Node::TaggedChildList<void>& rhs)
{
	for (const Base::Ptr& child: rhs.children_) {
		children_.push_back (child->clone());
	}
}

inline bool TaggedChildList<void>::compare_child (const Base::Ptr& _1, const Base::Ptr& _2)
{
	return _1->compare (_2);
}

} // namespace Node
