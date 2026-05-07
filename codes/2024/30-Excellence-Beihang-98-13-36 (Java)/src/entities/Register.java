package entities;

import java.util.HashMap;
import java.util.Map;

public class Register {

    private final int num;
    private int delta;
    private final Type type;
    private final String label;
    private final boolean isAddress;

    private Register(Type type) {
        this(0, type);
    }

    private Register(int num, Type type) {
        this(num, type, null, false);
    }

    private Register(String label) {
        this(0, Type.LABEL, label, false);
    }

    public Register(int num, Type type, String label, boolean isAddress) {
        this.num = num;
        this.type = type;
        this.label = label;
        this.isAddress = isAddress;
        this.delta = 0;
    }

    public static boolean equal(Register r1, Register r2) {
        if (r1 == r2) return true;
        if (r1.isAddress || r2.isAddress || r1.type == Type.LABEL || r2.type == Type.LABEL) return false;
        return r1.num == r2.num && r1.type == r2.type;
    }

    public static boolean equalNotStrict(Register r1, Register r2) {
        if (r1 == r2) return true;
        if (r1.type == Type.LABEL || r2.type == Type.LABEL) return false;
        return r1.num == r2.num && r1.type == r2.type;
    }

    public Register copy() {
        return new Register(num, type, label, isAddress);
    }

    public Register addr() {
        return new Register(num, type, label, true);
    }

    public boolean isAddress() {
        return isAddress;
    }

    public String getLabel() {
        return label;
    }

    public int getNum() {
        return num;
    }

    public Register setDelta(int num) {
        this.delta = num;
        return this;
    }

    public boolean is(Type type) {
        return this.type == type;
    }

    protected static Register t(int num) {
        return new Register(num, Type.T);
    }

    protected static Register s(int num) {
        return new Register(num, Type.S);
    }

    protected static Register ft(int num) {
        return new Register(num, Type.FT);
    }

    protected static Register fs(int num) {
        return new Register(num, Type.FS);
    }

    protected static Register a(int num) {
        return new Register(num, Type.A);
    }

    protected static Register fa(int num) {
        return new Register(num, Type.FA);
    }

    protected static Register sp() {
        return new Register(Type.SP);
    }

    protected static Register ra() {
        return new Register(Type.RA);
    }

    public static Register tr(int num) {
        return new Register(num, Type.TEMP_R);
    }

    public static Register tf(int num) {
        return new Register(num, Type.TEMP_F);
    }

    public static Register td(int num) {
        return new Register(num, Type.TEMP_D);
    }

    public static Register num(int num) {
        if (!NUM_REGISTER.containsKey(num)) {
            NUM_REGISTER.put(num, new Register(num, Type.NUM));
        }
        return NUM_REGISTER.get(num);
    }

    public static Register label(String label) {
        return new Register(label);
    }

    public static Register parse(String name) {
        if (name.startsWith("t")) {
            return t(Integer.parseInt(name.substring(1)));
        } else if (name.startsWith("s")) {
            return s(Integer.parseInt(name.substring(1)));
        } else if (name.startsWith("ft")) {
            return ft(Integer.parseInt(name.substring(2)));
        } else if (name.startsWith("fs")) {
            return fs(Integer.parseInt(name.substring(2)));
        }
        return label("ERROR");
    }

    private static final Map<Integer, Register> NUM_REGISTER = new HashMap<>();

    public enum Type {
        T, S, FT, FS, TEMP_R, TEMP_F, TEMP_D, NUM, LABEL, SP, RA, A, FA,
    }

    @Override
    public String toString() {
        String row = switch (type) {
            case T -> "t" + num;
            case S -> "s" + num;
            case FT -> "ft" + num;
            case FS -> "fs" + num;
            case NUM -> String.valueOf(num);
            case LABEL -> label;
            case SP -> "sp";
            case RA -> "ra";
            case A -> "a" + num;
            case FA -> "fa" + num;
            default -> "";
        };
        if (isAddress) {
            if (delta == 0) {
                return "(" + row + ")";
            }
            return delta + "(" + row + ")";
        }
        return row;
    }
}
