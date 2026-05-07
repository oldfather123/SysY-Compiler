package riscv.util;

import riscv.value.AsmReg;

public class TempRegPool {
    private final AsmReg t1, t2;

    public TempRegPool(AsmReg t1, AsmReg t2) {
        this.t1 = t1;
        this.t2 = t2;
    }

    private boolean t1Free = true;
    private boolean t2Free = true;

    public AsmReg fetchReg() {
        if (t1Free) {
            t1Free = false;
            return t1;
        }
        else if (t2Free) {
            t2Free = false;
            return t2;
        }
        else throw new AssertionError("临时寄存器池已空");
    }

    public void freeReg(AsmReg r) {
        if (r == t1) t1Free = true;
        else if (r == t2) t2Free = true;
        else throw new AssertionError("意外的寄存器 " + r.toString());
    }

    // 占用指定的某个寄存器，需要手动释放
    public void occupyReg(AsmReg r) {
        if (t1Free && r == t1) t1Free = false;
        else if (t2Free && r == t2) t2Free = false;
        else throw new AssertionError("意外或已占用的寄存器 " + r.toString());
    }
}
