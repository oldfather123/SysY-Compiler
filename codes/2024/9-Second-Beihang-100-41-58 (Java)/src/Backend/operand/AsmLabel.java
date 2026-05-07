package Backend.operand;

public class AsmLabel extends AsmOperand {
    private final String name;

    public AsmLabel(String name) {
        this.name = name;
    }

    @Override
    public String toString() {
        return name;
    }
}