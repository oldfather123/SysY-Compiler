package cn.edu.bit.newnewcc.backend.asm.operand;

import java.util.HashMap;
import java.util.Map;

//riscv的浮点寄存器为f0~f31
public class FloatRegister extends Register {
    private final int index;

    /**
     * 生成指定下标的浮点寄存器
     */
    private FloatRegister(int index) {
        this.index = index;
    }

    @Override
    public String getName() {
        return isVirtual() ? "vf" + getAbsoluteIndex() : "f" + getAbsoluteIndex();
    }

    @Override
    public int getIndex() {
        return index;
    }

    @Override
    public String emit() {
        return getName();
    }

    @Override
    public String toString() {
        return String.format("FloatRegister(%s)", getIndex());
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        FloatRegister that = (FloatRegister) o;

        return index == that.index;
    }

    @Override
    public int hashCode() {
        return index;
    }

    private static final Map<Integer, FloatRegister> cache = new HashMap<>();

    public static FloatRegister getVirtual(int absoluteIndex) {
        return new FloatRegister(-absoluteIndex);
    }

    public static FloatRegister getPhysical(int index) {
        if (!cache.containsKey(index)) {
            cache.put(index, new FloatRegister(index));
        }
        return cache.get(index);
    }

    public static FloatRegister getParameter(int index) {
        return getPhysical(index + 10);
    }
}
