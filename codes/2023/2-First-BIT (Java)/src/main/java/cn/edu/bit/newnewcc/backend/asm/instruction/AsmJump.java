package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.AsmOperand;
import cn.edu.bit.newnewcc.backend.asm.operand.Label;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.List;
import java.util.Objects;
import java.util.Set;

public class AsmJump extends AsmInstruction {
    public enum Opcode {
        J("j"),
        BLTZ("bltz"),
        BGTZ("bgtz"),
        BLEZ("blez"),
        BGEZ("bgez"),
        BEQZ("beqz"),
        BNEZ("bnez"),
        BLT("blt"),
        BGT("bgt"),
        BLE("ble"),
        BGE("bge"),
        BEQ("beq"),
        BNE("bne");

        private final String name;

        Opcode(String name) {
            this.name = name;
        }

        public String getName() {
            return name;
        }
    }

    public enum Condition {
        UNCONDITIONAL,
        LTZ,
        GTZ,
        LEZ,
        GEZ,
        EQZ,
        NEZ,
        LT,
        GT,
        LE,
        GE,
        EQ,
        NE,
    }

    private final Opcode opcode;
    private final Condition condition;

    private AsmJump(Opcode opcode, Condition condition, AsmOperand operand1, AsmOperand operand2, AsmOperand operand3) {
        super(operand1, operand2, operand3);
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
            case J ->
                String.format("AsmJump(%s, %s)", getOpcode(), getOperand(1));
            case BLTZ, BGTZ, BLEZ, BGEZ, BEQZ, BNEZ ->
                String.format("AsmJump(%s, %s, %s)", getOpcode(), getOperand(1), getOperand(2));
            case BLT, BGT, BLE, BGE, BEQ, BNE ->
                String.format("AsmJump(%s, %s, %s, %s)", getOpcode(), getOperand(1), getOperand(2), getOperand(3));
        };
    }

    @Override
    public String emit() {
        return switch (getOpcode()) {
            case J ->
                String.format("\t%s %s\n", getOpcode().getName(), getOperand(1).emit());
            case BLTZ, BGTZ, BLEZ, BGEZ, BEQZ, BNEZ ->
                String.format("\t%s %s, %s\n", getOpcode().getName(), getOperand(1).emit(), getOperand(2).emit());
            case BLT, BGT, BLE, BGE, BEQ, BNE ->
                String.format("\t%s %s, %s, %s\n", getOpcode().getName(), getOperand(1).emit(), getOperand(2).emit(), getOperand(3).emit());
        };
    }

    @Override
    public Set<Register> getDef() {
        return Set.of();
    }

    @Override
    public Set<Register> getUse() {
        return switch (getCondition()) {
            case UNCONDITIONAL -> Set.of();
            case LTZ, GTZ, LEZ, GEZ, EQZ, NEZ -> Set.of((Register) getOperand(1));
            case LT, GT, LE, GE, EQ, NE -> Set.copyOf(List.of((Register) getOperand(1), (Register) getOperand(2)));
        };
    }

    @Override
    public boolean mayNotReturn() {
        return true;
    }

    @Override
    public boolean willNeverReturn() {
        return getCondition() == Condition.UNCONDITIONAL;
    }

    @Override
    public boolean mayHaveSideEffects() {
        return false;
    }

    public static AsmJump createUnconditional(Label targetLabel) {
        return new AsmJump(
            Opcode.J,
            Objects.requireNonNull(Condition.UNCONDITIONAL),
            targetLabel,
            null,
            null
        );
    }

    public static AsmJump createLTZ(Label targetLabel, IntRegister source) {
        return new AsmJump(
            Opcode.BLTZ,
            Condition.LTZ,
            Objects.requireNonNull(source),
            Objects.requireNonNull(targetLabel),
            null
        );
    }

    public static AsmJump createGTZ(Label targetLabel, IntRegister source) {
        return new AsmJump(
            Opcode.BGTZ,
            Condition.GTZ,
            Objects.requireNonNull(source),
            Objects.requireNonNull(targetLabel),
            null
        );
    }

    public static AsmJump createLEZ(Label targetLabel, IntRegister source) {
        return new AsmJump(
            Opcode.BLEZ,
            Condition.LEZ,
            Objects.requireNonNull(source),
            Objects.requireNonNull(targetLabel),
            null
        );
    }

    public static AsmJump createGEZ(Label targetLabel, IntRegister source) {
        return new AsmJump(
            Opcode.BGEZ,
            Condition.GEZ,
            Objects.requireNonNull(source),
            Objects.requireNonNull(targetLabel),
            null
        );
    }

    public static AsmJump createEQZ(Label targetLabel, IntRegister source) {
        return new AsmJump(
            Opcode.BEQZ,
            Condition.EQZ,
            Objects.requireNonNull(source),
            Objects.requireNonNull(targetLabel),
            null
        );
    }

    public static AsmJump createNEZ(Label targetLabel, IntRegister source) {
        return new AsmJump(
            Opcode.BNEZ,
            Condition.NEZ,
            Objects.requireNonNull(source),
            Objects.requireNonNull(targetLabel),
            null
        );
    }

    public static AsmJump createLT(Label targetLabel, IntRegister source1, IntRegister source2) {
        return new AsmJump(
            Opcode.BLT,
            Condition.LT,
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2),
            Objects.requireNonNull(targetLabel)
        );
    }

    public static AsmJump createGT(Label targetLabel, IntRegister source1, IntRegister source2) {
        return new AsmJump(
            Opcode.BGT,
            Condition.GT,
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2),
            Objects.requireNonNull(targetLabel)
        );
    }

    public static AsmJump createLE(Label targetLabel, IntRegister source1, IntRegister source2) {
        return new AsmJump(
            Opcode.BLE,
            Condition.LE,
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2),
            Objects.requireNonNull(targetLabel)
        );
    }

    public static AsmJump createGE(Label targetLabel, IntRegister source1, IntRegister source2) {
        return new AsmJump(
            Opcode.BGE,
            Condition.GE,
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2),
            Objects.requireNonNull(targetLabel)
        );
    }

    public static AsmJump createEQ(Label targetLabel, IntRegister source1, IntRegister source2) {
        return new AsmJump(
            Opcode.BEQ,
            Condition.EQ,
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2),
            Objects.requireNonNull(targetLabel)
        );
    }

    public static AsmJump createNE(Label targetLabel, IntRegister source1, IntRegister source2) {
        return new AsmJump(
            Opcode.BNE,
            Condition.NE,
            Objects.requireNonNull(source1),
            Objects.requireNonNull(source2),
            Objects.requireNonNull(targetLabel)
        );
    }

    public static AsmJump createUnary(Condition condition, Label targetLabel, IntRegister source) {
        return switch (condition) {
            case LTZ -> createLTZ(targetLabel, source);
            case GTZ -> createGTZ(targetLabel, source);
            case LEZ -> createLEZ(targetLabel, source);
            case GEZ -> createGEZ(targetLabel, source);
            case EQZ -> createEQZ(targetLabel, source);
            case NEZ -> createNEZ(targetLabel, source);
            default -> throw new IllegalArgumentException();
        };
    }

    public static AsmJump createBinary(Condition condition, Label targetLabel, IntRegister source1, IntRegister source2) {
        return switch (condition) {
            case LT -> createLT(targetLabel, source1, source2);
            case GT -> createGT(targetLabel, source1, source2);
            case LE -> createLE(targetLabel, source1, source2);
            case GE -> createGE(targetLabel, source1, source2);
            case EQ -> createEQ(targetLabel, source1, source2);
            case NE -> createNE(targetLabel, source1, source2);
            default -> throw new IllegalArgumentException();
        };
    }
}
