package backend.operand;

public class RiscvFloatConst extends RiscvLabel {
    private double val;

    public RiscvFloatConst(String name, double val) {
        super(name);
        this.val = val;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(super.toPrint());
        sb.append("\t.word\t" + Float.floatToIntBits((float) val) + "\n");
        return sb.toString();
    }
}
