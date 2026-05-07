package backend.operand;

public class ObjImm extends ObjOperand {
    private int immediate;

    public ObjImm(int immediate) {
        super(Integer.toString(immediate));
        this.immediate = immediate;
    }

    public int getImmediate() {
        return immediate;
    }
}
