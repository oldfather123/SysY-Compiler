package backend.asmInstr.asmLS;

import backend.itemStructure.AsmOperand;

public class AsmFlw extends AsmL {
    public int isPassIarg = 0;
    public AsmFlw(AsmOperand dst, AsmOperand src, AsmOperand offset) {
        super();
        changeDst(dst);
        changeSrc(src);
        changeOffset(offset);
    }
    public AsmFlw(AsmOperand dst, AsmOperand src, AsmOperand offset, int isPassIarg) {
        super();
        changeDst(dst);
        changeSrc(src);
        changeOffset(offset);
        this.isPassIarg = isPassIarg;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("flw\t");
        sb.append(dst);
        sb.append(",\t");
        sb.append(offset);
        sb.append("(");
        sb.append(src);
        sb.append(")");
        return sb.toString();
    }
}
