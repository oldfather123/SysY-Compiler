package backend.asm.instr.arm.others;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.LiIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * 实际上是使用 MOVZ(Move with Zero) + MOVK(Move with Keep) 两个指令实现的
 */
public class ARMLi extends ARMIns implements LiIns, PureIns {
    private final ASMImmediate imm;

    public ARMLi(ASMImmediate imm, ASMBasicBlock parentBlock, Reg reg) {
        super(parentBlock, reg);
        this.imm = imm;

        this.addUsedVal(imm);
    }

    @Override
    public ASMImmediate getImmediate() {
        return imm;
    }

    @Override
    public List<ASMValue> getOperands() {
        return null;
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        throw new RuntimeException("LI 指令不接受操作数");
    }

    @Override
    protected String printIns() {
        StringBuilder sb = new StringBuilder();
        int immNum = imm.getImm();

        sb.append("MOVZ ").append(this.printAsOperand()).append(", #").append(immNum & 0xFFFF);
        immNum >>>= 16;
        if (immNum != 0) {
            sb.append("\n\t\t").append("MOVK ").append(this.printAsOperand()).append(", #").append(immNum & 0xFFFF).append(", LSL #16");
        }

        return sb.toString();
    }
}
