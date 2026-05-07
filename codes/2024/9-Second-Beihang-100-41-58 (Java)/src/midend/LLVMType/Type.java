package midend.LLVMType;

public abstract class Type {
    private final Name name;

    public Type(Name name) {
        this.name = name;
    }

    public enum Name {
        INT,
        FLOAT,
        PTR,
        ARRAY,
        FUNC,
        VOID,
        UNDEFINED
    }

    public boolean isVoid() {
        return isType(Name.VOID);
    }

    public boolean isInteger() {
        return isType(Name.INT);
    }

    public boolean isFloat() {
        return isType(Name.FLOAT);
    }

    public boolean isArray() {
        return isType(Name.ARRAY);
    }

    public boolean isPointer() {
        return isType(Name.PTR);
    }

    public boolean isFunction() {
        return isType(Name.FUNC);
    }

    private boolean isType(Name type) {
        return name.equals(type);
    }
}


