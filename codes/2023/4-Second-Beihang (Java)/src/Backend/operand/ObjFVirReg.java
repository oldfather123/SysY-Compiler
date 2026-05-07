package Backend.operand;

public class ObjFVirReg extends ObjReg {
    private static int indexCounter = 0;
    private String name;
    public ObjFVirReg() {
        this.name =  "vrf" + indexCounter++;
    }

    @Override
    public String toString() {
        return name;
    }
}
