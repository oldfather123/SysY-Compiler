package midend;

import midend.LLVMType.Type;

public class Param extends Value {

    public Param(Type type) {
        super(type, "%reg" + Value.num++);
    }

    public String toString() {
        return getType().toString();
    }
}
