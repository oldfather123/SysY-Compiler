package backend.asmInstr.asmConv;

import backend.asmInstr.AsmInstr;
import backend.itemStructure.AsmOperand;

public class AsmitoF extends AsmInstr {
    private AsmOperand src;
    private AsmOperand dst;
    public AsmitoF(AsmOperand dst, AsmOperand src){
        super("AsmitoF");
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
        sb.append("fcvt.s.w");
        sb.append("\t");
        sb.append(dst);
        sb.append(",\t");
        sb.append(src);
        return sb.toString();
    }
}
