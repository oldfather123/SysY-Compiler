package backend.asmInstr.asmConv;

import backend.asmInstr.AsmInstr;
import backend.itemStructure.AsmOperand;

public class AsmZext extends AsmInstr {
    private AsmOperand src;
    private AsmOperand dst;
    public AsmZext(AsmOperand src, AsmOperand dst){
        super("AsmZext");
        this.src = src;
        this.dst = dst;
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
        sb.append("zext");
        sb.append("\t");
        sb.append(dst);
        sb.append(",\t");
        sb.append(src);
        return sb.toString();
    }
}
