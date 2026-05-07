package backend.asm.instr.arm.others;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.FReg;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * 按位拷贝指令，在整数和浮点寄存器之间直接拷贝二进制位而不进行类型转换
 */
public class ARMFmov extends ARMIns {
    private Reg src;

    public ARMFmov(Reg src, ASMBasicBlock block, Reg reg) {
        super(block, reg);
        this.src = src;

        this.addUsedVal(src);
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(src);
    }

    public void resetOperands(List<ASMValue> values) {
        if (values.size() != 1 || !(values.getFirst() instanceof FReg)) {
            throw new RuntimeException("FMOV 指令的操作数必须是一个浮点寄存器");
        }
        modifyUse(src, values.getFirst());
        this.src = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        return "FMOV " + this.printAsOperand() + ", " + src.printAsOperand();
    }
}
