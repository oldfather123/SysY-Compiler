package backend.asmInstr.asmBr;

import backend.asmInstr.AsmInstr;
import backend.itemStructure.AsmBlock;
import backend.itemStructure.AsmOperand;

public class AsmBnez extends AsmInstr {
    private AsmOperand cond;
    private AsmBlock target;
    public AsmBnez(AsmOperand cond, AsmBlock target) {
        super("AsmBnez");
        this.cond = cond;
        this.target = target;
        addUseReg(this.cond,cond);
    }
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("bnez\t");
        sb.append(cond);
        sb.append(",\t");
        sb.append(target);
        return sb.toString();
    }
    public void ReSetSrc(AsmOperand src) {
        this.cond = src;
    }

    public AsmBlock getTarget() {
        return target;
    }

    public void setTarget(AsmBlock target) {
        this.target = target;
    }

}
