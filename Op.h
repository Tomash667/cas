#pragma once

enum Op
{
	PUSH_INT,
	PUSH_STRING,
	PUSH_VAR,
	POP,
	POP_VAR,
	CAST,
	NEG,
	ADD,
	SUB,
	MUL,
	DIV,
	CALL,
	RET,

	GROUP
};
