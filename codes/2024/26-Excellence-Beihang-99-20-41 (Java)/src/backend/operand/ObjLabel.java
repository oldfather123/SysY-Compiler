package backend.operand;

public class ObjLabel extends ObjOperand {
    private String name;

    public ObjLabel(String name) {
        super(name);
        this.name = name;
    }

    public String getName() {
        return this.name;
    }
}
