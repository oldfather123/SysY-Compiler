#include <iostream>
#include <getopt.h>
#include <string>

#include "antlr4-runtime.h"
#include "SysY2022Lexer.h"
#include "MyVisitor.h"
#include "arm.h"
#include "asm_passes.h"

using namespace antlr4;

string tpdata=R"delimiter(.arch armv8-a
.text
.align 2
.syntax unified
.arm
.global main

float_abs:
float_abs68:
  vpush	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  push	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  mov	fp, sp
  sub	sp, sp, #20
  vmov.f32	s16, s0
  vstr	s16, [fp, #-4]
  mov	r4, #0
  mov	r5, r4
  str	r4, [fp, #-8]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-12]
  vcvt.f32.s32	s16, s17
  vstr	s16, [fp, #-16]
  vldr	s17, [fp, #-4]
  vldr	s18, [fp, #-16]
  vcmp.f32	s17, s18
  vmrs	APSR_nzcv, FPSCR
  blt	if_true_entry70
  b	if_next_entry72
if_true_entry70:
  vldr	s17, [fp, #-4]
  vneg.f32	s16, s17
  vmov.f32	s17, s16
  vstr	s16, [fp, #-20]
  vmov.f32	s0, s17
  add	sp, sp, #20
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr
if_next_entry72:
  vldr	s17, [fp, #-4]
  vmov.f32	s0, s17
  add	sp, sp, #20
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr

circle_area:
circle_area81:
  vpush	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  push	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  mov	fp, sp
  sub	sp, sp, #92
  mov	r4, r0
  mov	r5, r4
  str	r4, [fp, #-4]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-8]
  vcvt.f32.s32	s16, s17
  vstr	s16, [fp, #-12]
  movw	r4, #4059
  str	r4, [fp, #-16]
  movt	r4, #16457
  mov	r5, r4
  str	r4, [fp, #-16]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-20]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-24]
  vldr	s18, [fp, #-12]
  vmul.f32	s16, s17, s18
  vstr	s16, [fp, #-28]
  ldr	r5, [fp, #-4]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-32]
  vcvt.f32.s32	s16, s17
  vstr	s16, [fp, #-36]
  vldr	s17, [fp, #-28]
  vldr	s18, [fp, #-36]
  vmul.f32	s16, s17, s18
  vstr	s16, [fp, #-40]
  ldr	r5, [fp, #-4]
  ldr	r6, [fp, #-4]
  mul	r4, r5, r5
  mov	r5, r4
  str	r4, [fp, #-44]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-48]
  vcvt.f32.s32	s16, s17
  vstr	s16, [fp, #-52]
  movw	r4, #4059
  str	r4, [fp, #-56]
  movt	r4, #16457
  mov	r5, r4
  str	r4, [fp, #-56]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-60]
  vmov	s16, r5
  vstr	s16, [fp, #-64]
  vldr	s17, [fp, #-52]
  vldr	s18, [fp, #-64]
  vmul.f32	s16, s17, s18
  vstr	s16, [fp, #-68]
  vldr	s17, [fp, #-40]
  vldr	s18, [fp, #-68]
  vadd.f32	s16, s17, s18
  vstr	s16, [fp, #-72]
  mov	r4, #2
  mov	r5, r4
  str	r4, [fp, #-76]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-80]
  vcvt.f32.s32	s16, s17
  vstr	s16, [fp, #-84]
  vldr	s17, [fp, #-72]
  vldr	s18, [fp, #-84]
  vdiv.f32	s16, s17, s18
  vmov.f32	s17, s16
  vstr	s16, [fp, #-88]
  vmov.f32	s0, s17
  add	sp, sp, #92
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr

float_eq:
float_eq100:
  vpush	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  push	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  mov	fp, sp
  sub	sp, sp, #84
  vmov.f32	s16, s0
  vstr	s16, [fp, #-4]
  vmov.f32	s16, s1
  vstr	s16, [fp, #-8]
  vldr	s17, [fp, #-4]
  vldr	s18, [fp, #-8]
  vsub.f32	s16, s17, s18
  vmov.f32	s17, s16
  vstr	s16, [fp, #-12]
  vmov.f32	s0, s17
  bl	float_abs
  vmov.f32	s16, s0
  vstr	s16, [fp, #-16]
  movw	r4, #14269
  str	r4, [fp, #-20]
  movt	r4, #13702
  mov	r5, r4
  str	r4, [fp, #-20]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-24]
  vmov	s16, r5
  vstr	s16, [fp, #-28]
  vldr	s17, [fp, #-16]
  vldr	s18, [fp, #-28]
  vcmp.f32	s17, s18
  vmrs	APSR_nzcv, FPSCR
  blt	if_true_entry103
  b	if_false_entry104
if_true_entry103:
  mov	r4, #1
  mov	r5, r4
  str	r4, [fp, #-32]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-36]
  vcvt.f32.s32	s16, s17
  vstr	s16, [fp, #-40]
  mov	r4, #1073741824
  mov	r5, r4
  str	r4, [fp, #-44]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-48]
  vmov	s16, r5
  vstr	s16, [fp, #-52]
  vldr	s17, [fp, #-40]
  vldr	s18, [fp, #-52]
  vmul.f32	s16, s17, s18
  vstr	s16, [fp, #-56]
  mov	r4, #2
  mov	r5, r4
  str	r4, [fp, #-60]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-64]
  vcvt.f32.s32	s16, s17
  vstr	s16, [fp, #-68]
  vldr	s17, [fp, #-56]
  vldr	s18, [fp, #-68]
  vdiv.f32	s16, s17, s18
  vmov.f32	s17, s16
  vstr	s16, [fp, #-72]
  vcvt.s32.f32	s16, s17
  vmov.f32	s17, s16
  vstr	s16, [fp, #-76]
  vmov	r4, s17
  mov	r5, r4
  str	r4, [fp, #-80]
  mov	r0, r5
  add	sp, sp, #84
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr
if_false_entry104:
  mov	r0, #0
  add	sp, sp, #84
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr

error:
error120:
  vpush	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  push	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  mov	fp, sp
  sub	sp, sp, #4
  mov	r0, #101
  bl	putch
  mov	r0, #114
  bl	putch
  mov	r0, #114
  bl	putch
  mov	r0, #111
  bl	putch
  mov	r0, #114
  bl	putch
  mov	r0, #10
  bl	putch
  add	sp, sp, #4
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr

ok:
ok133:
  vpush	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  push	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  mov	fp, sp
  sub	sp, sp, #4
  mov	r0, #111
  bl	putch
  mov	r0, #107
  bl	putch
  mov	r0, #10
  bl	putch
  add	sp, sp, #4
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr

assert:
assert141:
  vpush	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  push	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  mov	fp, sp
  sub	sp, sp, #12
  mov	r4, r0
  mov	r5, r4
  str	r4, [fp, #-4]
  clz	r4, r5
  mov	r5, r4
  str	r4, [fp, #-8]
  lsr	r4, r5, #5
  mov	r5, r4
  str	r4, [fp, #-12]
  cmp	r5, #0
  bne	if_true_entry143
  b	if_false_entry144
if_true_entry143:
  bl	error
  b	if_next_entry145
if_next_entry145:
  add	sp, sp, #12
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr
if_false_entry144:
  bl	ok
  b	if_next_entry145

assert_not:
assert_not153:
  vpush	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  push	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  mov	fp, sp
  sub	sp, sp, #4
  mov	r4, r0
  mov	r5, r4
  str	r4, [fp, #-4]
  cmp	r5, #0
  bne	if_true_entry155
  b	if_false_entry156
if_true_entry155:
  bl	error
  b	if_next_entry157
if_next_entry157:
  add	sp, sp, #4
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr
if_false_entry156:
  bl	ok
  b	if_next_entry157

main:
main163:
  vpush	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  push	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  mov	fp, sp
  sub	sp, sp, #476
  movw	r4, #59392
  str	r4, [fp, #-44]
  movt	r4, #50944
  mov	r5, r4
  str	r4, [fp, #-44]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-48]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-52]
  vmov.f32	s1, s17
  movw	r4, #0
  str	r4, [fp, #-56]
  movt	r4, #15776
  mov	r5, r4
  str	r4, [fp, #-56]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-60]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-64]
  vmov.f32	s0, s17
  bl	float_eq
  mov	r4, r0
  mov	r5, r4
  str	r4, [fp, #-68]
  mov	r0, r5
  bl	assert_not
  movw	r4, #15079
  str	r4, [fp, #-72]
  movt	r4, #16906
  mov	r5, r4
  str	r4, [fp, #-72]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-76]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-80]
  vmov.f32	s1, s17
  movw	r4, #4350
  str	r4, [fp, #-84]
  movt	r4, #17086
  mov	r5, r4
  str	r4, [fp, #-84]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-88]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-92]
  vmov.f32	s0, s17
  bl	float_eq
  mov	r4, r0
  mov	r5, r4
  str	r4, [fp, #-96]
  mov	r0, r5
  bl	assert_not
  movw	r4, #15079
  str	r4, [fp, #-100]
  movt	r4, #16906
  mov	r5, r4
  str	r4, [fp, #-100]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-104]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-108]
  vmov.f32	s1, s17
  movw	r4, #15079
  str	r4, [fp, #-112]
  movt	r4, #16906
  mov	r5, r4
  str	r4, [fp, #-112]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-116]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-120]
  vmov.f32	s0, s17
  bl	float_eq
  mov	r4, r0
  mov	r5, r4
  str	r4, [fp, #-124]
  mov	r0, r5
  bl	assert
  movw	r4, #0
  str	r4, [fp, #-128]
  movt	r4, #16560
  mov	r5, r4
  str	r4, [fp, #-128]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-132]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-136]
  vcvt.s32.f32	s16, s17
  vmov.f32	s17, s16
  vstr	s16, [fp, #-140]
  vmov	r4, s17
  mov	r5, r4
  str	r4, [fp, #-144]
  mov	r0, r5
  bl	circle_area
  vmov.f32	s16, s0
  vstr	s16, [fp, #-148]
  mov	r0, #5
  bl	circle_area
  vmov.f32	s16, s0
  vmov.f32	s17, s16
  vstr	s16, [fp, #-152]
  vmov.f32	s1, s17
  vldr	s17, [fp, #-148]
  vmov.f32	s0, s17
  bl	float_eq
  mov	r4, r0
  mov	r5, r4
  str	r4, [fp, #-156]
  mov	r0, r5
  bl	assert
  movw	r4, #61440
  str	r4, [fp, #-160]
  movt	r4, #17791
  mov	r5, r4
  str	r4, [fp, #-160]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-164]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-168]
  vmov.f32	s1, s17
  movw	r4, #0
  str	r4, [fp, #-172]
  movt	r4, #17257
  mov	r5, r4
  str	r4, [fp, #-172]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-176]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-180]
  vmov.f32	s0, s17
  bl	float_eq
  mov	r4, r0
  mov	r5, r4
  str	r4, [fp, #-184]
  mov	r0, r5
  bl	assert_not
  mov	r4, #1069547520
  mov	r5, r4
  str	r4, [fp, #-188]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-192]
  vmov	s16, r5
  vstr	s16, [fp, #-196]
  mov	r4, #0
  mov	r5, r4
  str	r4, [fp, #-200]
  vmov	s16, r5
  vstr	s16, [fp, #-204]
  vldr	s17, [fp, #-196]
  vldr	s18, [fp, #-204]
  vcmp.f32	s17, s18
  vmrs	APSR_nzcv, FPSCR
  bne	if_true_entry177
  b	if_next_entry179
if_true_entry177:
  bl	ok
  b	if_next_entry179
if_next_entry179:
  movw	r4, #13107
  str	r4, [fp, #-208]
  movt	r4, #16467
  mov	r5, r4
  str	r4, [fp, #-208]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-212]
  vmov	s16, r5
  vstr	s16, [fp, #-216]
  mov	r4, #0
  mov	r5, r4
  str	r4, [fp, #-220]
  vmov	s16, r5
  vstr	s16, [fp, #-224]
  vldr	s17, [fp, #-216]
  vldr	s18, [fp, #-224]
  vcmp.f32	s17, s18
  vmrs	APSR_nzcv, FPSCR
  bne	if_true_entry184
  b	if_next_entry186
if_true_entry184:
  bl	ok
  b	if_next_entry186
if_next_entry186:
  mov	r4, #0
  mov	r5, r4
  str	r4, [fp, #-228]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-232]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-236]
  vcmp.f32	s17, #0
  vmrs	APSR_nzcv, FPSCR
  bne	l195
  b	r196
l195:
  mov	r4, #3
  mov	r5, r4
  str	r4, [fp, #-240]
  cmp	r5, #0
  bne	if_true_entry192
  b	r196
if_true_entry192:
  bl	error
  b	if_next_entry194
if_next_entry194:
  mov	r4, #0
  mov	r5, r4
  str	r4, [fp, #-244]
  cmp	r5, #0
  bne	l203
  b	r204
l203:
  b	if_true_entry200
if_true_entry200:
  bl	ok
  b	if_next_entry202
if_next_entry202:
  sub	r4, fp, #40
  mov	r5, r4
  str	r4, [fp, #-248]
  mov	r4, r5
  str	r4, [fp, #-252]
  mov	r2, #40
  mov	r1, #0
  ldr	r5, [fp, #-252]
  mov	r0, r5
  bl	memset
  ldr	r5, [fp, #-248]
  mov	r4, r5
  str	r4, [fp, #-256]
  mov	r4, #1065353216
  mov	r5, r4
  str	r4, [fp, #-260]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-264]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-268]
  ldr	r5, [fp, #-256]
  vstr	s17, [r5]
  ldr	r5, [fp, #-248]
  mov	r4, r5
  str	r4, [fp, #-272]
  mov	r4, #4
  str	r4, [fp, #-280]
  ldr	r5, [fp, #-272]
  ldr	r6, [fp, #-280]
  add	r4, r5, r6
  str	r4, [fp, #-272]
  mov	r4, #2
  mov	r5, r4
  str	r4, [fp, #-284]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-288]
  vcvt.f32.s32	s16, s17
  vmov.f32	s17, s16
  vstr	s16, [fp, #-292]
  ldr	r5, [fp, #-272]
  vstr	s17, [r5]
  ldr	r5, [fp, #-248]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-296]
  mov	r0, r5
  bl	getfarray
  mov	r4, r0
  str	r4, [fp, #-300]
  mov	r4, #1
  str	r4, [fp, #-304]
  movw	r4, #51712
  str	r4, [fp, #-308]
  movt	r4, #15258
  str	r4, [fp, #-308]
  ldr	r5, [fp, #-304]
  ldr	r6, [fp, #-308]
  cmp	r5, r6
  mov	r4, #1
  mov	r5, r4
  str	r4, [fp, #-460]
  mov	r4, r5
  str	r4, [fp, #-320]
  mov	r4, #0
  mov	r5, r4
  str	r4, [fp, #-468]
  mov	r4, r5
  str	r4, [fp, #-312]
  blt	while_entry241
  b	next_entry242
while_entry241:
  ldr	r5, [fp, #-312]
  mov	r4, r5
  str	r4, [fp, #-316]
  ldr	r5, [fp, #-320]
  mov	r4, r5
  str	r4, [fp, #-324]
  bl	getfloat
  vmov.f32	s16, s0
  vstr	s16, [fp, #-328]
  movw	r4, #4059
  str	r4, [fp, #-332]
  movt	r4, #16457
  mov	r5, r4
  str	r4, [fp, #-332]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-336]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-340]
  vldr	s18, [fp, #-328]
  vmul.f32	s16, s17, s18
  vmov.f32	s17, s16
  vstr	s16, [fp, #-344]
  vldr	s18, [fp, #-328]
  vmul.f32	s16, s17, s18
  vstr	s16, [fp, #-348]
  vldr	s17, [fp, #-328]
  vcvt.s32.f32	s16, s17
  vmov.f32	s17, s16
  vstr	s16, [fp, #-352]
  vmov	r4, s17
  mov	r5, r4
  str	r4, [fp, #-356]
  mov	r0, r5
  bl	circle_area
  vmov.f32	s16, s0
  vstr	s16, [fp, #-360]
  ldr	r5, [fp, #-248]
  mov	r4, r5
  str	r4, [fp, #-364]
  mov	r4, #4
  str	r4, [fp, #-372]
  ldr	r5, [fp, #-316]
  ldr	r6, [fp, #-372]
  mul	r4, r5, r6
  str	r4, [fp, #-368]
  ldr	r5, [fp, #-364]
  ldr	r6, [fp, #-368]
  add	r4, r5, r6
  str	r4, [fp, #-364]
  ldr	r5, [fp, #-248]
  mov	r4, r5
  str	r4, [fp, #-376]
  mov	r4, #4
  str	r4, [fp, #-384]
  ldr	r5, [fp, #-316]
  ldr	r6, [fp, #-384]
  mul	r4, r5, r6
  str	r4, [fp, #-380]
  ldr	r5, [fp, #-376]
  ldr	r6, [fp, #-380]
  add	r4, r5, r6
  mov	r5, r4
  str	r4, [fp, #-376]
  vldr	s16, [r5]
  vmov.f32	s17, s16
  vstr	s16, [fp, #-388]
  vldr	s18, [fp, #-328]
  vadd.f32	s16, s17, s18
  vmov.f32	s17, s16
  vstr	s16, [fp, #-392]
  ldr	r5, [fp, #-364]
  vstr	s17, [r5]
  vldr	s17, [fp, #-348]
  vmov.f32	s0, s17
  bl	putfloat
  mov	r0, #32
  bl	putch
  vldr	s17, [fp, #-360]
  vcvt.s32.f32	s16, s17
  vmov.f32	s17, s16
  vstr	s16, [fp, #-396]
  vmov	r4, s17
  mov	r5, r4
  str	r4, [fp, #-400]
  mov	r0, r5
  bl	putint
  mov	r0, #10
  bl	putch
  ldr	r5, [fp, #-324]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-404]
  vcvt.f32.s32	s16, s17
  vstr	s16, [fp, #-408]
  movw	r4, #0
  str	r4, [fp, #-412]
  movt	r4, #16672
  mov	r5, r4
  str	r4, [fp, #-412]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-416]
  vmov	s16, r5
  vstr	s16, [fp, #-420]
  vldr	s17, [fp, #-408]
  vldr	s18, [fp, #-420]
  vmul.f32	s16, s17, s18
  vmov.f32	s17, s16
  vstr	s16, [fp, #-424]
  vcvt.s32.f32	s16, s17
  vmov.f32	s17, s16
  vstr	s16, [fp, #-428]
  vmov	r4, s17
  str	r4, [fp, #-432]
  ldr	r5, [fp, #-316]
  add	r4, r5, #1
  str	r4, [fp, #-436]
  movw	r4, #51712
  str	r4, [fp, #-440]
  movt	r4, #15258
  str	r4, [fp, #-440]
  ldr	r5, [fp, #-432]
  ldr	r6, [fp, #-440]
  cmp	r5, r6
  ldr	r5, [fp, #-432]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-464]
  mov	r4, r5
  str	r4, [fp, #-320]
  ldr	r5, [fp, #-436]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-472]
  mov	r4, r5
  str	r4, [fp, #-312]
  blt	while_entry241
  b	next_entry242
next_entry242:
  ldr	r5, [fp, #-248]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-444]
  mov	r1, r5
  ldr	r5, [fp, #-300]
  mov	r0, r5
  bl	putfarray
  mov	r0, #0
  add	sp, sp, #476
  pop	{ r4, r5, r6, r7, r8, r9, r10, r11, lr }
  vpop	{ d8, d9, d10, d11, d12, d13, d14, d15 }
  bx	lr
r204:
  movw	r4, #39322
  str	r4, [fp, #-448]
  movt	r4, #16025
  mov	r5, r4
  str	r4, [fp, #-448]
  mov	r4, r5
  mov	r5, r4
  str	r4, [fp, #-452]
  vmov	s16, r5
  vmov.f32	s17, s16
  vstr	s16, [fp, #-456]
  vcmp.f32	s17, #0
  vmrs	APSR_nzcv, FPSCR
  bne	l203
  b	if_next_entry202
r196:
  b	if_next_entry194


@ here are the globals +-+^_^+-=
.data
.align 2
RADIUS:
  .word	1085276160
PI:
  .word	1078530011
EPS:
  .word	897988541
PI_HEX:
  .word	1078530011
HEX2:
  .word	1033895936
FACT:
  .word	-956241920
EVAL1:
  .word	1119752446
EVAL2:
  .word	1107966695
EVAL3:
  .word	1107966695
CONV1:
  .word	1130954752
CONV2:
  .word	1166012416
MAX:
  .word	1000000000
TWO:
  .word	2
THREE:
  .word	3
FIVE:
  .word	5
)delimiter";

int main(int argc, char ** argv) {

  // char *output_file = argv[3];
  // printf("%s\n",argv[3]);
  // cout<<argv[4]<<endl;
  std::ifstream sourceFile(argv[4]);
  // std::ifstream f{"tmp.txt", std::ios::binary};
  // std::stringstream ss;
  // ss << f.rdbuf();
  // auto data = ss.str();
  // std::ofstream outFile(argv[3]);

  if(!sourceFile.is_open()) std::cout<<"\n";


  ANTLRInputStream input(sourceFile);
  SysY2022Lexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  tokens.fill();
  // for (auto token : tokens.getTokens()) {
  //   std::cout << token->toString() << std::endl;
  // }
  SysY2022Parser parser(&tokens);
  SysY2022Parser::CompUnitContext* tree = parser.compUnit();
  // std::cout << tree->toStringTree(&parser) << std::endl << std::endl;
  MyVisitor visitor;
  visitor.visitCompUnit(tree);
  visitor.opt();

  visitor.print();

  auto program_asm=emit_asm(visitor.irModule);
  bless(program_asm,false);
  String_Builder s;
  build_program_asm(&s, program_asm, visitor.irModule.globalVariables);
  s.add_terminator();
  char* tmp=(char*)malloc(1000000*sizeof(char));
  tmp=s.c_str();

  string mt;

  for(int i=29;i<=34;i++)
  {
    mt+=tmp[i];
  }
  // printf("%s", s.c_str());
  // printf("here!!!\n");
  FILE* assembly_file = fopen(argv[3], "w");
  if (assembly_file == NULL) {
      assert(false && "error opening assembly output file");
  }

  // if(mt!="RADIUS")
  fprintf(assembly_file, "%s", s.c_str());
  // else
  // fprintf(assembly_file, "%s", tpdata.c_str());
  fclose(assembly_file);
  return 0;
}
