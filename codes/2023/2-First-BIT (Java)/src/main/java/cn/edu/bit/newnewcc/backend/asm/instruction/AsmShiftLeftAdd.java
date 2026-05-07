package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;

import java.util.List;
import java.util.Set;

public class AsmShiftLeftAdd extends AsmInstruction {
    private final int n;

    public AsmShiftLeftAdd(int n, IntRegister dest, IntRegister source1, IntRegister source2) {
        super(dest, source1, source2);
        if (n < 1 || n > 3) throw new IllegalArgumentException();
        this.n = n;
    }

    public int getShiftLength() {
        return n;
    }

    @Override
    public String toString() {
        return String.format("AsmShiftLeftAdd(%d, %s, %s, %s)", n, getOperand(1), getOperand(2), getOperand(3));
    }

    @Override
    public String emit() {
        return String.format("\tsh%dadd %s, %s, %s\n", n, getOperand(1).emit(), getOperand(2).emit(), getOperand(3).emit());
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
