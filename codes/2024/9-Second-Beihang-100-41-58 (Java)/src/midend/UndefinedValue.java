package midend;

import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.Type;

public class UndefinedValue extends Value{
    public UndefinedValue(Type type) {
        super(type, "0");
    }

    public String getName() {
        if (getType() == IntegerType.i32) {
            return "0";
        } else if (getType() == FloatType.f32){
            return String.format("0x%x", Double.doubleToRawLongBits(0));
        }
        return null;
    }

    public String toString() {
        return "undefined";
    }
}
