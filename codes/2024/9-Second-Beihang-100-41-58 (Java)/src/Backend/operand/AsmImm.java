package Backend.operand;

public class AsmImm extends AsmOperand {
    private final int immediate;
    private final boolean is12;

    public AsmImm(int immediate, boolean is12) {
        this.immediate = immediate;
        this.is12 = is12;
    }

    public int getImmediate() {
        return immediate;
    }

    @Override
    public String toString() {
        return String.valueOf(immediate);
    }

    public boolean Is12() {
        return is12;
    }
}
