package backend.risc_v.entity.instrcution;

import backend.risc_v.entity.register.Register;

public class BTypeInstruction extends RiscVInstruction {
    public enum BTypeOperation {
        BEQ("beq"),
        BNE("bne"),
        BLT("blt"),
        BGE("bge"),
        BLTU("bltu"),
        BGEU("bgeu");

        private final String name;

        BTypeOperation(String name) {
            this.name = name;
        }

        @Override
        public String toString() {
            return name;
        }
    }

    private final BTypeOperation operation;
    private final Register rs1;
    private final Register rs2;
    private final String label;

    public BTypeInstruction(BTypeOperation operation, Register rs1, Register rs2, String label) {
        this.operation = operation;
        this.rs1 = rs1;
        this.rs2 = rs2;
        this.label = label;
    }

    @Override
    public String toString() {
        return operation + " " + rs1 + ", " + rs2 + ", " + label;
    }
}

