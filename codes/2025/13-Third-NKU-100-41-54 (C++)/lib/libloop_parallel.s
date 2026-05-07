	.file	"libloop_parallel.c"
	.option nopic
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.align	1
	.globl	___parallel_loop_constant_100
	.type	___parallel_loop_constant_100, @function
___parallel_loop_constant_100:
	addi	sp,sp,-176
	sd	s1,120(sp)
	mv	s1,a3
	slliw	a3,a4,1
	addw	a3,a3,s1
	sd	s3,104(sp)
	slliw	s3,a3,2
	addiw	s3,s3,12
	addi	t1,s3,15
	sd	s0,128(sp)
	sd	s2,112(sp)
	sd	s4,96(sp)
	sd	s5,88(sp)
	sd	s6,80(sp)
	sd	s8,64(sp)
	sd	ra,136(sp)
	sd	s7,72(sp)
	sd	s9,56(sp)
	addi	s0,sp,144
	andi	t1,t1,-16
	sub	sp,sp,t1
	mv	s8,sp
	sub	sp,sp,t1
	sd	a5,8(s0)
	sd	a6,16(s0)
	sd	a7,24(s0)
	mv	s6,sp
	sub	sp,sp,t1
	mv	s5,sp
	sw	zero,0(s8)
	sub	sp,sp,t1
	sw	a1,4(s8)
	sw	a2,8(s8)
	mv	s2,a0
	mv	s4,sp
	bne	a3,zero,.L17
.L2:
	mv	a2,s3
	mv	a1,s8
	mv	a0,s6
	call	memcpy
	mv	a2,s3
	mv	a1,s8
	mv	a0,s5
	call	memcpy
	mv	a2,s3
	mv	a1,s8
	mv	a0,s4
	call	memcpy
	li	a5,1
	sw	a5,0(s6)
	li	a5,2
	sw	a5,0(s5)
	li	a5,3
	sw	a5,0(s4)
	mv	a2,s2
	li	a1,0
	mv	a3,s8
	addi	a0,s0,-128
	call	pthread_create
	mv	a2,s2
	mv	a3,s6
	li	a1,0
	addi	a0,s0,-120
	call	pthread_create
	mv	a2,s2
	mv	a3,s5
	li	a1,0
	addi	a0,s0,-112
	call	pthread_create
	mv	a2,s2
	mv	a3,s4
	li	a1,0
	addi	a0,s0,-104
	call	pthread_create
	addi	s1,s0,-128
	addi	s2,s0,-96
.L8:
	ld	a0,0(s1)
	li	a1,0
	addi	s1,s1,8
	call	pthread_join
	bne	s2,s1,.L8
	addi	sp,s0,-144
	ld	ra,136(sp)
	ld	s0,128(sp)
	ld	s1,120(sp)
	ld	s2,112(sp)
	ld	s3,104(sp)
	ld	s4,96(sp)
	ld	s5,88(sp)
	ld	s6,80(sp)
	ld	s7,72(sp)
	ld	s8,64(sp)
	ld	s9,56(sp)
	addi	sp,sp,176
	jr	ra
.L17:
	slli	a2,s1,2
	addi	a5,a2,15
	andi	a5,a5,-16
	addi	a6,s0,8
	mv	s7,sp
	sd	a6,-136(s0)
	sub	sp,sp,a5
	mv	s9,a4
	mv	a1,sp
	ble	s1,zero,.L3
	slli	a0,s1,3
	mv	a5,a6
	mv	a3,a1
	add	a0,a6,a0
.L4:
	lw	a4,0(a5)
	addi	a5,a5,8
	addi	a3,a3,4
	sw	a4,-4(a3)
	bne	a5,a0,.L4
	slli	a4,s1,32
	srli	a5,a4,29
	add	a6,a6,a5
	sd	a6,-136(s0)
.L3:
	addi	a0,s8,12
	addiw	s1,s1,3
	call	memcpy
	slliw	s1,s1,2
	ble	s9,zero,.L7
	ld	a1,-136(s0)
	slli	a4,s9,3
	add	a5,s8,s1
	add	a4,a1,a4
.L6:
	ld	a3,0(a1)
	addi	a1,a1,8
	addi	a5,a5,8
	sd	a3,-8(a5)
	bne	a4,a1,.L6
.L7:
	mv	sp,s7
	j	.L2
	.size	___parallel_loop_constant_100, .-___parallel_loop_constant_100
	.ident	"GCC: () 12.2.0"
	.section	.note.GNU-stack,"",@progbits
