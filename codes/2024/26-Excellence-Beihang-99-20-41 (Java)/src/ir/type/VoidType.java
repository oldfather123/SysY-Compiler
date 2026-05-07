package ir.type;

public class VoidType implements IrType {
    @Override
    public boolean equals(Object obj) {
        return obj instanceof VoidType;
    }

    @Override
    public String toString() {
        return "void";
    }

    @Override
    public boolean is(String tyName) {
        return tyName.equals("VOID") || tyName.equals(this.toString());
    }

    @Override
    public int sizeof() {
        return 0;
    }

    @Override
    public int hashCode() {
        return 0;
    }
}
