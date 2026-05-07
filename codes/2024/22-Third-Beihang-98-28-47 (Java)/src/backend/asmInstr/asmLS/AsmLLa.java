package backend.asmInstr.asmLS;

import backend.itemStructure.AsmOperand;
import backend.regs.AsmFVirReg;

public class AsmLLa extends AsmL{
    public AsmLLa(AsmOperand dst, AsmOperand src) {
        changeDst(dst);
        changeSrc(src);
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        if(dst instanceof AsmFVirReg)
            sb.append("flla\t");
        else
            sb.append("lla\t");
        sb.append(dst);
        sb.append(",\t");
        sb.append(src);
        return sb.toString();
    }
}
