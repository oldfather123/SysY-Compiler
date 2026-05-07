package cn.edu.bit.newnewcc.ir.value.constant;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.exception.IndexOutOfBoundsException;
import cn.edu.bit.newnewcc.ir.type.ArrayType;
import cn.edu.bit.newnewcc.ir.value.Constant;

import java.util.List;

/**
 * 常量数组
 * <p>
 * 出于性能的考虑，此类型并非单例
 *
 * @see <a href="https://llvm.org/docs/LangRef.html#complex-constants">LLVM IR文档</a>
 */
public class ConstArray extends Constant {
    private final int length;
    private final List<Constant> valueList;

    /**
     * @param baseType        数组的基类。若想定义高维数组，请使用“数组的数组”
     * @param length          数组的实际长度
     * @param initializerList 数组已初始化的部分。未初始化部分将被初始化为0。
     */
    public ConstArray(Type baseType, int length, List<Constant> initializerList) {
        super(ArrayType.getInstance(length, baseType));
        if (initializerList.size() > length) {
            throw new IllegalArgumentException();
        }
        this.length = length;
        this.valueList = initializerList;
    }

    @Override
    public ArrayType getType() {
        return (ArrayType) super.getType();
    }

    /**
     * @return 数组已初始化部分的长度
     */
    public int getInitializedLength() {
        return valueList.size();
    }

    /**
     * @return 数组（最高维）的长度
     */
    public int getLength() {
        return length;
    }

    public Constant getValueAt(int index) {
        if (index < 0 || index >= length) {
            throw new IndexOutOfBoundsException(index, 0, length);
        }
        if (index < valueList.size()) {
            return valueList.get(index);
        } else {
            return getType().getBaseType().getZeroInitialization();
        }
    }

    @Override
    public boolean isFilledWithZero() {
        return valueList.size() == 0;
    }

    @Override
    public String getValueName() {
        if (isFilledWithZero()) {
            return "zeroinitializer";
        } else {
            var builder = new StringBuilder();
            builder.append('[');
            for (var i = 0; i < length; i++) {
                if (i != 0) {
                    builder.append(", ");
                }
                builder.append(getType().getBaseType().getTypeName()).append(' ');
                if (i < valueList.size()) {
                    builder.append(valueList.get(i).getValueNameIR());
                } else {
                    builder.append(getType().getBaseType().getZeroInitialization().getValueNameIR());
                }
            }
            builder.append(']');
            return builder.toString();
        }
    }
}
