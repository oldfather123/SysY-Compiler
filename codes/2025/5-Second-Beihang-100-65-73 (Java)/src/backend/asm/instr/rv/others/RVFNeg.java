package backend.asm.instr.rv.others;

import backend.asm.ASMValue;
import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Collections;
import java.util.List;

public class RVFNeg extends RVIns implements PureIns {
    private ASMValue src;

    public RVFNeg(ASMValue src, ASMBasicBlock parentBlock, Reg reg) {
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
            throw new RuntimeException("MOVE 指令接受且只接受一个操作数");
        }
        modifyUse(src, values.getFirst());
        this.src = values.getFirst();
    }

    @Override
    protected String printIns() {
        return "fneg.s " +
                this.printAsOperand() +
                ", " +
                src.printAsOperand();
    }
}
