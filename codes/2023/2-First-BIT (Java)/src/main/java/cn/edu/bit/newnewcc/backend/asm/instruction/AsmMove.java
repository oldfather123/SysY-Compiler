package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.FloatRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.Objects;
import java.util.Set;

public class AsmMove extends AsmInstruction {
    public enum Opcode {
        MV("mv"),
        FMVS("fmv.s");

        private final String name;

        Opcode(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    private final Opcode opcode;

    private Opcode getOpcode() {
        return opcode;
    }

    public AsmMove(Register dest, Register source) {
        super(Objects.requireNonNull(dest), Objects.requireNonNull(source), null);

        if (dest instanceof IntRegister && source instanceof IntRegister)
            opcode = Opcode.MV;
        else if (dest instanceof FloatRegister && source instanceof FloatRegister)
            opcode = Opcode.FMVS;
        else
            throw new IllegalArgumentException();
    }

    @Override
    public String emit() {
        return String.format("\t%s %s, %s\n", getOpcode().getName(), getOperand(1).emit(), getOperand(2).emit());
    }

    @Override
    public String toString() {
        return String.format("AsmMove(%s, %s, %s)", getOpcode(), getOperand(1), getOperand(2));
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
