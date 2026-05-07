package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.AsmOperand;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.List;
import java.util.Objects;
import java.util.Set;

public class AsmAnd extends AsmInstruction {
    public enum Opcode {
        AND("and"),
        ANDI("andi");

        private final String name;

        Opcode(String name) {
            this.name = name;
        }

        public final String getName() {
            return name;
        }
    }

    private final Opcode opcode;

    public AsmAnd(IntRegister dest, IntRegister source1, AsmOperand source2) {
        super(Objects.requireNonNull(dest), Objects.requireNonNull(source1), Objects.requireNonNull(source2));
        if (source2 instanceof IntRegister) opcode = Opcode.AND;
        else if (source2 instanceof Immediate) opcode = Opcode.ANDI;
        else throw new IllegalArgumentException();
    }

    public Opcode getOpcode() {
        return opcode;
    }

    @Override
    public String toString() {
        return String.format("AsmAnd(%s, %s, %s, %s)", getOpcode(), getOperand(1), getOperand(2), getOperand(3));
    }

    @Override
    public String emit() {
        return String.format("\t%s %s, %s, %s\n", getOpcode().getName(), getOperand(1).emit(), getOperand(2).emit(), getOperand(3).emit());
    }

    @Override
    public Set<Register> getDef() {
        return Set.of((Register) getOperand(1));
    }

    @Override
    public Set<Register> getUse() {
        return switch (getOpcode()) {
            case AND -> Set.copyOf(List.of((Register) getOperand(2), (Register) getOperand(3)));
            case ANDI -> Set.of((Register) getOperand(2));
        };
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
