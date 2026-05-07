package ir.type;

public class PointerType implements IrType {
    private final IrType type;

    public PointerType(IrType type) {
        this.type = type;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof PointerType) {
            return type.equals(((PointerType) obj).type);
        }
        return false;
    }

    @Override
    public String toString() {
        return "ptr";
    }

    @Override
    public boolean is(String tyName) {
        return tyName.equals("POINTER") || tyName.equals(this.toString());
    }

    @Override
    public int sizeof() {
        return 8;
    }

    public boolean baseIs(String tyName) {
        return type.is(tyName);
    }

    public IrType getBaseType() {
        return type;
    }

    public IrType getFinalBase() {
        if (type instanceof ArrayType arrayType) {
            return arrayType.getFinalBase();
        }
        if (type instanceof PointerType pointerType) {
            // Maybe unreachable
            return pointerType.getFinalBase();
        }
        return type;
    }

    @Override
    public int hashCode() {
        return type.hashCode() * 13 + getBaseType().hashCode();
    }
}
