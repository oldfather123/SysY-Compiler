package ir.type;

public class FloatType extends Type {

    public FloatType() {
        super(TypeName.F);
    }

    @Override
    public String toString() {
        return "float";
    }

    @Override
    public int getSpace() {
        return 4;
    }

    @Override
    public int getOffset() {
        return 4;
    }

}
