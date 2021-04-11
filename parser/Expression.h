/*

SIN Toolchain
Copyright 2019 Riley Lannon
Expression.h

Contains the Expression class and all of its child classes; these are used by the Parser and Compiler to contain all of the different types of expressions available in the language.

*/

#pragma once
#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <exception>

#include "../util/EnumeratedTypes.h"
#include "../util/DataType.h"


bool is_literal(const lexeme_type candidate_type);

// Base class for all expressions
class Expression
{
protected:
	bool _const;	// if we have the 'constexpr' keyword, this will be set
	bool overridden;
	exp_type expression_type;	// replace "string type" with "exp_type expression_type"
public:
    bool is_const() const;
	void set_const();
	exp_type get_expression_type() const;

	virtual void override_qualities(symbol_qualities sq);
	virtual bool has_type_information() const;
	bool was_overridden() const;

	virtual std::unique_ptr<Expression> clone() const
	{
		return std::make_unique<Expression>();
	}

	Expression(const exp_type expression_type);
	Expression();

	virtual ~Expression();
};

// Derived classes

class Literal : public Expression
{
	DataType type;
	std::string value;
public:
	void set_type(DataType t);
	const DataType& get_data_type() const;
	const std::string& get_value() const;

	void override_qualities(symbol_qualities sq) override;
	bool has_type_information() const override;

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<Literal>(type, value);
	}

    Literal(Type data_type, const std::string& value, Type subtype = NONE);
	Literal(const DataType& t, const std::string& value);
	Literal();
};

// Identifier -- a variable name, function name, etc.
class Identifier : public Expression
{
protected:
	std::string value;	// the name of the variable
public:
	const std::string& getValue() const;
	void setValue(const std::string& new_value);

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<Identifier>(value);
	}

    Identifier(const std::string& value);
	Identifier();
};

class ListExpression : public Expression
{
	Type primary;
	std::vector<std::unique_ptr<Expression>> list_members;
public:
	std::vector<const Expression*> get_list() const;
	bool has_type_information() const override;
	Type get_list_type() const;	// the list type that we parsed -- () yields TUPLE, {} yields ARRAY

    void add_item(std::unique_ptr<Expression> to_add, const size_t index);

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		auto le = std::make_unique<ListExpression>();
		le->primary = primary;
		for (auto it = list_members.begin(); it != list_members.end(); it++)
		{
			le->list_members.push_back(it->get()->clone());
		}
		return le;
	}

	ListExpression(std::vector<std::unique_ptr<Expression>>& list_members, Type list_type);
	ListExpression(std::unique_ptr<Expression> arg, Type list_type);
	ListExpression();
};

class Indexed : public Expression
{
	std::unique_ptr<Expression> index_value;	// the index value is simply an expression
	std::unique_ptr<Expression> to_index;	// what we are indexing
public:
	const Expression &get_index_value() const;
	const Expression &get_to_index() const;

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<Indexed>(
			to_index->clone(),
			index_value->clone()
		);
	}

	Indexed(std::unique_ptr<Expression> to_index, std::unique_ptr<Expression> index_value);
	Indexed();
};

class KeywordExpression: public Expression
{
	DataType t;
	std::string keyword;
public:
    bool has_type_information() const override;
    void override_qualities(symbol_qualities sq) override;

	const std::string& get_keyword() const;
	const DataType &get_type() const;

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<KeywordExpression>(t, keyword);
	}

	KeywordExpression(const std::string& keyword);
	KeywordExpression(const DataType& t);
	KeywordExpression(const DataType& t, const std::string& keyword);
};

// Address Of -- the address of a variable
class AddressOf : public Expression
{
	std::shared_ptr<Expression> target;
public:
	Expression &get_target();

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<AddressOf>(target);
	}

    AddressOf(std::shared_ptr<Expression> target);
	AddressOf();
};

class AttributeSelection;
class Cast;
class Binary : public Expression
{
	friend class AttributeSelection;
	friend class Cast;

	exp_operator op;	// +, -, etc.
	std::unique_ptr<Expression> left_exp;
	std::unique_ptr<Expression> right_exp;

	std::unique_ptr<Expression> get_left_unique();
	std::unique_ptr<Expression> get_right_unique();
public:
	const Expression &get_left() const;
	const Expression &get_right() const;

	exp_operator get_operator() const;

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<Binary>(
			left_exp->clone(),
			right_exp->clone(),
			op
		);
	}

	Binary(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, const exp_operator op);
	Binary();
};

