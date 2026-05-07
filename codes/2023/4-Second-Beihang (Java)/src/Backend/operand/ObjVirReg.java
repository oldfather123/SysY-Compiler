package Backend.operand;

public class ObjVirReg extends ObjReg {
    private static int indexCounter = 0;
    private String name;
    public ObjVirReg() {
        this.name =  "vr" + indexCounter++;
    }

    @Override
    public String toString() {
        return name;
    }
}
