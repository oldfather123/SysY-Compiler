package midend.LLVMType;

public class UndefinedType extends Type {
    public UndefinedType() {
        super(Name.UNDEFINED);
    }

    public static UndefinedType undefined = new UndefinedType();

    public String toString() {
        return "undefined";
    }
}
