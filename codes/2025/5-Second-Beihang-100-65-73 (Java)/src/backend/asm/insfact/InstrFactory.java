package backend.asm.insfact;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.ASMInstruction;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 为了适应不同的架构，给出生成一些指令的抽象方法
 */
public abstract class InstrFactory {
    public abstract ASMInstruction createAdd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg);

    public abstract ASMInstruction createLi(ASMImmediate imm, ASMBasicBlock parentBlock, Reg reg);

    public abstract ASMInstruction createLoad(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg,
                                              boolean isDouble, boolean isFloat);

    public abstract ASMInstruction createStore(ASMImmediate offset, ASMValue base, ASMValue value, ASMBasicBlock parentBB,
                                               boolean isDouble, boolean isFloat);

    public abstract ASMInstruction createMove(Reg src, ASMBasicBlock parentBlock, Reg dst);
}
