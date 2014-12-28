#pragma once

#include <util/util.h>
#include <util/variable.h>

#include <memory>

namespace Visitor {

class Base;

} // namespace Visitor

#define DECLARE_ACCEPTOR virtual boost::any accept (Visitor::Base& visitor) const
#define IMPLEMENT_ACCEPTOR(type) boost::any type::accept (Visitor::Base& visitor) const { return visitor.visit (*this); }

#define IMPLEMENT_GET_TYPE(type) TypeOrdered type::get_type() const { return TypeOrdered::type; }

namespace Node
{

enum class TypeOrdered
{
	Value = 0,
	Function,
	Variable,
	Power,
	MultiplicationDivision,
	AdditionSubtraction
};

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
	bool compare (const Ptr& rhs) const;
	bool less (const Ptr& rhs) const;

	Node::Base::Ptr accept_ptr (Visitor::Base& visitor) const { return Node::Base::Ptr (boost::any_cast<Node::Base*> (accept (visitor))); }

	friend std::ostream& operator<< (std::ostream& out, const Base& node);

protected:
	virtual bool compare_same_type (const Ptr& rhs) const = 0;
 	virtual bool less_same_type (const Ptr& rhs) const = 0;
	virtual TypeOrdered get_type() const = 0;
};

template <typename Tag>
struct TaggedChild
{
	Base::Ptr node;
	Tag tag;

	bool operator== (const TaggedChild& rhs) const
	{
		return (tag == rhs.tag) &&
		       (node->compare (rhs.node));
	}

	bool operator!= (const TaggedChild& rhs) const
	{
		return !operator== (rhs);
	}

	bool operator< (const TaggedChild& rhs) const
	{
		return (tag < rhs.tag) ||
		       ((tag == rhs.tag) && (node->less (rhs.node)));
	}

	TaggedChild() = default;
	TaggedChild (const TaggedChild&) = delete;
	TaggedChild (TaggedChild&&) = default;
    TaggedChild clone() const { return { node->clone(), tag }; }
};

template <>
struct TaggedChild<void>
{
	Base::Ptr node;

	bool operator== (const TaggedChild<void>& rhs) const
	{
		return node->compare (rhs.node);
	}

	bool operator!= (const TaggedChild<void>& rhs) const
	{
		return !operator== (rhs);
	}

	bool operator< (const TaggedChild<void>& rhs) const
	{
		return node->less (rhs.node);
	}

	TaggedChild() = default;
	TaggedChild (const TaggedChild<void>&) = delete;
	TaggedChild (TaggedChild<void>&&) = default;
    TaggedChild clone() const { return { node->clone() }; }
};

template <typename Tag>
class TaggedChildSet : public Base
{
public:
	std::multiset<TaggedChild<Tag>>& children() { return children_; }
	const std::multiset<TaggedChild<Tag>>& children() const { return children_; }

	void add_children_from (const TaggedChildSet<Tag>& rhs);

protected:
    void add_child (TaggedChild<Tag>&& child);

	std::multiset<TaggedChild<Tag>> children_;
};

template <typename Tag>
class TaggedChildList : public Base
{
public:
	std::list<TaggedChild<Tag>>& children() { return children_; }
	const std::list<TaggedChild<Tag>>& children() const { return children_; }

    void add_children_from (const TaggedChildList<Tag>& rhs);

protected:
    void add_child (TaggedChild<Tag>&& child);
    void add_child_front (TaggedChild<Tag>&& child);

	std::list<TaggedChild<Tag>> children_;
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

protected:
    virtual bool compare_same_type (const Base::Ptr& rhs) const;
 	virtual bool less_same_type (const Base::Ptr& rhs) const;
	virtual TypeOrdered get_type() const;

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

protected:
    virtual bool compare_same_type (const Base::Ptr& rhs) const;
 	virtual bool less_same_type (const Base::Ptr& rhs) const;
	virtual TypeOrdered get_type() const;

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

    void add_child (Node::Base::Ptr&& node) { TaggedChildList::add_child ({ std::move (node) }); }

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;

protected:
    virtual bool compare_same_type (const Base::Ptr& rhs) const;
 	virtual bool less_same_type (const Base::Ptr& rhs) const;
	virtual TypeOrdered get_type() const;

private:
	std::string name_;
};

