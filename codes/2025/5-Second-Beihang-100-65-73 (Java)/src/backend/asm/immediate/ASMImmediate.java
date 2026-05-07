package backend.asm.immediate;

import IO.Arg;
import backend.asm.ASMValue;

/**
 * 立即数
 */
public class ASMImmediate extends ASMValue {
    public static final ASMImmediate ZERO = new ASMImmediate(0);
    public static final ASMImmediate One = new ASMImmediate(1);
    private final int imm;
    private long longImm = 0;

    public ASMImmediate(int imm) {
        this.imm = imm;
    }

    public ASMImmediate(long imm) {
        this.longImm = imm;
        this.imm = (int) imm;
    }

    public int getImm() {
        return imm;
    }

    @Override
    public String toString() {
        return this.printAsOperand();
    }

    @Override
    public String printAsOperand() {
        if (Arg.ARG.getFramework() == Arg.Framework.ARM) {
            return "#" + this.getImm();
        } else {
            if (longImm != 0) {
                return String.valueOf(this.longImm);
            } else {
                return String.valueOf(getImm());
            }
        }
    }
}
