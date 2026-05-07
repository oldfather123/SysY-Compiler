package cn.edu.bit.newnewcc.ir.type;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.value.Constant;

import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

/**
 * 指针类型
 */
public class PointerType extends Type {

    private final Type baseType;

    /**
     * @param baseType 被指向的类型
     */
    private PointerType(Type baseType) {
        this.baseType = baseType;
    }

    public Type getBaseType() {
        return baseType;
    }

    @Override
    protected String getTypeName_() {
        return baseType.getTypeName() + "*";
    }

    @Override
    public Constant getZeroInitialization() {
        throw new UnsupportedOperationException();
    }

    @Override
    public long getSize() {
        // 理论上应该返回指针的位数，但是SysY不涉及指针，所以这个方法应该不会被调用到
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        PointerType that = (PointerType) o;
        // 所有类型都是单例，这里直接用==比较地址，可以避免递归比较内部类型
        return baseType == that.baseType;
    }

    @Override
    public int hashCode() {
        return Objects.hash(baseType);
    }

    private static class Cache {
        private static final Map<Type, PointerType> cache = new HashMap<>();
    }

    public static PointerType getInstance(Type baseType) {
        if (!Cache.cache.containsKey(baseType))
            Cache.cache.put(baseType, new PointerType(baseType));

        return Cache.cache.get(baseType);
    }
}
