package ir.type;

import java.util.ArrayList;
import java.util.List;

public class ArrayType implements IrType {
    private final IrType base;
    private final int length;

    public ArrayType(IrType base, int length) {
        this.base = base;
        this.length = length;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ArrayType) {
            return base.equals(((ArrayType) obj).base) && length == ((ArrayType) obj).length;
        }
        return false;
    }

    public static String getDimensionString(List<Integer> dimension, String inner) {
        StringBuilder sb = new StringBuilder();
        if (dimension.isEmpty()) {
            sb.append(inner);
        } else {
            sb.append("[");
            sb.append(dimension.get(0)).append(" x ");
            sb.append(getDimensionString(dimension.subList(1, dimension.size()), inner));
            sb.append("]");
        }
        return sb.toString();
    }

    @Override
    public String toString() {
        return getDimensionString(getDimension(), getFinalBase().toString());
    }

    @Override
    public boolean is(String tyName) {
        return tyName.equals("ARRAY") || tyName.equals(this.toString());
    }

    @Override
    public int sizeof() {
        return length * base.sizeof();
    }

    public boolean baseIs(String tyName) {
        return base.is(tyName);
    }

    public List<Integer> getDimension() {
        List<Integer> dimension = new ArrayList<>();
        dimension.add(length);
        if (base instanceof ArrayType) {
            dimension.addAll(((ArrayType) base).getDimension());
        }
        return dimension;
    }

    public IrType getFinalBase() {
        if (base instanceof ArrayType) {
            return ((ArrayType) base).getFinalBase();
        }
        return base;
    }

    public IrType getBase() {
        return base;
    }

    public int getLength() {
        return length;
    }

    @Override
    public int hashCode() {
        return base.hashCode() ^ length;
    }
}
