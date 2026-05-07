package backend.risc_v.entity.instrcution;

public class JTypeInstruction extends RiscVInstruction {
    public enum JTypeOperation {
        J("j"),
        JAL("jal");

        private final String name;

        JTypeOperation(String name) {
            this.name = name;
        }

        @Override
        public String toString() {
            return name;
        }
    }

    private final JTypeOperation operation;
    private final String label;

    public JTypeInstruction(JTypeOperation operation, String label) {
        this.operation = operation;
        this.label = label;
    }


    @Override
    public String toString() {
        return operation + " " + label;
    }
}