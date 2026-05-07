package backend.asm.instr.arm.jump;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.BranchIns;
import backend.asm.register.Reg;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.Collections;
import java.util.List;

/**
 * 比较，并在结果等于 0 的情况下进行跳转
 */
public class ARMCbz extends ARMIns implements BranchIns {
    private Reg operand;
    private final ASMBasicBlock target;

    public ARMCbz(ASMBasicBlock target, Reg operand, ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
        this.operand = operand;
        this.target = target;

        this.addUsedVal(operand);
    }

    @Override
    public ASMBasicBlock getTarget() {
        return target;
    }

    @Override
    public List<ASMValue> getOperands() {
        return Collections.singletonList(operand);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("CBZ 指令接受且只接受一个操作数");
        }

        this.modifyUse(operand, values.getFirst());
        this.operand = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        return "CBZ " + operand.printAsOperand() + ", " + target.printAsOperand();
    }
}
