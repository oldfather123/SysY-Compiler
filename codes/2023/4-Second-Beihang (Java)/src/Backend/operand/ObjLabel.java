package Backend.operand;

public class ObjLabel extends ObjOperand {
    private String name;
    public ObjLabel(String name) {
        this.name = name;
    }

    @Override
    public String toString() {
        return name;
    }
}
