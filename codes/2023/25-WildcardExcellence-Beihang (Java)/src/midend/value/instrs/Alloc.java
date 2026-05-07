package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;

public class Alloc extends Loc {
    public Alloc(Type type, BasicBlock parent) {
        super(parent);
        setType(new Type.PointerType(type));
    }

    public String toString() {
        return getName() + " = " +
                "alloca " +
                getType().getElementType().toString();
    }
}
