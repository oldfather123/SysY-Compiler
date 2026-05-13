    .section .rodata
.LC_int:
    .string "%d"
.LC_char:
    .string "%c"
.LC_float:
    .string "%a"
.LC_int_arr:
    .string "%d:"
.LC_int_elem:
    .string " %d"
.LC_float_arr:
    .string "%d:"
.LC_float_elem:
    .string " %a"
.LC_newline:
    .string "\n"

    .text

# int getint()
    .globl getint
    .type getint, @function
getint:
    addi sp, sp, -16
    sd ra, 8(sp)
    addi a1, sp, 4
    la a0, .LC_int
    call scanf
    lw a0, 4(sp)
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

# int getch()
    .globl getch
    .type getch, @function
getch:
    addi sp, sp, -16
    sd ra, 8(sp)
    addi a1, sp, 4
    la a0, .LC_char
    call scanf
    lbu a0, 4(sp)
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

# float getfloat()
    .globl getfloat
    .type getfloat, @function
getfloat:
    addi sp, sp, -16
    sd ra, 8(sp)
    addi a1, sp, 4
    la a0, .LC_float
    call scanf
    flw fa0, 4(sp)
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

# int getarray(int a[])
    .globl getarray
    .type getarray, @function
getarray:
    addi sp, sp, -48
    sd ra, 40(sp)
    sd s0, 32(sp)
    sd s1, 24(sp)
    sd s2, 16(sp)
    mv s1, a0           # s1 = array base
    addi a1, sp, 12
    la a0, .LC_int
    call scanf           # read n
    lw s0, 12(sp)        # s0 = n
    li s2, 0             # i = 0
.Lgetarr_loop:
    bge s2, s0, .Lgetarr_end
    slli t2, s2, 2
    add a1, s1, t2       # &a[i]
    la a0, .LC_int
    call scanf
    addi s2, s2, 1
    j .Lgetarr_loop
.Lgetarr_end:
    mv a0, s0
    ld ra, 40(sp)
    ld s0, 32(sp)
    ld s1, 24(sp)
    ld s2, 16(sp)
    addi sp, sp, 48
    ret

# int getfarray(float a[])
    .globl getfarray
    .type getfarray, @function
getfarray:
    addi sp, sp, -48
    sd ra, 40(sp)
    sd s0, 32(sp)
    sd s1, 24(sp)
    sd s2, 16(sp)
    mv s1, a0
    addi a1, sp, 12
    la a0, .LC_int
    call scanf
    lw s0, 12(sp)
    li s2, 0
.Lgetfarr_loop:
    bge s2, s0, .Lgetfarr_end
    slli t2, s2, 2
    add a1, s1, t2
    la a0, .LC_float
    call scanf
    addi s2, s2, 1
    j .Lgetfarr_loop
.Lgetfarr_end:
    mv a0, s0
    ld ra, 40(sp)
    ld s0, 32(sp)
    ld s1, 24(sp)
    ld s2, 16(sp)
    addi sp, sp, 48
    ret

# void putint(int n)
    .globl putint
    .type putint, @function
putint:
    addi sp, sp, -16
    sd ra, 8(sp)
    mv a1, a0
    la a0, .LC_int
    call printf
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

# void putch(int c)
    .globl putch
    .type putch, @function
putch:
    addi sp, sp, -16
    sd ra, 8(sp)
    call putchar
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

# void putfloat(float f)
    .globl putfloat
    .type putfloat, @function
putfloat:
    addi sp, sp, -16
    sd ra, 8(sp)
    fcvt.d.s fa0, fa0
    fmv.x.d a1, fa0
    la a0, .LC_float
    call printf
    ld ra, 8(sp)
    addi sp, sp, 16
    ret

# void putarray(int n, int a[])
    .globl putarray
    .type putarray, @function
putarray:
    addi sp, sp, -48
    sd ra, 40(sp)
    sd s0, 32(sp)
    sd s1, 24(sp)
    sd s2, 16(sp)
    mv s0, a0
    mv s1, a1
    mv a1, a0
    la a0, .LC_int_arr
    call printf
    li s2, 0
