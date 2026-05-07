package cn.edu.bit.newnewcc.ir.value;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.FunctionType;

import java.util.List;

public abstract class BaseFunction extends Value {
    public BaseFunction(FunctionType type) {
        super(type);
    }

    /**
     * @return 返回值类型
     */
    public Type getReturnType() {
        return getType().getReturnType();
    }

    /**
     * @return 形参类型列表（只读）
     */
    public List<Type> getParameterTypes() {
        // FunctionType.getParameterTypes()中已经包装了unmodifiableList，故此处无需再次包装
        return getType().getParameterTypes();
    }

    @Override
    public FunctionType getType() {
        return (FunctionType) super.getType();
    }
}
