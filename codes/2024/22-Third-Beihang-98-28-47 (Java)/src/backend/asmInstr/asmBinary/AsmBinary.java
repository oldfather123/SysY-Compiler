package backend.asmInstr.asmBinary;

import backend.asmInstr.AsmInstr;
import backend.itemStructure.AsmOperand;

public class AsmBinary extends AsmInstr {
    protected AsmOperand dst, src1, src2;

    public AsmBinary(AsmOperand dst, AsmOperand src1, AsmOperand src2) {
        super("AsmBinary");
        changeDef(dst);
        changeUse1(src1);
        changeUse2(src2);
    }

    public void changeDef(AsmOperand dst) {
        addDefReg(this.dst, dst);
        this.dst = dst;
    }

    public void changeUse1(AsmOperand src1) {
        addUseReg(this.src1, src1);
        this.src1 = src1;
    }

    public void changeUse2(AsmOperand src2) {
        addUseReg(this.src2, src2);
        this.src2 = src2;
    }
    public void ReSetSrc(int index,AsmOperand src) {
        if (index == 0) {
            this.src1 = src;
        } else if (index == 1) {
            this.src2 = src;
        }
    }

    public void ReSetDst(AsmOperand dst) {
        this.dst = dst;
    }
    public void changeSrc1(AsmOperand src1) {
        this.src1 = src1;
    }
    public void changeSrc2(AsmOperand src2) {
        this.src2 = src2;
    }

    public AsmOperand getSrc1() {
        return src1;
    }
}
