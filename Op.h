#pragma once

enum Op
{
	PUSH_TRUE,
	PUSH_FALSE,
	PUSH_INT,
	PUSH_FLOAT,
	PUSH_STRING,
	PUSH_VAR,
	PUSH_VAR_REF,
	POP,
	SET_VAR,
	CAST,
	NEG,
	ADD,
	SUB,
	MUL,
	DIV,
	MOD,
	PRE_INC,
	PRE_DEC,
	POST_INC,
	POST_DEC,
	DEREF,
	EQ,
	NOT_EQ,
	GR,
	GR_EQ,
	LE,
	LE_EQ,
	AND,
	OR,
	NOT,
	JMP,
	TJMP,
	FJMP,
	CALL,
	RET,
	MAX_OP
};