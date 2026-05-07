package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.*;

import java.util.List;
import java.util.Objects;
import java.util.Set;

/**
 * 汇编加指令，分为普通加和加立即数两种
 */
public class AsmAdd extends AsmInstruction {
    public enum Opcode {
        ADD("add"),
        ADDI("addi"),
        ADDW("addw"),
        ADDIW("addiw"),
        FADDS("fadd.s");

        private final String name;

        Opcode(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    private final Opcode opcode;

    public AsmAdd(IntRegister dest, IntRegister source1, AsmOperand source2, int bitLength) {
        super(Objects.requireNonNull(dest), Objects.requireNonNull(source1), Objects.requireNonNull(source2));

        if (bitLength != 64 && bitLength != 32)
            throw new IllegalArgumentException();

        if (source2 instanceof Register) {
            if (bitLength == 64) opcode = Opcode.ADD;
            else opcode = Opcode.ADDW;
        } else if (source2 instanceof Immediate || source2 instanceof Label || source2 instanceof MemoryAddress) {
            if (bitLength == 64) opcode = Opcode.ADDI;
            else opcode = Opcode.ADDIW;
        } else {
            throw new IllegalArgumentException();
        }
    }
    public AsmAdd(FloatRegister dest, FloatRegister source1, FloatRegister source2) {
        super(Objects.requireNonNull(dest), Objects.requireNonNull(source1), Objects.requireNonNull(source2));
        opcode = Opcode.FADDS;
    }

    public Opcode getOpcode() {
        return opcode;
    }

    @Override
    public String toString() {
        return String.format("AsmAdd(%s, %s, %s, %s)", getOpcode(), getOperand(1), getOperand(2), getOperand(3));
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
            case ADD, ADDW, FADDS -> Set.copyOf(List.of((Register) getOperand(2), (Register) getOperand(3)));
            case ADDI, ADDIW -> Set.of((Register) getOperand(2));
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
