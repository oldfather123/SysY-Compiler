#pragma once
#include <string>

inline std::string libFunction =
    "\
	.text\n\
	.align	1\n\
	.globl	functionCacheLookup\n\
	.type	functionCacheLookup, @function\n\
functionCacheLookup:\n\
	slli	a1,a1,32\n\
	li	a5,1021\n\
	or	a2,a1,a2\n\
	remu	a5,a2,a5\n\
	slli	a4,a5,4\n\
	sext.w	a5,a5\n\
	add	a0,a0,a4\n\
	lw	a4,12(a0)\n\
	beq	a4,zero,.L4\n\
	ld	a4,0(a0)\n\
	beq	a4,a2,.L3\n\
	sw	zero,12(a0)\n\
.L4:\n\
	sd	a2,0(a0)\n\
.L3:\n\
	mv	a0,a5\n\
	ret\n\
	.size	functionCacheLookup, .-functionCacheLookup ";
