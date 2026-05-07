package backend.operand;

public class ObjVirReg extends ObjReg {
    public static int virRegIndex = 0;
    private int index;

    public ObjVirReg() {
        super("vr" + virRegIndex);
        index = virRegIndex++;
    }

    public int getIndex() {
        return index;
    }

}
