package midend.LLVMType;

public class PointerType extends Type{
    private Type elementType;

    public PointerType(Type elementType) {
        super(Name.PTR);
        this.elementType = elementType;
    }

    public Type getElementType() {
        return elementType;
    }

    public String toString() {
        return elementType.toString() + "*";
    }
}
