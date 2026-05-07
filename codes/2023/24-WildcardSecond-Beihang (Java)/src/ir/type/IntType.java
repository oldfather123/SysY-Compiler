package ir.type;

public class IntType extends Type{
    private int len = 32;

    public IntType(){
        super(TypeName.I);
    }

    public IntType(int i) {
        super(TypeName.I);
        len = i;
    }

    @Override
    public String toString() {return "i" + len;}

    @Override
    public int getSpace() {
        return len / 8;
    }

    @Override
    public int getOffset() {
        return len / 8;
    }

    public int getLen() {
        return len;
    }
}
