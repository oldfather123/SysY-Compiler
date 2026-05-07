package frontend.ir.symbol;

import frontend.ir.Value;
import frontend.ir.constant.IRConst;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.derived.TPointer;

public class GlbObjPtr extends Value {
    private final String name;

    public GlbObjPtr(Type type, String name) {
        super(new TPointer(type));  // llvm ir 中，全局对象标识符实际上是其指针，因此类型也要做相应调整
        this.name = name;
    }

    @Override
    public String value2string() {
        return "@" + this.name;
    }
}
