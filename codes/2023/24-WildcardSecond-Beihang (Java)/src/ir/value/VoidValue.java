package ir.value;

import ir.Value;

public class VoidValue extends Value {
    @Override
    public boolean hasName() {
        return false;
    }

    @Override
    public String getNameWithType() {
        return "void";
    }
}
