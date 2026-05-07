	.type	_ZN12_GLOBAL__N_110cmmcWorkerEPv, @function
_ZN12_GLOBAL__N_110cmmcWorkerEPv:
.LFB1070:
	.cfi_startproc
	addi	sp,sp,-208
	.cfi_def_cfa_offset 208
	sd	s5,152(sp)
	.cfi_offset 21, -56
	la	s5,__stack_chk_guard
	sd	s0,192(sp)
	ld	a5, 0(s5)
	sd	a5, 136(sp)
	li	a5, 0
	sd	ra,200(sp)
	.cfi_offset 8, -16
	.cfi_offset 1, -8
	mv	s0,a0
	sd	s1,184(sp)
	sd	s2,176(sp)
	sd	s3,168(sp)
	sd	s4,160(sp)
	sd	s6,144(sp)
	.cfi_offset 9, -24
	.cfi_offset 18, -32
	.cfi_offset 19, -40
	.cfi_offset 20, -48
	.cfi_offset 22, -64
	fence	iorw,iorw
	lw	a5,16(a0)
	fence	iorw,iorw
	li	a4,1023
	sext.w	a2,a5
	addi	s1,sp,8
	zext.w	a5,a5
	bgtu	a5,a4,.L2
	li	a3,1
	srli	a5,a5,6
	sll	a3,a3,a2
	sh3add	a5,a5,s1
	ld	a4,0(a5)
	or	a4,a4,a3
	sd	a4,0(a5)
.L2:
	li	a0,178
	addi	s2,s0,20
	call	syscall@plt
	addi	s3,s0,48
	mv	a2,s1
	sext.w	a0,a0
	li	a1,128
	addi	s1,s0,52
	call	sched_setaffinity@plt
	li	s4,1
	j	.L6
.L22:
	fence iorw,ow;  1: lr.w.aq a5,0(s3); bne a5,s4,1f; sc.w.aq a4,zero,0(s3); bnez a4,1b; 1:
	addiw	a5,a5,-1
	li	s6,1
	bne	a5,zero,.L4
.L7:
	fence	iorw,iorw
	lw	a5,0(s2)
	fence	iorw,iorw
	beq	a5,zero,.L5
	fence	iorw,iorw
	ld	a5,24(s0)
	fence	iorw,iorw
	fence	iorw,iorw
	lw	a0,32(s0)
	fence	iorw,iorw
	fence	iorw,iorw
	sext.w	a0,a0
	lw	a1,36(s0)
	fence	iorw,iorw
	fence	iorw,iorw
	sext.w	a1,a1
	ld	a2,40(s0)
	fence	iorw,iorw
	jalr	a5
	fence iorw,ow;  1: lr.w.aq a5,0(s1); bne a5,zero,1f; sc.w.aq a4,s4,0(s1); bnez a4,1b; 1:
	sext.w	a5,a5
	bne	a5,zero,.L6
	mv	a1,s1
	li	a6,0
	li	a4,0
	li	a3,1
	li	a2,1
	li	a0,98
	call	syscall@plt
.L6:
	fence	iorw,iorw
	lw	a5,0(s2)
	fence	iorw,iorw
	bne	a5,zero,.L22
.L5:
	ld	a4, 136(sp)
	ld	a5, 0(s5)
	xor	a5, a4, a5
	li	a4, 0
	bne	a5,zero,.L23
	ld	ra,200(sp)
	.cfi_remember_state
	.cfi_restore 1
	li	a0,0
	ld	s0,192(sp)
	.cfi_restore 8
	ld	s1,184(sp)
	.cfi_restore 9
	ld	s2,176(sp)
	.cfi_restore 18
	ld	s3,168(sp)
	.cfi_restore 19
	ld	s4,160(sp)
	.cfi_restore 20
	ld	s5,152(sp)
	.cfi_restore 21
	ld	s6,144(sp)
	.cfi_restore 22
	addi	sp,sp,208
	.cfi_def_cfa_offset 0
	jr	ra
.L4:
	.cfi_restore_state
	mv	a1,s3
	li	a6,0
	li	a5,0
	li	a4,0
	li	a3,0
	li	a2,0
	li	a0,98
	call	syscall@plt
	fence iorw,ow;  1: lr.w.aq a5,0(s3); bne a5,s6,1f; sc.w.aq a4,zero,0(s3); bnez a4,1b; 1:
	addiw	a5,a5,-1
	beq	a5,zero,.L7
	j	.L4
