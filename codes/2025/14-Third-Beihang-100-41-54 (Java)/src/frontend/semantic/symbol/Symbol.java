package frontend.semantic.symbol;

import backend.ir.entity.Value;

public class Symbol {
    protected final String name;
    protected final IRBasicType type;
    protected final Boolean isGlobal;
    protected final Boolean isConst;
    protected Value value;

    public Symbol(String name, IRBasicType type, Boolean isGlobal, Boolean isConst) {
        this.name = name;
        this.type = type;
        this.isGlobal = isGlobal;
        this.isConst = isConst;
    }

    public Boolean getGlobal() {
        return isGlobal;
    }

    public Boolean getConst() {
        return isConst;
    }

    public Value getValue() {
        return value;
    }

    public void setValue(Value value) {
        this.value = value;
    }

    public String getName() {
        return name;
    }

    public IRBasicType getType() {
        return type;
    }

    public Boolean isGlobal() {
        return isGlobal;
    }

    public Boolean isConst() {
        return isConst;
    }
}
