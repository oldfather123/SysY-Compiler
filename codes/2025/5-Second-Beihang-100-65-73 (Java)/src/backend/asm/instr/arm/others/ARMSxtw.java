package backend.asm.instr.arm.others;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Collections;
import java.util.List;

/**
 * Sign-Extend Word 符号拓展
 */
public class ARMSxtw extends ARMIns implements PureIns {
    private Reg src;

    public ARMSxtw(Reg src, ASMBasicBlock parentBlock, Reg reg) {
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
            throw new RuntimeException("SXTW 指令接受且只接受一个操作数");
        }
        modifyUse(src, values.getFirst());
        this.src = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        return "SXTW " + this.printAsOperand() + ", " + src.printAsOperand();
    }
}
