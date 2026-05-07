package backend.asm.instr.arm.neon.binary;

import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 注意，被定义的寄存器的旧值也是操作数
 * MLA Vd.<T>, Vn.<T>, Vm.<T>     // Vd[i] += Vn[i] * Vm[i]
 */
public class ARMVecMAdd extends ARMVecBinary {
    public ARMVecMAdd(VirFReg operand1, VirFReg operand2, ASMBasicBlock parentBB, VirFReg reg) {
        super(operand1, operand2, parentBB, reg);

        this.addUsedVal(reg);   // 乘加指令要使用自己赋值寄存器的值
    }

    @Override
    protected String printIns() {
        if (!(this.getRegister() instanceof VirFReg virFReg
                && virFReg.getPhyReg() instanceof PhyFReg fReg0
                && this.operand1.getPhyReg() instanceof PhyFReg fReg1
                && this.operand2.getPhyReg() instanceof PhyFReg fReg2)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }
        return "MLA " + fReg0.printAsFullVec() + ", " + fReg1.printAsFullVec() + ", " +
                (operand2.isVector() ? fReg2.printAsFullVec() : fReg2.printAsPartNOfVec(0));
    }

}
