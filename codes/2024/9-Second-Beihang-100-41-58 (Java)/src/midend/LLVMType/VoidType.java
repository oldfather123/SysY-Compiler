package midend.LLVMType;

public class VoidType extends Type{
    private VoidType() {
        super(Name.VOID);
    }

    public static VoidType voidType = new VoidType();

    public String toString() {
        return "void";
    }
 }