class Power : public Base
{
public:
	typedef std::unique_ptr<Power> Ptr;

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

    void set_base (Node::Base::Ptr&& node) { base_ = std::move (node); }
    Node::Base::Ptr& get_base() { return base_; }
    const Node::Base::Ptr& get_base() const { return base_; }

    void set_exponent (Node::Base::Ptr&& node) { exponent_ = std::move (node); }
    Node::Base::Ptr& get_exponent() { return exponent_; }
    const Node::Base::Ptr& get_exponent() const { return exponent_; }

	rational_t get_exponent_constant (bool release);
	void insert_exponent_constant (rational_t value);

	Base::Ptr decay_move (Base::Ptr&& self);
	void decay_assign (Base::Ptr& dest);

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;

protected:
    virtual bool compare_same_type (const Base::Ptr& rhs) const;
 	virtual bool less_same_type (const Base::Ptr& rhs) const;
	virtual TypeOrdered get_type() const;

    Node::Base::Ptr base_, exponent_;
};

struct AdditionSubtractionTag
{
	bool negated;

	AdditionSubtractionTag (bool n) : negated (n) { }

	bool operator== (const AdditionSubtractionTag& rhs) const { return negated == rhs.negated; }
	bool operator< (const AdditionSubtractionTag& rhs) const { return !negated && rhs.negated; }
};

class AdditionSubtraction : public TaggedChildSet<AdditionSubtractionTag>
{
public:
	typedef std::unique_ptr<AdditionSubtraction> Ptr;

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

    void add_child (Node::Base::Ptr&& node, bool negate) { TaggedChildSet::add_child ({ std::move (node), negate }); }

	Base::Ptr decay_move (Base::Ptr&& self);
	void decay_assign (Base::Ptr& dest);

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;

protected:
    virtual bool compare_same_type (const Base::Ptr& rhs) const;
 	virtual bool less_same_type (const Base::Ptr& rhs) const;
	virtual TypeOrdered get_type() const;

private:
	std::list<bool> negation_;
};

struct MultiplicationDivisionTag
{
	bool reciprocated;

	MultiplicationDivisionTag (bool r) : reciprocated (r) { }

	bool operator== (const MultiplicationDivisionTag& rhs) const { return reciprocated == rhs.reciprocated; }
	bool operator< (const MultiplicationDivisionTag& rhs) const { return !reciprocated && rhs.reciprocated; }
};

class MultiplicationDivision : public TaggedChildSet<MultiplicationDivisionTag>
{
public:
	typedef std::unique_ptr<MultiplicationDivision> Ptr;

	virtual int priority() const;
	virtual void Dump (std::ostream& str) const;

    void add_child (Node::Base::Ptr&& node, bool reciprocate) { TaggedChildSet::add_child ({ std::move (node), reciprocate }); }

	rational_t get_constant (bool release);
	void insert_constant (rational_t value);

	Base::Ptr decay_move (Base::Ptr&& self);
	void decay_assign (Base::Ptr& dest);

	DECLARE_ACCEPTOR;

    virtual Base::Ptr clone() const;

protected:
    virtual bool compare_same_type (const Base::Ptr& rhs) const;
 	virtual bool less_same_type (const Base::Ptr& rhs) const;
	virtual TypeOrdered get_type() const;
};

template <typename Tag>
void TaggedChildSet<Tag>::add_child (TaggedChild<Tag>&& child)
{
	children_.insert (std::move (child));
}

template <typename Tag>
void TaggedChildSet<Tag>::add_children_from (const TaggedChildSet<Tag>& rhs)
{
	for (const TaggedChild<Tag>& child: rhs.children_) {
		children_.insert (child.clone());
	}
}

template <typename Tag>
void TaggedChildList<Tag>::add_child (TaggedChild<Tag>&& child)
{
    children_.push_back (std::move (child));
}

template <typename Tag>
void TaggedChildList<Tag>::add_child_front (TaggedChild<Tag>&& child)
{
    children_.push_front (std::move (child));
}

template <typename Tag>
void TaggedChildList<Tag>::add_children_from (const TaggedChildList<Tag>& rhs)
{
    for (const TaggedChild<Tag>& child: rhs.children_) {
        children_.push_back (child.clone());
    }
}

} // namespace Node
