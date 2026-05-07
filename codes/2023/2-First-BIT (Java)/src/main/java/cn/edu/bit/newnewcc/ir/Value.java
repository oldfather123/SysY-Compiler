package cn.edu.bit.newnewcc.ir;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.NoSuchElementException;

/**
 * Value是一种具有类型、名字，并且可以被作为Operand使用的东西。
 * <p>
 * 例如：
 * <ul>
 *     <li>BasicBlock是一个Value，因为其可以作为跳转指令的操作数</li>
 *     <li>Function是一个Value，因为其可以作为Call指令的操作数</li>
 *     <li>Instruction是一个Value，因为表达式通常都有一个结果值，并可以被用作操作数</li>
 *     <li>Constant是一个Value，原因显然</li>
 * </ul>
 */
// 理论上来说这应该是一个interface，因为它的子类和它并没有太强的联系
// 但这个类型需要存储一些必要的信息，而Java并不支持多继承，因而采取了这种写法
public abstract class Value {
    private final Type type;

    protected Value(Type type) {
        this.type = type;
    }

    /**
     * @return 当前value的类型
     */
    public Type getType() {
        return type;
    }

    public String getTypeName() {
        return type.getTypeName_();
    }

    /**
     * 获取值的名字
     * <p>
     * 此名字不可直接用作ir的名字，因为其不包含 '@','%' 等标记
     *
     * @return 值的名字
     */
    public abstract String getValueName();

    /**
     * 获取值在IR中的名字
     * <p>
     * 此名词通常带有 '@','%' 等前缀标记；对于常量，其返回值与getValueName相同
     *
     * @return 值在IR中的名字
     */
    public abstract String getValueNameIR();

    /**
     * 设置值的名字
     * <p>
     * 不要设置纯数字的名字，因为这可能会与自动生成的名字相冲突
     * <p>
     * 当此函数未被调用时，getValueName将自动生成一个数字名
     *
     * @param valueName 值的名字
     */
    public abstract void setValueName(String valueName);

    /// 使用-被使用关系

    /**
     * 记录当前Value被哪些Operand所使用
     */
    private final HashSet<Operand> usages = new HashSet<>();

    /**
     * 添加一个当前Value被作为操作数的记录
     * <p>
     * 不要在Operand类以外的任何地方调用该函数！
     *
     * @param operand 操作数
     */
    public void __addUsage__(Operand operand) {
        usages.add(operand);
    }

    /**
     * 移除一个当前Value被作为操作数的记录
     * <p>
     * 不要在Operand类以外的任何地方调用该函数！
     *
     * @param operand 操作数
     */
    public void __removeUsage__(Operand operand) {
        boolean success = usages.remove(operand);
        if (!success) {
            throw new NoSuchElementException("Removing a non-existent usage");
        }
    }

    /**
     * @return 使用当前Value的Operand的列表的副本
     */
    public ArrayList<Operand> getUsages() {
        return new ArrayList<>(usages);
    }

    /**
     * 将该值的所有用法替换为传入值
     *
     * @param value 替代当前值的新值
     */
    public void replaceAllUsageTo(Value value) {
        for (Operand operand : getUsages()) {
            operand.setValue(value);
        }
    }

}
