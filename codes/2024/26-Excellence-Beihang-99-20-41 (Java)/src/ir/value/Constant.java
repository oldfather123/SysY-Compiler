package ir.value;

import ir.type.ArrayType;
import ir.type.IrType;
import ir.type.Ty;
import utils.RefCell;

import java.util.List;
import java.util.TreeMap;

public class Constant extends Value {
    public final Object initialValue;
    private String mangling;

    public Constant(String name, Integer value) {
        super(name, Ty.I32);
        this.initialValue = value;
    }

    public Constant(String name, Boolean value) {
        super(name, Ty.I1);
        this.initialValue = value;
    }

    public Constant(String name, Float value) {
        super(name, Ty.F32);
        this.initialValue = value;
    }

    public Constant(String name, ArrayType type, List<Object> value) {
        super(name, type);
        this.initialValue = value;
    }

    public Constant(String name, IrType type, Object value) {
        super(name, type);
        this.initialValue = value;
    }

    public void mangle(String mangling) {
        this.mangling = mangling;
    }

    public void applyMangleToName() {
        super.rename(mangling);
    }

    public String getMangle() {
        return mangling;
    }

    public Object getValue() {
        return initialValue;
    }

    public String getDisplayValue() {
        if (initialValue instanceof Float) {
            return "0x" + Long.toHexString(Double.doubleToRawLongBits((Float) initialValue));
        }
        if (initialValue instanceof TreeMap<?, ?> list) {
            List<Integer> dimension = ((ArrayType) getType()).getDimension();
            return getArrayString(new RefCell<>(0), (TreeMap<Integer, Object>) list, dimension, ((ArrayType) getType()).getFinalBase().toString());
        }
        return initialValue.toString();
    }

    private static String getArrayString(RefCell<Integer> curIndex, TreeMap<Integer, Object> value, List<Integer> dimension, String inner) {
        if (value.isEmpty()) {
            // 理论上应该考虑这一层是不是全是0，但是这里就不考虑了，因为这个函数只会在输出 IR 的时候调用，不影响大局
            return "zeroinitializer";
        }
        StringBuilder sb = new StringBuilder();
        sb.append("[");
        for (int i = 0; i < dimension.get(0); i++) {
            if (dimension.size() == 1) {
                String val;
                if (inner.equals("float")) {
                    val = "0x" + Long.toHexString(Double.doubleToRawLongBits((Float) value.getOrDefault(curIndex.get(), 0f)));
                } else {
                    val = value.getOrDefault(curIndex.get(), "0").toString();
                }
                sb.append(inner).append(" ").append(val).append(", ");
                curIndex.set(curIndex.get() + 1);
            } else {
                sb.append(ArrayType.getDimensionString(dimension.subList(1, dimension.size()), inner)).append(" ");
                sb.append(getArrayString(curIndex, value, dimension.subList(1, dimension.size()), inner)).append(", ");
            }
        }
        sb.delete(sb.length() - 2, sb.length());
        sb.append("]");
        return sb.toString();
    }

    public String getVarLabel() {
        return super.toString();
    }

    @Override
    public String toString() {
        return "const " + super.toString() + " = " + initialValue;
    }
}
