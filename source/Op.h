#pragma once

enum Op
{
	PUSH, // [] ..., x -> ..., x, x
	PUSH_TRUE, // [] ... -> ..., true
	PUSH_FALSE, // [] ... -> ..., false
	PUSH_CHAR, // [char] ... -> ..., char
	PUSH_INT, // [int] ... -> ..., int
	PUSH_FLOAT, // [float] ... -> ..., float
	PUSH_STRING, // [str_index] ... -> ..., string
	PUSH_LOCAL, // [local_index] ... -> ..., local value
	PUSH_LOCAL_REF, // [local_index] ... -> ..., local address
	PUSH_GLOBAL, // [global_index] ... -> ..., global value
	PUSH_GLOBAL_REF, // [global_index] ... -> ..., global address
	PUSH_ARG, // [arg_index] ... -> ..., arg value
	PUSH_ARG_REF, // [arg_index] ... -> ..., arg address
	PUSH_MEMBER,
	PUSH_MEMBER_REF,
	PUSH_THIS_MEMBER,
	PUSH_THIS_MEMBER_REF,
	PUSH_TMP, // [] ... -> ..., tmp
	POP, // [] ..., x -> ...
	SET_LOCAL,
	SET_GLOBAL,
	SET_ARG,
	SET_MEMBER, // [member_index] ..., class, value -> value, ... (class.member = value)
	SET_THIS_MEMBER,
	SET_TMP, // [] ..., x -> ..., x (tmp = x)
	CAST,
	NEG, // [] ..., x -> ..., -x (x=int/float)
	ADD, // [] ..., x, y -> ..., x+y (x,y=int/float/string)
	SUB, // [] ..., x, y -> ..., x-y (x,y=int/float)
	MUL, // [] ..., x, y -> ..., x*y (x,y=int/float)
	DIV, // [] ..., x, y -> ..., x/y (x,y=int/float)
	MOD, // [] ..., x, y -> ..., x%y (x,y=int/float)
	BIT_AND, // [] ..., x, y -> ..., x&y (x,y=int)
	BIT_OR, // [] ..., x, y -> ..., x|y (x,y=int)
	BIT_XOR, // [] ..., x, y -> ..., x^y (x,y=int)
	BIT_LSHIFT, // [] ..., x, y -> ..., x<<y (x,y=int)
	BIT_RSHIFT, // [] ..., x, y -> ..., x>>y (x,y=int)
	INC, // [] ..., x -> ..., x+1 (x,y=char/int/float)
	DEC, // [] ..., x -> ..., x-1 (x,y=char/int/float)
	DEREF, // [] ..., x -> ..., [x] (x=address)
	SET_ADR, // [] ..., x, y -> ..., y (x=address, y=value, op= [x]=y)
	IS, // [] ..., x, y -> ..., x is y (x,y=string,class,ref)
	EQ, // [] ..., x, y -> ..., x==y (x,y=bool)
	NOT_EQ, // [] ..., x, y -> ..., x!=y (x,y=bool)
	GR, // [] ..., x, y -> ..., x>y (x,y=bool)
	GR_EQ, // [] ..., x, y -> ..., x>=y (x,y=bool)
	LE, // [] ..., x, y -> ..., x<y (x,y=bool)
	LE_EQ, // [] ..., x, y -> ..., x<=y (x,y=bool)
	AND, // [] ..., x, y -> ..., x&&y (x,y=bool)
	OR, // [] ..., x, y -> ..., x||y (x,y=bool)
	NOT, // [] ..., x -> ..., !x (x=bool)
	BIT_NOT, // [] ..., x -> ..., ~x (x=int)
	JMP,
	TJMP,
	FJMP,
	CALL,
	CALLU,
	CALLU_CTOR,
	RET,
	CTOR,
	COPY, // [] ..., x -> ..., x (x - single instance of struct)
	COPY_ARG, // [arg_index] - create single instance of struct in arg
	MAX_OP
};
