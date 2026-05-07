package backend.asm.instr.rv.others;

import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.MoveIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.FReg;
import backend.asm.ASMValue;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;


import java.util.Collections;
import java.util.List;

public class RVMove extends RVIns implements MoveIns {
    private Reg src;
    private boolean frozenFlag = false;

    public RVMove(Reg src, ASMBasicBlock parentBlock, Reg reg) {
        super(parentBlock, reg);
        this.src = src;

        this.addUsedVal(src);
    }

    public RVMove(Reg src, ASMBasicBlock parentBlock, Reg reg, boolean frozenFlag) {
        super(parentBlock, reg);
        this.src = src;
        this.frozenFlag = frozenFlag;

        this.addUsedVal(src);
    }

    @Override
    public boolean isFrozen() {
        return frozenFlag;
    }

    @Override
    public Reg getSrc() {
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
        this.src = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        StringBuilder sb = new StringBuilder();
        if (src instanceof FReg) {
            sb.append("fmv.s ");
            sb.append(this.printAsOperand());
            sb.append(", ");
            sb.append(src.printAsOperand());
        } else {
            sb.append("mv ");
            sb.append(this.printAsOperand());
            sb.append(", ");
            sb.append(src.printAsOperand());
        }
        return sb.toString();
    }
}
