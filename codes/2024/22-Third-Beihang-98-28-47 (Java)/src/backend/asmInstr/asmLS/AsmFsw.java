package backend.asmInstr.asmLS;

import backend.itemStructure.AsmOperand;

public class AsmFsw extends AsmS{
    public int isPassIarg = 0;
    public AsmFsw(AsmOperand src, AsmOperand addr, AsmOperand offset) {
        super();
        changeSrc(src);
        changeAddr(addr);
        changeOffset(offset);
    }

    public AsmFsw(AsmOperand src, AsmOperand addr, AsmOperand offset, int isPassIarg) {
        super();
        changeSrc(src);
        changeAddr(addr);
        changeOffset(offset);
        this.isPassIarg = isPassIarg;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("fsw\t");
        sb.append(src);
        sb.append(",\t");
        sb.append(offset);
        sb.append("(");
        sb.append(addr);
        sb.append(")");
        return sb.toString();
    }
}
