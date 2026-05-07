package entities;

import util.IntHandler;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

public class Val {

    public static final Val ZERO_F = new Val(false, true, "0");
    private final boolean isInt, isConst, isLabel;
    private final String name;
    private final ArrayList<Val> args = new ArrayList<>();

    public Val(String name, boolean isLabel) {
        this.name = name;
        this.isInt = true;
        this.isConst = false;
        this.isLabel = isLabel;
    }

    public Val(boolean isInt, boolean isConst, String name) {
        this.name = name;
        this.isInt = isInt;
        this.isConst = isConst;
        this.isLabel = false;
    }

    public Val addArgs(ArrayList<Val> args) {
        this.args.addAll(args);
        return this;
    }

    public static Val fromVariable(Variable variable) {
        return new Val(variable.isInteger(), false, variable.name());
    }

    public static Val withoutArgs(Val val) {
        return new Val(val.isInt, val.isConst, val.name);
    }

    public static Val fromVal(Val val) {
        return withoutArgs(val).addArgs(val.args);
    }
    public static Val constVal(Word type, FOI foi) {
        return type.is(Word.Type.INT) ? intConst(foi.getInt()) : floatConst(foi.getFloat());
    }

    public static Val constVal(FOI foi) {
        return foi.isInteger() ? intConst(foi.getInt()) : floatConst(foi.getFloat());
    }

    public static Val intConst(int number) {
        return new Val(true, true, String.valueOf(number));
    }

    public static Val floatConst(float number) {
        return new Val(false, true, String.valueOf(number));
    }

    public Val offset(List<Integer> offsets) {
        ArrayList<Val> args = new ArrayList<>();
        for (Integer offset : offsets) {
            args.add(Val.intConst(offset));
        }
        return addArgs(args);
    }

    public static Val string(String string) {
        return new Val(string, true);
    }

    public static Val label(String pre) {
        return new Val(pre + '_' + IntHandler.get(), true);
    }

    public String getName() {
        return name;
    }

    public boolean isIntConst() {
        return isConst && isInt;
    }

    public boolean isFloatConst() {
        return isConst && !isInt;
    }

    public boolean isConst() {
        return isConst;
    }

    public boolean isInt() {
        return isInt;
    }

    public boolean isFloat() {
        return !isInt;
    }

    public boolean isLabel() {
        return isLabel;
    }

    public ArrayList<Val> getArgs() {
        return args;
    }

    public FOI getConstValue() {
        if (isInt) {
            return new FOI(Integer.parseInt(name));
        }
        return new FOI(Float.parseFloat(name));
    }

    public void updateConstArgs(Map<String, FOI> constValues) {
        for (int i = 0, size = args.size(); i < size; ++i) {
            String name = args.get(i).getName();
            if (constValues.containsKey(name)) {
                args.set(i, Val.constVal(constValues.get(name)));
            }
            args.get(i).updateConstArgs(constValues);
        }
    }

    @Override
    public String toString() {
        return '{' + (isInt ? "int" : "float") + (isConst ? ", const" : "") +
                ", " + name + ", args=" + Arrays.toString(args.toArray()) + '}';
    }
}