class Unary : public Expression
{
	exp_operator op;
	std::unique_ptr<Expression> operand;
public:
	exp_operator get_operator() const;
	const Expression &get_operand() const;

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<Unary>(operand->clone(), op);
	}

	Unary(std::unique_ptr<Expression> operand, exp_operator op);
	Unary();
};


// Functions are expressions if they return a value
class Procedure: public Expression
{
    std::unique_ptr<Expression> name;
    std::unique_ptr<Expression> args;
public:
    const Expression &get_func_name() const;
    const ListExpression &get_args() const;
    const Expression &get_arg(size_t arg_no) const;
    size_t get_num_args() const;

    void insert_arg(std::unique_ptr<Expression> to_insert, const size_t index);

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<Procedure>(name->clone(), args->clone());
	}

	Procedure(Procedure& other);
    Procedure(std::unique_ptr<Expression> proc_name, std::unique_ptr<Expression> proc_args);
    Procedure();
};

class CallExpression : public Procedure
{
public:
	CallExpression(Procedure *proc);
	CallExpression(CallExpression& other);
	CallExpression();
};

// typecasting expressions
class Cast : public Expression
{
	std::unique_ptr<Expression> to_cast;	// any expression can be casted
	DataType new_type;	// the new type for the expression
public:
    const Expression &get_exp() const;
	const DataType &get_new_type() const;

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		return std::make_unique<Cast>(to_cast->clone(), new_type);
	}

    Cast(Cast &old);
    Cast(std::unique_ptr<Expression> to_cast, const DataType& new_type);
	Cast(std::unique_ptr<Binary> b);
};

// Attribute selection
class AttributeSelection : public Expression
{
	std::unique_ptr<Expression> selected;
	attribute attrib;
	DataType t;
public:
	static attribute to_attribute(const std::string& to_convert);
	static bool is_attribute(const std::string& a);

    const Expression &get_selected() const;
	attribute get_attribute() const;
	const DataType &get_data_type() const;

	inline virtual std::unique_ptr<Expression> clone() const override
	{
		auto c = std::make_unique<AttributeSelection>(selected->clone(), attrib, t);
		return c;
	}

    AttributeSelection(AttributeSelection &old);
	AttributeSelection(std::unique_ptr<Expression>&& selected, const std::string& attribute_name);
	AttributeSelection(std::unique_ptr<Binary>&& to_deconstruct);
	AttributeSelection(std::unique_ptr<Expression>&& selected, attribute attrib, const DataType& t);
};

// Construction expression
class Construction : public Expression
{
public:
	/*

	The purpose of the Construction::Constructor class is to contain information about one particular initialization
	For example, in the construction:
		construct
		{
			x: 10,
			y: 20,
			z: 30,
		};
	each of those lines would have its own instance of Constructor.
	
	*/
	class Constructor
	{
		std::unique_ptr<Expression> _member;	// this should probably always be an identifier
		std::unique_ptr<Expression> _value;
	public:
		inline const Expression& get_member() const
		{
			return *_member;
		}

		inline const Expression& get_value() const
		{
			return *_value;
		}

		inline Constructor(std::unique_ptr<Expression>&& member, std::unique_ptr<Expression>&& value)
			: _member(std::move(member))
			, _value(std::move(value)) { }
		inline Constructor(const Constructor& other)
			: _member(other._member->clone())
			, _value(other._member->clone()) { }
		~Constructor() = default;
	};

	inline const Constructor* get_initializer(const size_t index)
	{
		try
		{
			return &_initializers.at(index);
		}
		catch (std::out_of_range& e)
		{
			return nullptr;
		}
	}

	inline bool has_explicit_type() const noexcept { return _has_explicit_type; }
	inline const std::string& get_explicit_type() const noexcept { return _explicit_type; }
	inline bool has_default() const noexcept { return _has_default; }
	
	inline void set_explicit_type(const std::string& type_name)
	{
		this->_explicit_type = type_name;
		_has_explicit_type = true;
	}
	inline void set_default()
	{
		this->_has_default = true;
	}

	inline const std::vector<Constructor>& get_initializers() const noexcept
	{
		return this->_initializers;
	}

	inline size_t num_initializations() const noexcept
	{
		return _initializers.size();
	}

	inline Construction()
		: Expression(CONSTRUCTION_EXP) { }
	inline Construction(std::vector<Constructor>&& initializers)
		: Expression(CONSTRUCTION_EXP)
		, _initializers( std::move(initializers) ) { }
	
	virtual ~Construction() = default;
private:
	std::vector<Constructor> _initializers;
	std::string _explicit_type;
	bool _has_explicit_type;
	bool _has_default;
};
