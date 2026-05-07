package ir.value;

import ir.type.ArrayType;
import ir.type.IrType;

import java.util.List;

public class Variable extends Constant {
    boolean dirty = false;

    public Variable(String name, Integer initialValue) {
        super(name, initialValue);
    }

    public Variable(String name, Boolean initialValue) {
        super(name, initialValue);
    }

    public Variable(String name, Float initialValue) {
        super(name, initialValue);
    }

    public Variable(String name, ArrayType type, List<Object> initialValue) {
        super(name, type, initialValue);
    }

    public Variable(String name, IrType type, Object initialValue) {
        super(name, type, initialValue);
    }

    public void setDirty() {
        dirty = true;
    }

    public boolean isDirty() {
        return dirty;
    }

    public Object getInitialValue() {
        return super.getValue();
    }

    @Override
    public Object getValue() {
        if (dirty) {
            return null;
        }
        return super.getValue();
    }

    @Override
    public String toString() {
        return (dirty ? "[D] " : "") + "var " + getVarLabel() + " = " + getInitialValue();
    }
}
