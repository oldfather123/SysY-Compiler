package Backend.operand;

public class ObjImm12 extends ObjOperand {
    private int immediate;
    public ObjImm12(int immediate) {
        this.immediate = immediate;
    }

    public void setImmediate12(int immediate) {
        this.immediate = immediate;
    }
    public int getImmediate12() {
        return immediate;
    }
    @Override
    public String toString() {
        return String.valueOf(immediate);
    }
}
