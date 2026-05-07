package backend.asm.instr.arm.jump;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.Reg;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.Collections;
import java.util.List;

/**
 * 跳转到寄存器，用来实现函数返回
 */
public class ARMBr extends ARMIns {
    private Reg target;

    public ARMBr(Reg target, ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
        this.target = target;

        this.addUsedVal(target);
    }

    @Override
    public List<ASMValue> getOperands() {
        return Collections.singletonList(target);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("BR 指令接受且只接受一个操作数");
        }

        this.modifyUse(target, values.getFirst());
        this.target = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        return "BR " + target.printAsOperand();
    }
}
