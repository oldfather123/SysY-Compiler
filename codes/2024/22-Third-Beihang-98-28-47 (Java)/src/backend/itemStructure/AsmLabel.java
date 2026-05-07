package backend.itemStructure;

public class AsmLabel extends AsmOperand {
    private String name;

    public AsmLabel(String name) {
        this.name = name;
    }
    public String getName() {
        return name;
    }
    public String toString() {
        return name;
    }
}
