package backend.asmInstr.asmLS;

import backend.itemStructure.AsmOperand;
import backend.regs.AsmFVirReg;

public class AsmLa extends AsmL {
    public AsmLa(AsmOperand dst, AsmOperand src) {
        changeDst(dst);
        changeSrc(src);
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        if(dst instanceof AsmFVirReg)
            sb.append("fla\t");
        else
            sb.append("la\t");
        sb.append(dst);
        sb.append(",\t");
        sb.append(src);
        return sb.toString();
    }
}
