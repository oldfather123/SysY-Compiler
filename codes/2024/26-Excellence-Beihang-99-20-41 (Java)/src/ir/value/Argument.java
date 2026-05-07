package ir.value;

import ir.type.IrType;

public class Argument extends Value {
    public Argument(String name, IrType type) {
        super(name, type);
    }

    public Argument(Value value) {
        super(value.getName(), value.getType());
    }
}
