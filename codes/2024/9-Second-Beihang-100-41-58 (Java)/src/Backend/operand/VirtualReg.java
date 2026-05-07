package Backend.operand;

public class VirtualReg extends AsmReg {
    private final String name;

    public VirtualReg() {
        this.name = "int" + AllPhysicalReg.counterForPhy++;
    }

    @Override
    public String toString() {
        return name;
    }

}
