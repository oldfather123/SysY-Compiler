package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.AsmOperand;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.List;
import java.util.Objects;
import java.util.Set;

public class AsmIntegerCompare extends AsmInstruction {
    public enum Opcode {
        SEQZ("seqz"),
        SNEZ("snez"),
        SLT("slt"),
        SLTI("slti");

        private final String name;

        Opcode(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    public enum Condition {
        EQZ, NEZ, LT
    }

    private final Opcode opcode;
    private final Condition condition;

    private AsmIntegerCompare(Opcode opcode, Condition condition, IntRegister dest, AsmOperand source1, AsmOperand source2) {
        super(dest, source1, source2);
        this.opcode = opcode;
        this.condition = condition;
    }

    public Opcode getOpcode() {
        return opcode;
    }

    public Condition getCondition() {
        return condition;
    }

    @Override
    public String toString() {
        return switch (getOpcode()) {
            case SEQZ, SNEZ -> String.format("AsmIntegerCompare(%s, %s, %s)", getOpcode(), getOperand(1), getOperand(2));
            case SLT, SLTI -> String.format("AsmIntegerCompare(%s, %s, %s, %s)", getOpcode(), getOperand(1), getOperand(2), getOperand(3));
        };
    }

    @Override
    public String emit() {
        return switch (getOpcode()) {
            case SEQZ, SNEZ -> String.format("\t%s %s, %s\n", getOpcode().getName(), getOperand(1).emit(), getOperand(2).emit());
            case SLT, SLTI -> String.format("\t%s %s, %s, %s\n", getOpcode().getName(), getOperand(1).emit(), getOperand(2).emit(), getOperand(3).emit());
        };
    }

    @Override
    public Set<Register> getDef() {
        return Set.of((Register) getOperand(1));
    }

    @Override
    public Set<Register> getUse() {
        return switch (getOpcode()) {
            case SEQZ, SNEZ, SLTI -> Set.of((Register) getOperand(2));
            case SLT -> Set.copyOf(List.of((Register) getOperand(2), (Register) getOperand(3)));
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

    public static AsmIntegerCompare createEQZ(IntRegister dest, IntRegister source) {
        return new AsmIntegerCompare(
            Opcode.SEQZ,
            Condition.EQZ,
            Objects.requireNonNull(dest),
            Objects.requireNonNull(source),
            null
        );
    }

    public static AsmIntegerCompare createNEZ(IntRegister dest, IntRegister source) {
        return new AsmIntegerCompare(
            Opcode.SNEZ,
            Condition.NEZ,
            Objects.requireNonNull(dest),
            Objects.requireNonNull(source),
            null
        );
    }

    public static AsmIntegerCompare createLT(IntRegister dest, IntRegister source1, IntRegister source2) {
        return new AsmIntegerCompare(
            Opcode.SLT,
            Condition.LT,
            Objects.requireNonNull(dest),
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2)
        );
    }

    public static AsmIntegerCompare createLT(IntRegister dest, IntRegister source1, Immediate source2) {
        return new AsmIntegerCompare(
            Opcode.SLTI,
            Condition.LT,
            Objects.requireNonNull(dest),
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2)
        );
    }
}
