package cn.edu.bit.newnewcc.ir.value;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;

public abstract class Constant extends Value {

    /**
     * @param type 常量的类型
     */
    protected Constant(Type type) {
        super(type);
    }

    /**
     * 判断该常量在内存中的表示是否全为0
     *
     * @return 若该常量在内存中使用全0表示，返回true；反之返回false
     */
    abstract public boolean isFilledWithZero();

    @Override
    public String getValueNameIR() {
        return getValueName();
    }

    @Override
    public void setValueName(String valueName) {
        // 常量拥有固定的名字，不能被外界设置
        throw new UnsupportedOperationException();
    }
}
