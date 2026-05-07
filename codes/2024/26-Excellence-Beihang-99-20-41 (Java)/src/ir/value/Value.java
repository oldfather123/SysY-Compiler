package ir.value;

import ir.type.IrType;

public class Value {
    private String name;
    private final IrType type;

    public Value(String name, IrType type) {
        this.name = name;
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public IrType getType() {
        return type;
    }

    @Override
    public String toString() {
        return type + " " + name;
    }

    public IrValue into() {
        return IrValue.from(this);
    }

    @Override
    public int hashCode() {
        return name.hashCode()*31 + type.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof Value value) {
            return name.equals(value.name) && type.equals(value.type);
        }
        return false;
    }

    protected void rename(String name) {
        this.name = name;
    }
}
