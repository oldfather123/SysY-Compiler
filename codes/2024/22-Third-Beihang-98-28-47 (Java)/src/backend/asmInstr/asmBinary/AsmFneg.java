package backend.asmInstr.asmBinary;

import backend.asmInstr.AsmInstr;
import backend.itemStructure.AsmOperand;

public class AsmFneg extends AsmInstr {
    private AsmOperand dst, src;
    public AsmFneg(AsmOperand dst, AsmOperand src) {
        super("fneg");
        this.dst = dst;
        this.src = src;
        addDefReg(this.dst,dst);
        addUseReg(this.src,src);
    }
    public void ReSetSrc(int index,AsmOperand src) {
        if (index == 0) {
            this.src = src;
        }
    }
    public void ReSetDst(int index,AsmOperand dst) {
        if (index == 0) {
            this.dst = dst;
        }
    }
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("fneg.s\t");
        sb.append(dst);
        sb.append(",\t");
        sb.append(src);
        return sb.toString();
    }
}
