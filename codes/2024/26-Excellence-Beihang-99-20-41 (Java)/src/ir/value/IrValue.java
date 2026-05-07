package ir.value;

import ir.type.IrType;

/**
 * 尝试解决 IR 中现有 Value 过于复杂的问题，合并了 Constant，Variable，Intermediate 和 Literal
 */
public class IrValue extends Value {
    // 数字字面量，类型可以通过 getType() 得到
    public boolean isLiteral = false;
    public int intLiteral = 0;
    public float floatLiteral = 0f;
    // 是否是全局的
    public boolean isGlobal = false;
    // 是否是中间量，中间量均为 '%' 加数字表示
    public boolean isTemporary = false;

    private IrValue(Value value) {
        super(value.getName(), value.getType());
        if (value instanceof Literal literal) {
            isLiteral = true;
            if (literal.type.is("INT")) {
                intLiteral = literal.asInt();
            } else {
                floatLiteral = literal.asFloat();
            }
            return;
        }
        String name = value.getName();
        if (name.startsWith("@")) {
            isGlobal = true;
        } else {
            if (value instanceof Intermediate) {
                isTemporary = true;
            }
        }
    }

    public static IrValue from(Value value) {
        return new IrValue(value);
    }

    @Override
    public String getName() {
        return super.getName();
    }

    @Override
    public IrType getType() {
        return super.getType();
    }
}
