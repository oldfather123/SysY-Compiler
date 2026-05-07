package backend.asmInstr.asmBr;

import backend.asmInstr.AsmInstr;
import backend.itemStructure.AsmBlock;
import backend.regs.RegGeter;

public class AsmJ extends AsmInstr {
    private AsmBlock target;
    public AsmJ(AsmBlock target) {
        super("AsmJ");
        this.target = target;
    }
    public AsmJ(AsmBlock target, int IntArgRegNum,int floatArgRegNum) {
        super("AsmJ");
        this.target = target;
        for (int i = 0 ; i < IntArgRegNum ; i++) {
            addUseReg(null, RegGeter.AllRegsInt.get(i + 10));
        }
        for (int i = 0 ; i < floatArgRegNum ; i++) {
            addUseReg(null, RegGeter.AllRegsFloat.get(i + 10));
        }
    }
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("j\t");
        sb.append(target);
        return sb.toString();
    }

    public AsmBlock getTarget() {
        return target;
    }

    public void setTarget(AsmBlock target) {
        this.target = target;
    }
}
