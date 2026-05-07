package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.FloatRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.List;
import java.util.Objects;
import java.util.Set;

/**
 * 汇编加指令，分为普通加和加立即数两种
 */
public class AsmSub extends AsmInstruction {
    public enum Opcode {
        SUB("sub"),
        SUBW("subw"),
        FSUBS("fsub.s");

        private final String name;

        Opcode(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    private final Opcode opcode;

    public AsmSub(IntRegister dest, IntRegister source1, IntRegister source2, int bitLength) {
        super(Objects.requireNonNull(dest), Objects.requireNonNull(source1), Objects.requireNonNull(source2));

        if (bitLength != 64 && bitLength != 32)
            throw new IllegalArgumentException();

        if (bitLength == 64) opcode = Opcode.SUB;
        else opcode = Opcode.SUBW;
    }
    public AsmSub(FloatRegister dest, FloatRegister source1, FloatRegister source2) {
        super(Objects.requireNonNull(dest), Objects.requireNonNull(source1), Objects.requireNonNull(source2));
        opcode = Opcode.FSUBS;
    }

    public Opcode getOpcode() {
        return opcode;
    }

    @Override
    public String toString() {
        return String.format("AsmSub(%s, %s, %s, %s)", opcode.getName(), getOperand(1), getOperand(2), getOperand(3));
    }

    @Override
    public String emit() {
        return String.format("\t%s %s, %s, %s\n", opcode.getName(), getOperand(1).emit(), getOperand(2).emit(), getOperand(3).emit());
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
