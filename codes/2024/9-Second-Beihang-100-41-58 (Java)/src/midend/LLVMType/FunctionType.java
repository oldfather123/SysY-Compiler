package midend.LLVMType;

public class FunctionType extends Type {
    public FunctionType() {
        super(Name.FUNC);
    }

    public static FunctionType functionType = new FunctionType();

    public String toString() {
        return "function";
    }
}
