package ir.type;

public class VariLen extends Type{
    public VariLen() {
        super(TypeName.Vari);
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
        return "...";
    }
}
