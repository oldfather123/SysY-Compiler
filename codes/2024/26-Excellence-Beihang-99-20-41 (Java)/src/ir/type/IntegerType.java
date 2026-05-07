package ir.type;

public class IntegerType implements IrType {
    private final int bits;

    public IntegerType(int bits) {
        this.bits = bits;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof IntegerType) {
            return bits == ((IntegerType) obj).bits;
        }
        return false;
    }

    public int getBits() {
        return bits;
    }

    @Override
    public String toString() {
        return "i" + bits;
    }

    @Override
    public boolean is(String tyName) {
        return tyName.equals("INT") || tyName.equals(this.toString());
    }

    @Override
    public int sizeof() {
        return (bits + 7) / 8;
    }

    @Override
    public int hashCode() {
        return bits * 17;
    }
}
