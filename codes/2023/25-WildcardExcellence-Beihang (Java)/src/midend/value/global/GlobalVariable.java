package midend.value.global;

import midend.MyModule;
import midend.Type;
import midend.value.Value;

public class GlobalVariable extends Value {
    private Boolean isConst;
    private Value initialValue;

    public Type getInitialType() {
        return initialType;
    }

    private Type initialType;

    public GlobalVariable(Type type, String name, Value value, boolean isConst, Type initialType) {
        //globalVariable的 type 也为 pointerType, name 为 %g
        //全局变量未初始化时初始化为 0
        super(type, name);
        setType(new Type.PointerType(type));
        setName("@global" + GLO_NUM++);
        this.isConst = isConst;
        this.initialValue = value;
        this.initialType = initialType;
        MyModule.myModule.addGlobalVariable(this);
    }

    public Value getInitialValue() {
        return this.initialValue;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(getName()).append(" = ");
        if (isConst) {
            sb.append("constant ");
        } else {
            sb.append("global ");
        }
        sb.append((getType()).getElementType()).append(" ");
        sb.append(this.initialValue.getName());
        return sb.toString();
    }
}
