package frontend.ir.constant;

import frontend.ir.Value;
import frontend.ir.objecttype.Type;

public abstract class IRConst extends Value {
    protected IRConst(Type type) {
        super(type);
    }

    public abstract Number getConstVal();

    public abstract IRConst clone();
}
