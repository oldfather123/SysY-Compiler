package backend.risc_v.entity.instrcution;

import backend.risc_v.entity.register.Register;

public class ITypeInstruction extends RiscVInstruction {
    public enum ITypeOperation {
        ADDI("addi"),
        LW("lw"),
        LD("ld"),
        FLW("flw"),
        XORI("xori");

        private final String name;

        ITypeOperation(String name) {
            this.name = name;
        }

        public RTypeInstruction.RTypeOperation toRTypeOperation() {
            return switch (this) {
                case ADDI -> RTypeInstruction.RTypeOperation.ADD;
                case XORI -> RTypeInstruction.RTypeOperation.XOR;
                default -> null;
            };
        }

        @Override
        public String toString() {
            return name;
        }
    }

    private final ITypeOperation operation;
    private final Register rd;
    private final Register rs1;
    private final int immediate;

    public ITypeInstruction(ITypeOperation operation, Register rd, Register rs1, int immediate) {
        this.operation = operation;
        this.rd = rd;
        this.rs1 = rs1;
        this.immediate = immediate;
    }

    @Override
    public String toString() {
        if (operation == ITypeOperation.LW || operation == ITypeOperation.FLW || operation == ITypeOperation.LD)
            return operation + " " + rd + ", " + immediate + "(" + rs1 + ")";
        else
            return operation + " " + rd + ", " + rs1 + ", " + immediate;
    }
}
