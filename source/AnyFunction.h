#pragma once

struct CodeFunction;
struct Function;
struct ParseFunction;
struct ScriptFunction;

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
		CodeFunction* cf;
		ParseFunction* pf;
		ScriptFunction* sf;
	};
	Type type;

	AnyFunction(std::nullptr_t) : f(nullptr), type(NONE) {}
	AnyFunction(CodeFunction* cf) : cf(cf), type(CODE) {}
	AnyFunction(ParseFunction* pf) : pf(pf), type(PARSE) {}
	AnyFunction(ScriptFunction* sf) : sf(sf), type(SCRIPT) {}
	AnyFunction(Function* f, Type type) : f(f), type(type) {}

	operator bool() const { return type != NONE; }
	bool operator == (const AnyFunction& f) const { return type == f.type && cf == f.cf; }
	bool operator != (const AnyFunction& f) const { return type != f.type || cf != f.cf; }

	bool IsCode() const { return type == CODE; }
	bool IsParse() const { return type == PARSE; }
	bool IsScript() const { return type == SCRIPT; }
};
