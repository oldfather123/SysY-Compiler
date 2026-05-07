package backend.asm.instr.arm.neon.binary;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public abstract class ARMVecBinary extends ARMIns {
    protected VirFReg operand1;
    protected VirFReg operand2;

    protected ARMVecBinary(VirFReg operand1, VirFReg operand2, ASMBasicBlock parentBlock, VirFReg reg) {
        super(parentBlock, reg);
        this.operand1 = operand1;
        this.operand2 = operand2;

        this.addUsedVal(operand1);
        this.addUsedVal(operand2);
    }

    @Override
    public List<ASMValue> getOperands() {
        return Arrays.asList(operand1, operand2);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 2) {
            throw new RuntimeException("运算指令接受且只接受两个操作数");
        }

        this.modifyUse(operand1, values.get(0));
        this.modifyUse(operand2, values.get(1));
        this.operand1 = (VirFReg) values.get(0);
        this.operand2 = (VirFReg) values.get(1);
    }
}
