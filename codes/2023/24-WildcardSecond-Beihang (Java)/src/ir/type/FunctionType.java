package ir.type;

public class FunctionType extends Type{

    public FunctionType() {
        super(TypeName.FUNCTION);
    }

    @Override
    public int getSpace() {
        return 0;
    }

    @Override
    public int getOffset() {
        return 0;
    }

    @Override
    public String toString() {
        return null;
    }
}
