package backend.asm.instr.rv.cmp;

import backend.asm.ASMValue;
import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public class RVFeq extends RVIns implements PureIns {
    private ASMValue operand1;
    private ASMValue operand2;

    public RVFeq(ASMValue operand1, ASMValue operand2, ASMBasicBlock parentBlock, Reg reg) {
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
            throw new RuntimeException("FEQ 指令接受且只接受两个操作数");
        }

        this.modifyUse(operand1, values.get(0));
        this.modifyUse(operand2, values.get(1));
        this.operand1 = values.get(0);
        this.operand2 = values.get(1);
    }

    @Override
    protected String printIns() {
        return "feq.s " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
