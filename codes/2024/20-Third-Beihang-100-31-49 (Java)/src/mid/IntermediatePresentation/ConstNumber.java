package mid.IntermediatePresentation;

public class ConstNumber extends Value {

    public ConstNumber(Number val) {
        super(val.toString(), (val instanceof Float || val instanceof Double) ? ValueType.FLT : ValueType.I32);
        if (val instanceof Float || val instanceof Double) {
            reg = String.format("0x%x", Double.doubleToRawLongBits(Float.parseFloat(reg)));
        }
    }

    public Number getVal() {
        if (!isIntString()) {
            try {
                return Float.parseFloat(reg);
            } catch (Exception e) {
                return Double.longBitsToDouble(Long.parseUnsignedLong(reg.substring(2), 16));
            }
        } else {
            return Integer.parseInt(reg, 10);
        }
    }

    private boolean isIntString() {
        try {
            Integer.parseInt(reg, 10);
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    public boolean equals(Object o) {
        if (!(o instanceof ConstNumber)) {
            return false;
        }
        return getVal().floatValue() == ((ConstNumber) o).getVal().floatValue();
    }
}
