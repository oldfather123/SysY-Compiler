package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.Set;

/**
 * 此指令为基本块末尾的占位符，仅用于标示基本块结束
 */
public class AsmBlockEnd extends AsmInstruction {
    public AsmBlockEnd() {
        super(null, null, null);
    }

    @Override
    public String toString() {
        return "AsmBlockEnd()";
    }

    @Override
    public String emit() {
        return "";
    }

    @Override
    public Set<Register> getDef() {
        throw new UnsupportedOperationException();
    }

    @Override
    public Set<Register> getUse() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean mayNotReturn() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean willNeverReturn() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean mayHaveSideEffects() {
        throw new UnsupportedOperationException();
    }
}
