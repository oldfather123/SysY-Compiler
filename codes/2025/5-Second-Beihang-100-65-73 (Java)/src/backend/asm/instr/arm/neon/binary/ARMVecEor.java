package backend.asm.instr.arm.neon.binary;

import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 向量的按位异或，自己和自己异或可以实现寄存器清零
 */
public class ARMVecEor extends ARMVecBinary {
    public ARMVecEor(VirFReg operand1, VirFReg operand2, ASMBasicBlock parentBlock, VirFReg reg) {
        super(operand1, operand2, parentBlock, reg);
    }

    @Override
    protected String printIns() {
        if (!(this.getRegister() instanceof VirFReg virFReg
                && virFReg.getPhyReg() instanceof PhyFReg fReg0
                && this.operand1.getPhyReg() instanceof PhyFReg fReg1
                && this.operand2.getPhyReg() instanceof PhyFReg fReg2)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }
        return "EOR " + fReg0.printAsFullByteVec() + ", " + fReg1.printAsFullByteVec() + ", " +
                (operand2.isVector() ? fReg2.printAsFullByteVec() : fReg2.printAsPartNOfVec(0));
    }
}
