package backend.asmInstr.asmLS;

import backend.itemStructure.AsmOperand;

public class AsmSd extends AsmS {
    public int isPassIarg = 0;
    public AsmSd(AsmOperand src, AsmOperand addr, AsmOperand offset) {
        changeSrc(src);
        changeAddr(addr);
        changeOffset(offset);
    }

    public AsmSd(AsmOperand src, AsmOperand addr, AsmOperand offset, int isPassIarg) {
        changeSrc(src);
        changeAddr(addr);
        changeOffset(offset);
        this.isPassIarg = isPassIarg;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("sd\t");
        //sb.append("sw\t");
        sb.append(src);
        sb.append(",\t");
        sb.append(offset);
        sb.append("(");
        sb.append(addr);
        sb.append(")");
        return sb.toString();
    }
}
