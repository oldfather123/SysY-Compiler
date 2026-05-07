package Backend.operand;

public class FVirtualReg extends AsmReg {
    private final String name;

    public FVirtualReg() {
        this.name = "float" + AllPhysicalReg.counterForFPhy++;
    }

    @Override
    public String toString() {
        return name;
    }
}
