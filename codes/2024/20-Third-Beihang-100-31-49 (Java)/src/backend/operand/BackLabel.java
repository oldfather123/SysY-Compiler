package backend.operand;

public class BackLabel extends BackOperand {
    private String name;

    public BackLabel(String name) {
        this.name = name;
    }

    @Override
    public String toString() {
        return name;
    }
}
