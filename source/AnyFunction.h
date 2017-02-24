#pragma once

struct CommonFunction;
struct Function;
struct ParseFunction;
struct UserFunction;

// Code or script function
struct AnyFunction
{
	enum Type
	{
		NONE,
		CODE,
		PARSE,
		SCRIPT
	};

	union
	{
		Function* f;
		ParseFunction* pf;
		CommonFunction* cf;
		UserFunction* uf;
	};
	Type type;

	AnyFunction(std::nullptr_t) : cf(nullptr), type(NONE) {}
	AnyFunction(Function* f) : f(f), type(CODE) {}
	AnyFunction(ParseFunction* pf) : pf(pf), type(PARSE) {}
	AnyFunction(UserFunction* uf) : uf(uf), type(SCRIPT) {}
	AnyFunction(CommonFunction* cf, Type type) : cf(cf), type(type) {}

	operator bool() const { return type != NONE; }
	bool operator == (const AnyFunction& f) const { return type == f.type && cf == f.cf; }
	bool operator != (const AnyFunction& f) const { return type != f.type || cf != f.cf; }

	bool IsCode() const { return type == CODE; }
	bool IsParse() const { return type == PARSE; }
	bool IsScript() const { return type == SCRIPT; }
};
