package cn.edu.bit.newnewcc.ir;

import cn.edu.bit.newnewcc.ir.value.Constant;

/**
 * 值的类型
 * <p>
 * 所有类型都是单例，可以直接用==判断
 */
public abstract class Type {

    /**
     * 获取该类型零初始化的结果
     *
     * @return 零初始化值
     */
    public abstract Constant getZeroInitialization();

    /**
     * 获取类型的名字
     * <p>
     * 此方法至多被调用一次，因为其会缓存之前的结果
     *
     * @return 类型的名字
     */
    protected abstract String getTypeName_();

    private String typeNameCache;

    /**
     * 获取类型在 LLVM IR 中的标识符
     *
     * @return 类型名
     */
    public String getTypeName() {
        if (typeNameCache == null) {
            typeNameCache = getTypeName_();
        }
        return typeNameCache;
    }

    /**
     * 获取类型的实例占用内存的大小，以字节为单位
     *
     * @return 类型的实例占用内存的大小，以字节为单位
     */
    public abstract long getSize();

}
