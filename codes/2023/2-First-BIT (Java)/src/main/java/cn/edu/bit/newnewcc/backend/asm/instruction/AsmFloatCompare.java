package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.FloatRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.List;
import java.util.Objects;
import java.util.Set;

public class AsmFloatCompare extends AsmInstruction{
    public enum Opcode {
        FEQS("feq.s"),
        FLTS("flt.s"),
        FLES("fle.s");

        private final String name;

        Opcode(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    public enum Condition {
        EQ, LT, LE
    }

    private final Opcode opcode;
    private final Condition condition;

    private AsmFloatCompare(Opcode opcode, Condition condition, IntRegister dest, FloatRegister source1, FloatRegister source2) {
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
        return String.format("AsmFloatCompare(%s, %s, %s, %s)", getOpcode(), getOperand(1), getOperand(2), getOperand(3));
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

    public static AsmFloatCompare createEQ(IntRegister dest, FloatRegister source1, FloatRegister source2) {
        return new AsmFloatCompare(
            Opcode.FEQS,
            Condition.EQ,
            Objects.requireNonNull(dest),
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2)
        );
    }

    public static AsmFloatCompare createLT(IntRegister dest, FloatRegister source1, FloatRegister source2) {
        return new AsmFloatCompare(
            Opcode.FLTS,
            Condition.LT,
            Objects.requireNonNull(dest),
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2)
        );
    }

    public static AsmFloatCompare createLE(IntRegister dest, FloatRegister source1, FloatRegister source2) {
        return new AsmFloatCompare(
            Opcode.FLES,
            Condition.LE,
            Objects.requireNonNull(dest),
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2)
        );
    }
}
