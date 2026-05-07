package cn.edu.bit.newnewcc.ir.value.constant;

import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.Constant;

/**
 * 常整数类（未指定位数） <br>
 * 包含的子类有：
 * <ul>
 *     <li>ConstInt</li>
 *     <li>ConstBool</li>
 * </ul>
 */
public abstract class ConstInteger extends Constant {
    private final int bitWidth;

    protected ConstInteger(int bitWidth) {
        super(IntegerType.getInstance(bitWidth));
        this.bitWidth = bitWidth;
    }

    public int getBitWidth() {
        return bitWidth;
    }

    public static ConstInteger getInstance(int bitWidth, long value) {
        return switch (bitWidth) {
            case 1 -> ConstBool.getInstance(value != 0);
            case 32 -> ConstInt.getInstance((int) value);
            case 64 -> ConstLong.getInstance(value);
            default ->
                    throw new IllegalArgumentException(String.format("Bit width %d not suitable for integer type.", bitWidth));
        };
    }

    public static long valueOf(ConstInteger constInteger) {
        if (constInteger instanceof ConstBool constBool) {
            return constBool.getValue() ? 1 : 0;
        } else if (constInteger instanceof ConstInt constInt) {
            return constInt.getValue();
        } else if (constInteger instanceof ConstLong constLong) {
            return constLong.getValue();
        } else {
            throw new IllegalArgumentException("Unknown const integer class: " + constInteger.getClass());
        }
    }

}
