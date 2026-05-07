package backend.operand;

import java.util.Objects;

public class ObjOperand {
    public static int operandIndex = 0;
    private String name;
    boolean needsColor = true;
    public int spillPlace = -1;
    public int color = -1;
    private int linearBirthIndex = Integer.MAX_VALUE;
    private int linearDeathIndex = Integer.MIN_VALUE;
    private boolean canUseT = false;

    public void checkLivingRange(int start, int end) {
        if (start <= linearBirthIndex && linearDeathIndex <= end) {
            canUseT = true;
        }
    }

    public void updateBirth(int index) {
        linearBirthIndex = Math.min(index, linearBirthIndex);
    }

    public int getBirth() {
        return linearBirthIndex;
    }

    public int getDeath() {
        return linearDeathIndex;
    }

    public void updateDeath(int index) {
        linearDeathIndex = Math.max(index, linearDeathIndex);
    }

    public void notNeedColor() {
        this.needsColor = false;
    }

    public boolean isPrecolored() { return !needsColor; }

    public ObjOperand(String name) {
        this.name = name;
    }

    public ObjOperand() {
        name = "%" + operandIndex++;//调用时自增，重新建立一个操作数
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return name;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || !(obj instanceof ObjOperand)) {
            return false;
        }
        return ((ObjOperand) obj).name.equals(this.name);
    }

    @Override
    public int hashCode() {
        return Objects.hash(name);
    }
}
