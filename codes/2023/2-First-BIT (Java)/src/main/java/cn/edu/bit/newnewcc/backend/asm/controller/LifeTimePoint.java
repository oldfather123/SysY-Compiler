package cn.edu.bit.newnewcc.backend.asm.controller;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmJump;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLabel;

public class LifeTimePoint implements Comparable<LifeTimePoint> {
    public enum Type {
        USE, DEF
    }

    private final LifeTimeIndex index;
    private final Type type;

    private LifeTimePoint(LifeTimeIndex index, Type type) {
        this.index = index;
        this.type = type;
    }
    public static LifeTimePoint getUse(LifeTimeIndex index) {
        return new LifeTimePoint(index, Type.USE);
    }
    public static LifeTimePoint getDef(LifeTimeIndex index) {
        return new LifeTimePoint(index, Type.DEF);
    }

    public Type getType() {
        return type;
    }

    public LifeTimeIndex getIndex() {
        return index;
    }

    public boolean isDef() {
        return type == Type.DEF;
    }

    public boolean isUse() {
        return type == Type.USE;
    }

    public boolean isTruePoint() {
        var inst = getIndex().getSourceInst();
        return !(inst instanceof AsmLabel || inst instanceof AsmJump);
    }

    @Override
    public String toString() {
        return getIndex() + ":" + getType();
    }

    @Override
    public int compareTo(LifeTimePoint o) {
        return index.compareTo(o.getIndex());
    }

}
