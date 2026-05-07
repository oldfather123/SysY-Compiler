package backend.operand;

public class ObjFVirReg extends ObjReg {
    public static int fVirRegIndex = 0;
    private int index;

    public ObjFVirReg() {
        super("vrf" + fVirRegIndex);
        index = fVirRegIndex++;
    }

    public int getIndex() {
        return index;
    }
}
