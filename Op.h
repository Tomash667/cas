#pragma once

enum Op
{
	add,
	sub,
	mul,
	o_div,
	mod,
	neg,
	inc_pre,
	inc_post,
	dec_pre,
	dec_post,

	equal,
	not_equal,
	greater,
	greater_equal,
	less,
	less_equal,
	and,
	or,

	cast,

	init_array,
	set_local,
	set_local_index,
	set_local_indexvar,

	//push_arg,
	push_local,
	push_local_ref,
	push_local_index,
	push_local_indexvar,
	push_local_index_ref,
	push_local_indexvar_ref,
	push_cstr,
	push_int,
	push_float,
	dup,
	pop,

	call,
	jmp,
	jmp_if,

	ret,
	//retv
};
