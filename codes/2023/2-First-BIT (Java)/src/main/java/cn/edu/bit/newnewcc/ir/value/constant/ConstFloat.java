package cn.edu.bit.newnewcc.ir.value.constant;

import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.value.Constant;

import java.util.HashMap;
import java.util.Map;

/**
 * 浮点型常量
 */
public class ConstFloat extends Constant {

    private final float value;

    private ConstFloat(float value) {
        super(FloatType.getFloat());
        this.value = value;
    }

    @Override
    public boolean isFilledWithZero() {
        return value == 0;
    }

    public float getValue() {
        return value;
    }

    @Override
    public String getValueName() {
        return "0x" + Long.toHexString(Double.doubleToLongBits(value));
    }

    private static class Cache {
        private static final Map<Float, ConstFloat> cache = new HashMap<>();
    }

    public static ConstFloat getInstance(float value) {
        if (!Cache.cache.containsKey(value))
            Cache.cache.put(value, new ConstFloat(value));

        return Cache.cache.get(value);
    }
}
