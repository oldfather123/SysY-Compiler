package backend.asm.instr.arm.others;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * 加载 global_var 所在的 4KB 对齐页地址  `ADRP X0, global_var` <==> `X0 = page(global_var)`
 * 配合 ADD 实现加载全局变量的地址
 */
public class ARMAdrp extends ARMIns implements PureIns {
    private final String labelName;

    public ARMAdrp(String labelName, ASMBasicBlock parentBB, Reg reg) {
        super(parentBB, reg);
        this.labelName = labelName;
    }

    @Override
    public List<ASMValue> getOperands() {
        return null;
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        throw new RuntimeException("ADRP 指令不接受操作数");
    }

    @Override
    protected String printIns() {
        return "ADRP " + this.printAsOperand() + ", " + labelName;
    }
}
