package cn.edu.bit.newnewcc.ir.type;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.value.Constant;

import java.util.*;

/**
 * 函数类型
 */
public class FunctionType extends Type {
    private final Type returnType;
    private final List<Type> parameterTypes;

    /**
     * @param returnType     函数返回值的类型
     * @param parameterTypes 函数参数类型的列表
     */
    private FunctionType(Type returnType, List<Type> parameterTypes) {
        this.returnType = returnType;
        this.parameterTypes = new ArrayList<>(parameterTypes);
    }

    /**
     * @return 函数返回值类型
     */
    public Type getReturnType() {
        return returnType;
    }

    /**
     * @return 函数参数类型的列表，该列表是只读的
     */
    public List<Type> getParameterTypes() {
        return Collections.unmodifiableList(parameterTypes);
    }

    @Override
    protected String getTypeName_() {
        var builder = new StringBuilder();
        builder.append(returnType.getTypeName());
        builder.append('(');
        boolean needCommaBefore = false;
        for (var parameterType : parameterTypes) {
            if (needCommaBefore) {
                builder.append(',');
            }
            builder.append(parameterType.getTypeName());
            needCommaBefore = true;
        }
        builder.append(')');
        return builder.toString();
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
        FunctionType that = (FunctionType) o;
        return returnType.equals(that.returnType) && parameterTypes.equals(that.parameterTypes);
    }

    @Override
    public int hashCode() {
        return Objects.hash(returnType, parameterTypes);
    }

    private static class Cache {
        private static class Key {
            Type returnType;
            List<Type> parameterTypes;

            public Key(Type returnType, List<Type> parameterTypes) {
                this.returnType = returnType;
                this.parameterTypes = parameterTypes;
            }

            @Override
            public boolean equals(Object o) {
                if (this == o) return true;
                if (o == null || getClass() != o.getClass()) return false;
                Key key = (Key) o;
                return Objects.equals(returnType, key.returnType) && Objects.equals(parameterTypes, key.parameterTypes);
            }

            @Override
            public int hashCode() {
                return Objects.hash(returnType, parameterTypes);
            }

        }

        private static final Map<Key, FunctionType> cache = new HashMap<>();
    }

    public static FunctionType getInstance(Type returnType, List<Type> parameterTypes) {
        Cache.Key key = new Cache.Key(returnType, parameterTypes);

        if (!Cache.cache.containsKey(key))
            Cache.cache.put(key, new FunctionType(returnType, parameterTypes));

        return Cache.cache.get(key);
    }
}