.L23:
	call	__stack_chk_fail@plt
	.cfi_endproc
.LFE1070:
	.size	_ZN12_GLOBAL__N_110cmmcWorkerEPv, .-_ZN12_GLOBAL__N_110cmmcWorkerEPv
	.section	.text.startup,"ax",@progbits
	.align	1
	.globl	cmmcInitRuntime
	.type	cmmcInitRuntime, @function
cmmcInitRuntime:
.LFB1071:
	.cfi_startproc
	addi	sp,sp,-64
	.cfi_def_cfa_offset 64
	sd	s2,32(sp)
	.cfi_offset 18, -32
	li	s2,331776
	sd	s3,24(sp)
	.cfi_offset 19, -40
	li	s3,131072
	addi	s3,s3,34
	addi	s2,s2,-256
	sd	s0,48(sp)
	.cfi_offset 8, -16
	lla	s0,.LANCHOR0
	sd	s1,40(sp)
	.cfi_offset 9, -24
	li	s1,0
	sd	s4,16(sp)
	.cfi_offset 20, -48
	li	s4,4
	sd	s5,8(sp)
	.cfi_offset 21, -56
	lla	s5,_ZN12_GLOBAL__N_110cmmcWorkerEPv
	sd	s6,0(sp)
	.cfi_offset 22, -64
	li	s6,1
	sd	ra,56(sp)
	.cfi_offset 1, -8
.L25:
	addi	a5,s0,20
	fence iorw,ow; amoswap.w.aq zero,s6,0(a5)
	mv	a3,s3
	li	a5,0
	li	a4,-1
	li	a2,3
	li	a1,1048576
	li	a0,0
	call	mmap@plt
	sd	a0,8(s0)
	addi	a5,s0,16
	fence iorw,ow; amoswap.w.aq zero,s1,0(a5)
	li	a5,1048576
	mv	a3,s0
	ld	a1,8(s0)
	mv	a2,s2
	add	a1,a1,a5
	mv	a0,s5
	addi	s0,s0,56
	addiw	s1,s1,1
	call	clone@plt
	sw	a0,-56(s0)
	bne	s1,s4,.L25
	ld	ra,56(sp)
	.cfi_restore 1
	ld	s0,48(sp)
	.cfi_restore 8
	ld	s1,40(sp)
	.cfi_restore 9
	ld	s2,32(sp)
	.cfi_restore 18
	ld	s3,24(sp)
	.cfi_restore 19
	ld	s4,16(sp)
	.cfi_restore 20
	ld	s5,8(sp)
	.cfi_restore 21
	ld	s6,0(sp)
	.cfi_restore 22
	addi	sp,sp,64
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE1071:
	.size	cmmcInitRuntime, .-cmmcInitRuntime
	.section	.init_array,"aw"
	.align	3
	.dword	cmmcInitRuntime
	.section	.text.exit,"ax",@progbits
	.align	1
	.globl	cmmcUninitRuntime
	.type	cmmcUninitRuntime, @function
cmmcUninitRuntime:
.LFB1072:
	.cfi_startproc
	addi	sp,sp,-32
	.cfi_def_cfa_offset 32
	sd	s0,16(sp)
	.cfi_offset 8, -16
	lla	s0,.LANCHOR0+48
	sd	s1,8(sp)
	.cfi_offset 9, -24
	li	s1,1
	sd	s2,0(sp)
	.cfi_offset 18, -32
	lla	s2,.LANCHOR0+272
	sd	ra,24(sp)
	.cfi_offset 1, -8
.L32:
	addi	a5,s0,-28
	fence iorw,ow; amoswap.w.aq zero,zero,0(a5)
	fence iorw,ow;  1: lr.w.aq a7,0(s0); bne a7,zero,1f; sc.w.aq a5,s1,0(s0); bnez a5,1b; 1:
	sext.w	a7,a7
	mv	a1,s0
	li	a6,0
	li	a5,0
	li	a4,0
	li	a3,1
	li	a2,1
	li	a0,98
	bne	a7,zero,.L29
	addi	s0,s0,56
	call	syscall@plt
	li	a2,0
	li	a1,0
	lw	a0,-104(s0)
	call	waitpid@plt
	bne	s2,s0,.L32
