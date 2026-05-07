package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.Set;

public class AsmNop extends AsmInstruction {
    public AsmNop() {
        super(null, null, null);
    }

    @Override
    public String toString() {
        return "AsmNop()";
    }

    @Override
    public String emit() {
        return "\tnop\n";
    }

    @Override
    public Set<Register> getDef() {
        return Set.of();
    }

    @Override
    public Set<Register> getUse() {
        return Set.of();
    }

    @Override
    public boolean mayNotReturn() {
        return false;
    }

    @Override
    public boolean willNeverReturn() {
        return false;
    }

    @Override
    public boolean mayHaveSideEffects() {
        return false;
    }
}
