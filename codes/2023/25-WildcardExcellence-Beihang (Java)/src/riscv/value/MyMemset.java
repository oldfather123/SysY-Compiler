package riscv.value;

public class MyMemset {
    public static final String asm = """
            # a0: base address, a1: words to memset
            memset_zero:
              start:
                li t0, 64
                bge a1, t0, w64
                li t0, 32
                bge a1, t0, w32
                li t0, 16
                bge a1, t0, w16
                li t0, 8
                bge a1, t0, w8
                li t0, 4
                bge a1, t0, w4
                li t0, 2
                bge a1, t0, w2
                li t0, 1
                bge a1, t0, w1
                ret
              w64:
                sd zero, 248(a0)
                sd zero, 240(a0)
                sd zero, 232(a0)
                sd zero, 224(a0)
                sd zero, 216(a0)
                sd zero, 208(a0)
                sd zero, 200(a0)
                sd zero, 192(a0)
                sd zero, 184(a0)
                sd zero, 176(a0)
                sd zero, 168(a0)
                sd zero, 160(a0)
                sd zero, 152(a0)
                sd zero, 144(a0)
                sd zero, 136(a0)
                sd zero, 128(a0)
              w32:
                sd zero, 120(a0)
                sd zero, 112(a0)
                sd zero, 104(a0)
                sd zero, 96(a0)
                sd zero, 88(a0)
                sd zero, 80(a0)
                sd zero, 72(a0)
                sd zero, 64(a0)
              w16:
                sd zero, 56(a0)
                sd zero, 48(a0)
                sd zero, 40(a0)
                sd zero, 32(a0)
              w8:
                sd zero, 24(a0)
                sd zero, 16(a0)
              w4:
                sd zero, 8(a0)
              w2:
                sw zero, 4(a0)
              w1:
                sw zero, 0(a0)
                sub a1, a1, t0
                blt zero, a1, start
                ret
            """;
}
