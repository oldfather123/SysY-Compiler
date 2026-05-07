package cn.edu.bit.newnewcc.ir.value;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.PointerType;

/**
 * 全局变量
 * <p>
 * 此类型本质上是指向全局变量的指针
 */
public class GlobalVariable extends Value {

    private boolean isConstant;
    private final Constant initialValue;

    /**
     * 未指定初始值的全局变量
     *
     * @param type 全局变量的类型
     */
    public GlobalVariable(Type type) {
        this(false, type.getZeroInitialization());
    }

    /**
     * 指定初始值的全局变量
     *
     * @param isConstant   该全局变量是否被定义为常量
     * @param initialValue 全局变量的初始值
     */
    public GlobalVariable(boolean isConstant, Constant initialValue) {
        super(PointerType.getInstance(initialValue.getType()));
        this.isConstant = isConstant;
        this.initialValue = initialValue;
    }

    /**
     * 判断该全局变量是否被<strong>定义</strong>为常量
     *
     * @return 若该全局变量被定义为常量，则返回true；否则返回false
     */
    public boolean isConstant() {
        return isConstant;
    }

    public void setIsConstant(boolean isConstant) {
        this.isConstant = isConstant;
    }

    /**
     * 获取全局变量的初始值
     *
     * @return 全局变量的初始值
     */
    public Constant getInitialValue() {
        return initialValue;
    }

    /**
     * 获取全局变量的值的类型
     * <p>
     * 此类型与全局变量的类型不同，全局变量的类型是该类型的指针
     *
     * @return 全局变量的值的类型
     */
    public Type getStoredValueType() {
        return getType().getBaseType();
    }

    /**
     * 注意：全局变量的类型是其存储的类型的<strong>指针</strong>
     * <p>
     * 要获取其存储的值的类型，请使用 globalVariable.getType().getPointedType() 或 globalVariable.getStoredValueType()
     *
     * @return 全局变量存储的类型的指针
     */
    @Override
    public PointerType getType() {
        return (PointerType) super.getType();
    }

    private String valueName = null;

    @Override
    public String getValueName() {
        return valueName;
    }

    @Override
    public String getValueNameIR() {
        return '@' + getValueName();
    }

    @Override
    public void setValueName(String valueName) {
        this.valueName = valueName;
    }

    public void emitIr(StringBuilder builder) {
        // e.g.
        // @c = dso_local constant i32 3
        // @d = dso_local global i32 4
        builder.append(String.format(
                "%s = dso_local %s %s %s\n",
                this.getValueNameIR(),
                this.isConstant() ? "constant" : "global",
                this.getStoredValueType().getTypeName(),
                this.getInitialValue().getValueNameIR()
        ));
    }

}
