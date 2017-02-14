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
	PUSH_MEMBER, // [member_index] ..., class -> ..., class.member
	PUSH_MEMBER_REF, // [member_index] ..., class -> ..., class.member address
	PUSH_THIS_MEMBER, // [member_index] ... -> ..., this.member
	PUSH_THIS_MEMBER_REF, // [member_index] ... -> ..., this.member address
	PUSH_TMP, // [] ... -> ..., tmp
	PUSH_INDEX, // [] ..., arr, index -> ..., arr[index]
	PUSH_THIS, // [] ... -> ..., this
	PUSH_ENUM, // [type, value] ... -> ..., value
	POP, // [] ..., x -> ...
	SET_LOCAL, // [local_index] ..., x -> ..., x (local = value)
	SET_GLOBAL, // [global_index] ..., x -> ..., x (global = value)
	SET_ARG, // [arg_index] ..., x -> ..., x (arg = value)
	SET_MEMBER, // [member_index] ..., class, value -> ..., value (class.member = value)
	SET_THIS_MEMBER, // [member_index] ..., value -> ..., value (this.member = value)
	SET_TMP, // [] ..., x -> ..., x (tmp = x)
	SWAP, // [index] x(n...0) -> x(n...0, x[index] swaped with x[index+1])
	CAST, // [type] ..., x -> ..., (type)x
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
	JMP, // [address] - jump to address
	TJMP, // [address] ..., x -> ... - jump to adress if x is true
	FJMP, // [address] ..., x -> ... - jump to adress if x is false
	CALL, // [func_index] ..., arg0...argN -> ..., result - call code function
	CALLU, // [ufunc_index] ..., arg0...argN -> ..., result - call script function
	CALLU_CTOR, // [ufunc_index] ..., arg0...argN -> ..., result - call script constructor
	RET, // [] - returns from function/script
	COPY, // [] ..., x -> ..., x (x - single instance of struct)
	COPY_ARG, // [arg_index] - create single instance of struct in arg
	RELEASE_REF, // [index] - release reference to local/arg variable
	RELEASE_OBJ, // [index] - call release on local variable
	LINE, // [line number] - debug line numbers
	MAX_OP
};
