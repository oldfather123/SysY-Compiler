package ir.type;

public class FloatType implements IrType {
    private final int bits;

    public FloatType() {
        this.bits = 32;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof FloatType) {
            return bits == ((FloatType) obj).bits;
        }
        return false;
    }

    public int getBits() {
        return bits;
    }

    @Override
    public String toString() {
        return "float";
    }

    @Override
    public boolean is(String tyName) {
        return tyName.equals("FLOAT") || tyName.equals(this.toString());
    }

    @Override
    public int sizeof() {
        return (bits + 7) / 8;
    }

    @Override
    public int hashCode() {
        return bits * 37;
    }
}
