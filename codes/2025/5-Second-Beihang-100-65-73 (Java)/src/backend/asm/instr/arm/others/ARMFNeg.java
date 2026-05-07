package backend.asm.instr.arm.others;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Collections;
import java.util.List;

public class ARMFNeg extends ARMIns implements PureIns {
    private Reg src;

    public ARMFNeg(Reg src, ASMBasicBlock parentBlock, Reg reg) {
        super(parentBlock, reg);
        this.src = src;

        this.addUsedVal(src);
    }

    public ASMValue getSrc() {
        return src;
    }

    @Override
    public List<ASMValue> getOperands() {
        return Collections.singletonList(src);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("FNeg 指令接受且只接受一个操作数");
        }
        if (!(values.getFirst() instanceof Reg reg)) {
            throw new RuntimeException("FNeg <UNK>");
        }
        modifyUse(this.src, reg);
        this.src = reg;
    }

    @Override
    protected String printIns() {
        return "FNEG " + this.printAsOperand() + ", " + src.printAsOperand();
    }
}