.L28:
	ld	ra,24(sp)
	.cfi_remember_state
	.cfi_restore 1
	ld	s0,16(sp)
	.cfi_restore 8
	ld	s1,8(sp)
	.cfi_restore 9
	ld	s2,0(sp)
	.cfi_restore 18
	addi	sp,sp,32
	.cfi_def_cfa_offset 0
	jr	ra
.L29:
	.cfi_restore_state
	lw	a0,-48(s0)
	li	a2,0
	addi	s0,s0,56
	li	a1,0
	call	waitpid@plt
	bne	s0,s2,.L32
	j	.L28
	.cfi_endproc
.LFE1072:
	.size	cmmcUninitRuntime, .-cmmcUninitRuntime
	.section	.fini_array,"aw"
	.align	3
	.dword	cmmcUninitRuntime
	.text
	.align	1
	.globl	cmmcParallelFor
	.type	cmmcParallelFor, @function
cmmcParallelFor:
.LFB1077:
	.cfi_startproc
	bge	a0,a1,.L50
	addi	sp,sp,-96
	.cfi_def_cfa_offset 96
	li	a4,15
	sd	s2,64(sp)
	.cfi_offset 18, -32
	mv	s2,a1
	sd	s5,40(sp)
	.cfi_offset 21, -56
	mv	s5,a2
	sd	s6,32(sp)
	.cfi_offset 22, -64
	mv	s6,a3
	sd	ra,88(sp)
	subw	a3,a1,a0
	sd	s0,80(sp)
	sd	s1,72(sp)
	sd	s3,56(sp)
	sd	s4,48(sp)
	sd	s7,24(sp)
	sd	s8,16(sp)
	sd	s9,8(sp)
	.cfi_offset 1, -8
	.cfi_offset 8, -16
	.cfi_offset 9, -24
	.cfi_offset 19, -40
	.cfi_offset 20, -48
	.cfi_offset 23, -72
	.cfi_offset 24, -80
	.cfi_offset 25, -88
	ble	a3,a4,.L53
	srliw	s4,a3,2
	sext.w	s1,a0
	addiw	s4,s4,3
	lla	s0,.LANCHOR0+48
	andi	s4,s4,-4
	li	s3,0
	sext.w	s4,s4
	li	s9,3
	li	s8,1
	li	s7,4
.L39:
	sext.w	a4,s1
	addw	s1,s4,s1
	min	a5,s1,s2
	bne s3,s9,1f; mv a5,s2; 1: # movcc
	addi	a3,s0,-24
	fence iorw,ow; amoswap.d.aq zero,s5,0(a3)
	addi	a3,s0,-16
	fence iorw,ow; amoswap.w.aq zero,a4,0(a3)
	addi	a4,s0,-12
	fence iorw,ow; amoswap.w.aq zero,a5,0(a4)
	addi	a5,s0,-8
	fence iorw,ow; amoswap.d.aq zero,s6,0(a5)
	fence iorw,ow;  1: lr.w.aq a7,0(s0); bne a7,zero,1f; sc.w.aq a5,s8,0(s0); bnez a5,1b; 1:
	sext.w	a7,a7
	mv	a1,s0
	addiw	s3,s3,1
	li	a6,0
	li	a5,0
	li	a4,0
	li	a3,1
	li	a2,1
	li	a0,98
	bne	a7,zero,.L38
	call	syscall@plt
.L38:
	addi	s0,s0,56
	bne	s3,s7,.L39
	lla	s0,.LANCHOR0+52
	lla	s2,.LANCHOR0+276
	li	s1,1
.L40:
	fence iorw,ow;  1: lr.w.aq a5,0(s0); bne a5,s1,1f; sc.w.aq a4,zero,0(s0); bnez a4,1b; 1:
	addiw	a5,a5,-1
	li	s3,1
	bne	a5,zero,.L41
