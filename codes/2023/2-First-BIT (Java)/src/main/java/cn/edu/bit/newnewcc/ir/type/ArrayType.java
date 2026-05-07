package cn.edu.bit.newnewcc.ir.type;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.value.constant.ConstArray;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

/**
 * 数组类型
 * <p>
 * 包含两个参数：长度、基类型
 * <p>
 * 若要定义多维数组，需使用“数组的数组”的方式实现
 */
public class ArrayType extends Type {
    /**
     * 数组的长度
     * <p>
     * 即：数组内包含length个baseType类型的元素
     */
    private final int length;
    /**
     * 基类型
     * <p>
     * 即每个数组内单个元素的类型
     * <p>
     * 若基类型也是数组类型，则构成一个多维数组
     */
    private final Type baseType;

    /**
     * @param length   数组长度
     * @param baseType 数组内元素的类型
     */
    private ArrayType(int length, Type baseType) {
        this.length = length;
        this.baseType = baseType;
    }

    /**
     * 获取数组长度
     * <p>
     * 若类型是多维数组，返回最高维的长度
     *
     * @return 数组长度
     */
    public int getLength() {
        return length;
    }

    /**
     * 获取基类型
     * <p>
     * 即数组解引用一次后的类型
     *
     * @return 基类型
     */
    public Type getBaseType() {
        return baseType;
    }

    /**
     * 获取该类型扁平化后的类型 <br>
     * 即将多维数组压缩成一维数组后的类型 <br>
     *
     * @return 扁平化后的类型
     */
    public ArrayType getFlattenedType() {
        int flattenedLength = 1;
        Type flattenedType = this;
        while (flattenedType instanceof ArrayType arrayType) {
            flattenedLength *= arrayType.length;
            flattenedType = arrayType.baseType;
        }
        return ArrayType.getInstance(flattenedLength, flattenedType);
    }

    private ConstArray defaultInitialization;

    @Override
    public ConstArray getZeroInitialization() {
        if (defaultInitialization == null) {
            defaultInitialization = new ConstArray(baseType, length, new ArrayList<>());
        }
        return defaultInitialization;
    }

    @Override
    protected String getTypeName_() {
        return String.format("[%d x %s]", length, baseType.getTypeName());
    }

    @Override
    public long getSize() {
        return baseType.getSize() * length;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        ArrayType arrayType = (ArrayType) o;
        return length == arrayType.length && baseType == arrayType.baseType;
    }

    @Override
    public int hashCode() {
        return Objects.hash(length, baseType);
    }

    private static class Cache {
        private static class Key {
            int length;
            Type baseType;

            public Key(int length, Type baseType) {
                this.length = length;
                this.baseType = baseType;
            }

            @Override
            public boolean equals(Object o) {
                if (this == o) return true;
                if (o == null || getClass() != o.getClass()) return false;
                Key key = (Key) o;
                return length == key.length && Objects.equals(baseType, key.baseType);
            }

            @Override
            public int hashCode() {
                return Objects.hash(length, baseType);
            }

        }

        private static final Map<Key, ArrayType> cache = new HashMap<>();
    }

    /**
     * 获取数组类型的实例
     * <p>
     * 数组类型是单例
     *
     * @param length   数组长度
     * @param baseType 数组的基类型
     * @return 数组类型
     */
    public static ArrayType getInstance(int length, Type baseType) {
        Cache.Key key = new Cache.Key(length, baseType);

        if (!Cache.cache.containsKey(key))
            Cache.cache.put(key, new ArrayType(length, baseType));

        return Cache.cache.get(key);
    }
}
