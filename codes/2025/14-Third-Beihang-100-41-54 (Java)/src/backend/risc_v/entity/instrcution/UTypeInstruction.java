package backend.risc_v.entity.instrcution;

import backend.risc_v.entity.register.Register;

public class UTypeInstruction extends RiscVInstruction {
    public enum UTypeOperation {
        LUI("lui"),
        AUIPC("auipc");

        private final String name;

        UTypeOperation(String name) {
            this.name = name;
        }

        @Override
        public String toString() {
            return name;
        }
    }

    private final UTypeOperation operation;
    private final Register rd;
    private final int immediate;

    public UTypeInstruction(UTypeOperation operation, Register rd, int immediate) {
        this.operation = operation;
        this.rd = rd;
        this.immediate = immediate;
    }

    @Override
    public String toString() {
        return operation + " " + rd + ", " + immediate;
    }
}