.L42:
	addi	s0,s0,56
	bne	s2,s0,.L40
	ld	ra,88(sp)
	.cfi_remember_state
	.cfi_restore 1
	ld	s0,80(sp)
	.cfi_restore 8
	ld	s1,72(sp)
	.cfi_restore 9
	ld	s2,64(sp)
	.cfi_restore 18
	ld	s3,56(sp)
	.cfi_restore 19
	ld	s4,48(sp)
	.cfi_restore 20
	ld	s5,40(sp)
	.cfi_restore 21
	ld	s6,32(sp)
	.cfi_restore 22
	ld	s7,24(sp)
	.cfi_restore 23
	ld	s8,16(sp)
	.cfi_restore 24
	ld	s9,8(sp)
	.cfi_restore 25
	addi	sp,sp,96
	.cfi_def_cfa_offset 0
	jr	ra
.L53:
	.cfi_restore_state
	ld	ra,88(sp)
	.cfi_restore 1
	mv	a2,s6
	ld	s0,80(sp)
	.cfi_restore 8
	mv	a5,s5
	ld	s1,72(sp)
	.cfi_restore 9
	ld	s2,64(sp)
	.cfi_restore 18
	ld	s3,56(sp)
	.cfi_restore 19
	ld	s4,48(sp)
	.cfi_restore 20
	ld	s5,40(sp)
	.cfi_restore 21
	ld	s6,32(sp)
	.cfi_restore 22
	ld	s7,24(sp)
	.cfi_restore 23
	ld	s8,16(sp)
	.cfi_restore 24
	ld	s9,8(sp)
	.cfi_restore 25
	addi	sp,sp,96
	.cfi_def_cfa_offset 0
	jr	a5
.L50:
	ret
.L41:
	.cfi_def_cfa_offset 96
	.cfi_offset 1, -8
	.cfi_offset 8, -16
	.cfi_offset 9, -24
	.cfi_offset 18, -32
	.cfi_offset 19, -40
	.cfi_offset 20, -48
	.cfi_offset 21, -56
	.cfi_offset 22, -64
	.cfi_offset 23, -72
	.cfi_offset 24, -80
	.cfi_offset 25, -88
	mv	a1,s0
	li	a6,0
	li	a5,0
	li	a4,0
	li	a3,0
	li	a2,0
	li	a0,98
	call	syscall@plt
	fence iorw,ow;  1: lr.w.aq a5,0(s0); bne a5,s3,1f; sc.w.aq a4,zero,0(s0); bnez a4,1b; 1:
	addiw	a5,a5,-1
	beq	a5,zero,.L42
	j	.L41
	.cfi_endproc
.LFE1077:
	.size	cmmcParallelFor, .-cmmcParallelFor
	.align	1
	.globl	cmmcCacheLookup
	.type	cmmcCacheLookup, @function
cmmcCacheLookup:
.LFB1078:
	.cfi_startproc
	slli	a1,a1,32
	li	a5,1021
	or	a2,a1,a2
	remu	a5,a2,a5
	slli	a5,a5,4
	add	a0,a0,a5
	lw	a5,12(a0)
	beq	a5,zero,.L57
	ld	a5,0(a0)
	beq	a5,a2,.L54
	sw	zero,12(a0)
.L57:
	sd	a2,0(a0)
.L54:
	ret
	.cfi_endproc
.LFE1078:
	.size	cmmcCacheLookup, .-cmmcCacheLookup
	.align	1
	.globl	cmmcAddRec3SRem
	.type	cmmcAddRec3SRem, @function
cmmcAddRec3SRem:
.LFB1079:
	.cfi_startproc
	addi	a5,a0,-1
	mul	a5,a5,a0
	srli	a0,a5,63
	add	a0,a0,a5
	srai	a0,a0,1
	rem	a0,a0,a1
	sext.w	a0,a0
	ret
	.cfi_endproc
.LFE1079:
	.size	cmmcAddRec3SRem, .-cmmcAddRec3SRem
	.bss
	.align	3
	.set	.LANCHOR0,. + 0
	.type	_ZN12_GLOBAL__N_17workersE, @object
	.size	_ZN12_GLOBAL__N_17workersE, 224
_ZN12_GLOBAL__N_17workersE:
	.zero	224
	.ident	"GCC: (Ubuntu 12.3.0-1ubuntu1~22.04) 12.3.0"
	.section	.note.GNU-stack,"",@progbits
    