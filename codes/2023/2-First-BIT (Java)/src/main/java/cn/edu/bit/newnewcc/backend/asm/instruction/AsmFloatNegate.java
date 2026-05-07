package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.FloatRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.Objects;
import java.util.Set;

public class AsmFloatNegate extends AsmInstruction {
    public AsmFloatNegate(FloatRegister dest, FloatRegister source) {
        super(Objects.requireNonNull(dest), (source), null);
    }

    @Override
    public String toString() {
        return String.format("AsmFloatNegate(%s, %s)", getOperand(1), getOperand(2));
    }

    @Override
    public String emit() {
        return String.format("\tfneg.s %s, %s\n", getOperand(1).emit(), getOperand(2).emit());
    }

    @Override
    public Set<Register> getDef() {
        return Set.of((Register) getOperand(1));
    }

    @Override
    public Set<Register> getUse() {
        return Set.of((Register) getOperand(2));
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