.Lputarr_loop:
    bge s2, s0, .Lputarr_end
    slli t1, s2, 2
    add t1, s1, t1
    lw a1, 0(t1)
    la a0, .LC_int_elem
    call printf
    addi s2, s2, 1
    j .Lputarr_loop
.Lputarr_end:
    li a0, 10
    call putchar
    ld ra, 40(sp)
    ld s0, 32(sp)
    ld s1, 24(sp)
    ld s2, 16(sp)
    addi sp, sp, 48
    ret

# void putfarray(int n, float a[])
    .globl putfarray
    .type putfarray, @function
putfarray:
    addi sp, sp, -48
    sd ra, 40(sp)
    sd s0, 32(sp)
    sd s1, 24(sp)
    sd s2, 16(sp)
    mv s0, a0
    mv s1, a1
    mv a1, a0
    la a0, .LC_float_arr
    call printf
    li s2, 0
.Lputfarr_loop:
    bge s2, s0, .Lputfarr_end
    slli t1, s2, 2
    add t1, s1, t1
    flw fa0, 0(t1)
    fcvt.d.s fa0, fa0
    fmv.x.d a1, fa0
    la a0, .LC_float_elem
    call printf
    addi s2, s2, 1
    j .Lputfarr_loop
.Lputfarr_end:
    li a0, 10
    call putchar
    ld ra, 40(sp)
    ld s0, 32(sp)
    ld s1, 24(sp)
    ld s2, 16(sp)
    addi sp, sp, 48
    ret

# void putf(const char *fmt, ...)
    .globl putf
    .type putf, @function
putf:
    addi sp, sp, -208
    sd ra, 200(sp)
    sd s0, 192(sp)
    addi s0, sp, 208
    # Save variadic integer args
    sd a1, -88(s0)
    sd a2, -80(s0)
    sd a3, -72(s0)
    sd a4, -64(s0)
    sd a5, -56(s0)
    sd a6, -48(s0)
    sd a7, -40(s0)
    # Build va_list: stack pointer + gr_offs=0 + vr_offs=0
    addi t0, s0, -88     # start of saved gp args
    sd t0, -104(s0)      # __stack
    sd zero, -112(s0)    # __gr_offs
    sd zero, -120(s0)    # __vr_offs
    # Call vfprintf(stdout, fmt, &va_list)
    # Load stdout pointer
    la t0, stdout
    ld a0, 0(t0)
    mv a1, a0            # a1 = stdout (for vfprintf, a0=FILE*, a1=fmt)
    # Actually: vfprintf(FILE *stream, const char *format, va_list ap)
    # a0 = FILE* (stdout), a1 = fmt, a2 = &va_list
    # But a0 already has fmt from the caller! We need to rearrange.
    # On entry: a0=fmt, a1..a7=args
    # We saved them. Now:
    # a0 = stdout, a1 = fmt (original a0), a2 = &va_list
    la t0, stdout
    ld a0, 0(t0)
    # a1 needs to be the original a0 (fmt). We saved a1-a7 but not a0!
    # a0 (fmt) is already in a0 on entry. Let's save it too.
    # Actually, a0 = fmt on entry. We need to save it.
    sd a0, -32(s0)       # save original a0 (fmt)
    # Now reload: a0 = stdout, a1 = fmt
    la t0, stdout
    ld a0, 0(t0)
    ld a1, -32(s0)       # a1 = fmt
    addi a2, s0, -120    # a2 = &va_list
    call vfprintf
    ld ra, 200(sp)
    ld s0, 192(sp)
    addi sp, sp, 208
    ret

# void starttime() - no-op
    .globl starttime
    .type starttime, @function
starttime:
    ret

# void stoptime() - no-op
    .globl stoptime
    .type stoptime, @function
stoptime:
    ret

# void _sysy_starttime(int lineno) - no-op
    .globl _sysy_starttime
    .type _sysy_starttime, @function
_sysy_starttime:
    ret

# void _sysy_stoptime(int lineno) - no-op
    .globl _sysy_stoptime
    .type _sysy_stoptime, @function
_sysy_stoptime:
    ret
