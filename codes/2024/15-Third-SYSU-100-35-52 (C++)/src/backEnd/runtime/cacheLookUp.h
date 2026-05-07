#include <string>
std::string yatccCacheLookUpStr = 
R"(
    .file	"yatcc_runtime.cpp"
    .option nopic
    .attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0"
    .attribute unaligned_access, 0
    .attribute stack_align, 16
    .text
    .align	1
    .globl	yatccCacheLookup
    .type	yatccCacheLookup, @function
yatccCacheLookup:
    .LFB2:
        .cfi_startproc
        slli	a1,a1,32
        li	a5,1021
        or	a2,a1,a2
        remu	a5,a2,a5
        slli	a4,a5,4
        slli	a5,a5,2
        add	a0,a0,a4
        lw	a4,12(a0)
        beq	a4,zero,.L4
        ld	a4,0(a0)
        beq	a4,a2,.L3
        sw	zero,12(a0)
    .L4:
        sd	a2,0(a0)
    .L3:
        mv	a0,a5
        ret
        .cfi_endproc
    .LFE2:
        .size	yatccCacheLookup, .-yatccCacheLookup
    .ident	"GCC: (gc891d8dc23e) 13.2.0"
)";