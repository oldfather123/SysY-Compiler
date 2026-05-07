package midend.LLVMType;

public class FloatType extends Type{
    private FloatType() {
        super(Name.FLOAT);
    }

    public static FloatType f32 = new FloatType();

    public String toString() {
        return "float";
    }
}
