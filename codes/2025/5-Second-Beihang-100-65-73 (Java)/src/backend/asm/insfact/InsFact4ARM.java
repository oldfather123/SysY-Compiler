package backend.asm.insfact;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.arm.binary.ARMAdd;
import backend.asm.instr.arm.memory.ARMLoad;
import backend.asm.instr.arm.memory.ARMStore;
import backend.asm.instr.arm.others.ARMLi;
import backend.asm.instr.arm.others.ARMMove;
import backend.asm.register.Reg;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

public class InsFact4ARM extends InstrFactory {
    private static final InsFact4ARM INSTANCE = new InsFact4ARM();

    private InsFact4ARM() {}

    public static InsFact4ARM getInstance() {
        return INSTANCE;
    }

    @Override
    public ASMInstruction createAdd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        // 如果 32 位和 64 位相加需要进行符号拓展，要把 32 位放后面
        // 语义上应该只有加减法涉及不同位宽的混合，而且只有加法可能涉及到二者交换位置
        if (operand1 instanceof  VirIReg virIReg1 && operand2 instanceof VirIReg virIReg2) {
            if (!virIReg1.isDouble() && virIReg2.isDouble()) {
                return new ARMAdd(virIReg2, virIReg1, parentBB, reg);
            }
        }
        return new ARMAdd(operand1, operand2, parentBB, reg);
    }

    @Override
    public ASMInstruction createLi(ASMImmediate imm, ASMBasicBlock parentBlock, Reg reg) {
        return new ARMLi(imm, parentBlock, reg);
    }

    @Override
    public ASMInstruction createLoad(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg,
                                     boolean isDouble, boolean isFloat) {
        return new ARMLoad(offset, base, parentBB, reg);
    }

    @Override
    public ASMInstruction createStore(ASMImmediate offset, ASMValue base, ASMValue value, ASMBasicBlock parentBB,
                                      boolean isDouble, boolean isFloat) {
        return new ARMStore(offset, base, value, parentBB);
    }

    @Override
    public ASMInstruction createMove(Reg src, ASMBasicBlock parentBlock, Reg dst) {
        return new ARMMove(src, parentBlock, dst);
    }
}
