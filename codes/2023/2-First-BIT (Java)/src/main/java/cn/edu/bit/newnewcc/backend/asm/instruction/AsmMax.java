package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.List;
import java.util.Objects;
import java.util.Set;

public class AsmMax extends AsmInstruction {
    public AsmMax(IntRegister dest, IntRegister source1, IntRegister source2) {
        super(
            Objects.requireNonNull(dest),
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2)
        );
    }

    @Override
    public String toString() {
        return String.format("AsmMax(%s, %s, %s)", getOperand(1), getOperand(2), getOperand(3));
    }

    @Override
    public String emit() {
        return String.format("\tmax %s, %s, %s\n", getOperand(1).emit(), getOperand(2).emit(), getOperand(3).emit());
    }

    @Override
    public Set<Register> getDef() {
        return Set.of((Register) getOperand(1));
    }

    @Override
    public Set<Register> getUse() {
        return Set.copyOf(List.of((Register) getOperand(2), (Register) getOperand(3)));
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
