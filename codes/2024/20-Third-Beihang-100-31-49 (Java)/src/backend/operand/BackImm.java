package backend.operand;

public class BackImm extends BackOperand {
    protected int imm;
    public BackImm(int imm) {
        this.imm = imm;
    }

    public int getImm() {
        return imm;
    }

    @Override
    public String toString() {
        return String.valueOf(imm);
    }
}
