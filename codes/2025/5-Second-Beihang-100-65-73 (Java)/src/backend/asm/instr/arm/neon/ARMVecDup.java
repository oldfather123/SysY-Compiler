package backend.asm.instr.arm.neon;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * 将一个标量值复制到向量寄存器的所有 lanes（元素）中
 */
public class ARMVecDup extends ARMIns {
    private Reg scalarValue;

    public ARMVecDup(Reg scalarValue, ASMBasicBlock parentBlock, VirFReg register) {
        super(parentBlock, register);
        this.scalarValue = scalarValue;

        this.addUsedVal(scalarValue);
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(scalarValue);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("DUP 指令接受且只接受一个操作数");
        }

        modifyUse(scalarValue, values.getFirst());
        this.scalarValue = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        if (!(this.getRegister() instanceof VirFReg virFReg
                && virFReg.getPhyReg() instanceof PhyFReg fReg0)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }

        return "DUP " + fReg0.printAsFullVec() + ", " + scalarValue.printAsOperand();
    }
}
