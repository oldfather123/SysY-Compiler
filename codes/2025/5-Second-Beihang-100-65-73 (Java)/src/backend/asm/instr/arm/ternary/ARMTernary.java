package backend.asm.instr.arm.ternary;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public abstract class ARMTernary extends ARMIns implements PureIns {
    protected ASMValue operand1;
    protected ASMValue operand2;
    protected ASMValue operand3;

    protected ARMTernary(Reg operand1, ASMValue operand2, ASMValue operand3, ASMBasicBlock parentBlock, Reg reg) {
        super(parentBlock, reg);
        this.operand1 = operand1;
        this.operand2 = operand2;
        this.operand3 = operand3;

        this.addUsedVal(operand1);
        this.addUsedVal(operand2);
        this.addUsedVal(operand3);
    }

    @Override
    public List<ASMValue> getOperands() {
        return Arrays.asList(operand1, operand2, operand3);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 3) {
            throw new RuntimeException("三元运算指令接受且只接受三个操作数");
        }

        this.modifyUse(operand1, values.get(0));
        this.modifyUse(operand2, values.get(1));
        this.modifyUse(operand3, values.get(2));
        this.operand1 = values.get(0);
        this.operand2 = values.get(1);
        this.operand3 = values.get(2);
    }
}
