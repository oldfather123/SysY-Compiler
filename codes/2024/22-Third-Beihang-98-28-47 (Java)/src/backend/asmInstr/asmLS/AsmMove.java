package backend.asmInstr.asmLS;

import backend.itemStructure.AsmImm12;
import backend.itemStructure.AsmImm32;
import backend.itemStructure.AsmLabel;
import backend.itemStructure.AsmOperand;
import backend.regs.AsmFPhyReg;
import backend.regs.AsmFVirReg;

public class AsmMove extends AsmL {
    public AsmMove(AsmOperand dst, AsmOperand src) {
        changeDst(dst);
        changeSrc(src);
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (src instanceof AsmFPhyReg || src instanceof AsmFVirReg) {
            sb.append("fmv.s\t");
            sb.append(dst);
            sb.append(",\t");
            sb.append(src);
        } else if (src instanceof AsmLabel) {
            sb.append("la\t");
            sb.append(dst);
            sb.append(",\t");
            sb.append(src);
        } else if (src instanceof AsmImm12 || src instanceof AsmImm32) {
            sb.append("li\t");
            sb.append(dst);
            sb.append(",\t");
            sb.append(src);
        } else {
            sb.append("mv\t");
            sb.append(dst);
            sb.append(",\t");
            sb.append(src);
        }
        return sb.toString();
    }
}
