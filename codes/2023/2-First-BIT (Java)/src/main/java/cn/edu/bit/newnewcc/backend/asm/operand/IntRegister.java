package cn.edu.bit.newnewcc.backend.asm.operand;

import java.util.HashMap;
import java.util.Map;

/**
 * riscv的普通寄存器为x0~x31
 */
public class IntRegister extends Register {
    private final int index;

    /**
     * 生成指定下标的寄存器
     *
     * @param index 下标，下标为负数时代表其为暂时未分配的普通临时寄存器，等待分配过程
     */
    private IntRegister(int index) {
        this.index = index;
    }

    @Override
    public String getName() {
        return isVirtual() ? "vx" + getAbsoluteIndex() : "x" + getAbsoluteIndex();
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
        return String.format("IntRegister(%d)", getIndex());
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        IntRegister that = (IntRegister) o;

        return index == that.index;
    }

    @Override
    public int hashCode() {
        return index;
    }

    private static final Map<Integer, IntRegister> cache = new HashMap<>();

    public static IntRegister getVirtual(int absoluteIndex) {
        return new IntRegister(-absoluteIndex);
    }

    public static IntRegister getPhysical(int index) {
        if (!cache.containsKey(index)) {
            cache.put(index, new IntRegister(index));
        }
        return cache.get(index);
    }

    public static IntRegister getParameter(int index) {
        return getPhysical(index + 10);
    }

    public static final IntRegister ZERO = getPhysical(0);
    public static final IntRegister RA = getPhysical(1);
    public static final IntRegister SP = getPhysical(2);
    public static final IntRegister S0 = getPhysical(8);
    public static final IntRegister S1 = getPhysical(9);
}
