package midend;

import midend.LLVMType.Type;

public class Constant extends Value {

    public Constant(Type type, String name) {
        super(type, name);
    }
}
