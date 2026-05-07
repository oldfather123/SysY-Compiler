package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.FloatRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.Objects;
import java.util.Set;

public class AsmConvertFloatInt extends AsmInstruction {
    public enum Opcode {
        FCVTWS("fcvt.w.s"),
        FCVTSW("fcvt.s.w");

        private final String name;

        Opcode(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    private final Opcode opcode;

    public AsmConvertFloatInt(IntRegister dest, FloatRegister source) {
        super(Objects.requireNonNull(dest), Objects.requireNonNull(source), null);
        opcode = Opcode.FCVTWS;
    }

    public AsmConvertFloatInt(FloatRegister dest, IntRegister source) {
        super(Objects.requireNonNull(dest), Objects.requireNonNull(source), null);
        opcode = Opcode.FCVTSW;
    }

    public Opcode getOpcode() {
        return opcode;
    }

    @Override
    public String toString() {
        return String.format("AsmConvertFloatInt(%s, %s, %s)", getOpcode(), getOperand(1), getOperand(2));
    }

    @Override
    public String emit() {
        return switch (getOpcode()) {
            case FCVTWS -> String.format("\t%s %s, %s, rtz\n", getOpcode().getName(), getOperand(1).emit(), getOperand(2).emit());
            case FCVTSW -> String.format("\t%s %s, %s\n", getOpcode().getName(), getOperand(1).emit(), getOperand(2).emit());
        };
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
