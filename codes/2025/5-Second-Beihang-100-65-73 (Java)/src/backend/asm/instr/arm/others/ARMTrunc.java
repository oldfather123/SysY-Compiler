package backend.asm.instr.arm.others;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Collections;
import java.util.List;

/**
 * 不可以合并的 Move，用来将 64 位数据截断成 32 位
 */
public class ARMTrunc extends ARMIns implements PureIns {
    private VirIReg src;

    public ARMTrunc(VirIReg src, ASMBasicBlock parentBlock, Reg reg) {
        super(parentBlock, reg);
        this.src = src;

        this.addUsedVal(src);
    }

    @Override
    public List<ASMValue> getOperands() {
        return Collections.singletonList(src);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("TRUNC 指令接受且只接受一个操作数");
        }
        modifyUse(src, values.getFirst());
        this.src = (VirIReg) values.getFirst();
    }

    @Override
    protected String printIns() {
        return  "MOV " + this.printAsOperand() + ", " + src.printAsWord();
    }
}
