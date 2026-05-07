package backend.risc_v.entity.instrcution;

import backend.risc_v.entity.register.Register;

public class STypeInstruction extends RiscVInstruction {
    public enum STypeOperation {
        SW("sw"),
        SD("sd"),
        FSW("fsw");

        private final String name;

        STypeOperation(String name) {
            this.name = name;
        }

        @Override
        public String toString() {
            return name;
        }
    }

    private final STypeOperation operation;
    private final Register rs1;
    private final Register rs2;
    private final int immediate;

    public STypeInstruction(STypeOperation operation, Register rs1, Register rs2, int immediate) {
        this.operation = operation;
        this.rs1 = rs1;
        this.rs2 = rs2;
        this.immediate = immediate;
    }

    @Override
    public String toString() {
        return operation + " " + rs2 + ", " + immediate + "(" + rs1 + ")";
    }
}

