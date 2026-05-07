package backend.asm.instr.arm.neon;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * 	将立即数广播到 v 寄存器的 4 个 32-bit 元素中
 */
public class ARMVecMoveImm extends ARMIns {
    private ASMImmediate immediate;

    public ARMVecMoveImm(ASMImmediate immediate, ASMBasicBlock parentBlock, VirFReg register) {
        super(parentBlock, register);

        this.immediate = immediate;

        addUsedVal(immediate);
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(immediate);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("常量广播指令接受且只接受一个操作数");
        }

        this.modifyUse(immediate, values.getFirst());
        this.immediate = (ASMImmediate) values.getFirst();
    }

    @Override
    protected String printIns() {
        if (!(this.getRegister() instanceof VirFReg virFReg
                && virFReg.getPhyReg() instanceof PhyFReg fReg)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }

        return "MOVI " + fReg.printAsFullVec() + ", " + immediate.printAsOperand();
    }
}
