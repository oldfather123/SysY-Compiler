package cn.edu.bit.newnewcc.ir.value.constant;

import java.util.HashMap;
import java.util.Map;

public class ConstLong extends ConstInteger {
    private final long value;

    private ConstLong(long value) {
        super(64);
        this.value = value;
    }

    @Override
    public boolean isFilledWithZero() {
        return value == 0;
    }

    public long getValue() {
        return value;
    }

    @Override
    public String getValueName() {
        return String.valueOf(value);
    }

    private static class Cache {
        private static final Map<Long, ConstLong> cache = new HashMap<>();
    }

    public static ConstLong getInstance(long value) {
        if (!Cache.cache.containsKey(value))
            Cache.cache.put(value, new ConstLong(value));

        return Cache.cache.get(value);
    }
}
