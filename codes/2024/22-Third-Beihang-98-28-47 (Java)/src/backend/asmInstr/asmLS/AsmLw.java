package backend.asmInstr.asmLS;

import backend.itemStructure.AsmOperand;

public class AsmLw extends AsmL {
    public int isPassIarg = 0;
    public AsmLw(AsmOperand dst, AsmOperand src, AsmOperand offset) {
        changeDst(dst);
        changeSrc(src);
        changeOffset(offset);
    }
    public AsmLw(AsmOperand dst, AsmOperand src, AsmOperand offset, int isPassIarg) {
        changeDst(dst);
        changeSrc(src);
        changeOffset(offset);
        this.isPassIarg = isPassIarg;
    }
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("lw\t");
        sb.append(dst);
        sb.append(",\t");
        sb.append(offset);
        sb.append("(");
        sb.append(src);
        sb.append(")");
        return sb.toString();
    }
}
