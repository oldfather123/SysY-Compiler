package backend.asm.insfact;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.rv.arithmetic.RVAdd;
import backend.asm.instr.rv.memory.RVLoad;
import backend.asm.instr.rv.memory.RVStore;
import backend.asm.instr.rv.others.RVLi;
import backend.asm.instr.rv.others.RVMove;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

public class InsFact4RV extends InstrFactory {
    private static final InsFact4RV INSTANCE = new InsFact4RV();

    private InsFact4RV() {}

    public static InsFact4RV getInstance() {
        return INSTANCE;
    }

    @Override
    public ASMInstruction createAdd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        return new RVAdd(operand1, operand2, parentBB, reg);
    }

    @Override
    public ASMInstruction createLi(ASMImmediate imm, ASMBasicBlock parentBlock, Reg reg) {
        return new RVLi(imm, parentBlock, reg);
    }

    @Override
    public ASMInstruction createLoad(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg,
                                     boolean isDouble, boolean isFloat) {
        return new RVLoad(offset, base, parentBB, reg, isDouble, isFloat);
    }

    @Override
    public ASMInstruction createStore(ASMImmediate offset, ASMValue base, ASMValue value, ASMBasicBlock parentBB,
                                      boolean isDouble, boolean isFloat) {
        return new RVStore(offset, base, value, parentBB, isDouble, isFloat);
    }

    @Override
    public ASMInstruction createMove(Reg src, ASMBasicBlock parentBlock, Reg dst) {
        return new RVMove(src, parentBlock, dst);
    }
}
