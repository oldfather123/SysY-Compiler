package cn.edu.bit.newnewcc.backend.asm.controller;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;

public class LifeTimeIndex implements Comparable<LifeTimeIndex> {
    public enum Type {
        IN, OUT
    }

    private final LifeTimeController lifeTimeController;
    private final AsmInstruction sourceInst;
    private final Type type;

    public Type getType() {
        return type;
    }

    public boolean isIn() {
        return type == Type.IN;
    }

    public boolean isOut() {
        return type == Type.OUT;
    }

    private LifeTimeIndex(LifeTimeController lifeTimeController, AsmInstruction sourceInst, Type type) {
        this.lifeTimeController = lifeTimeController;
        this.sourceInst = sourceInst;
        this.type = type;
    }

    public int getInstID() {
        return lifeTimeController.getInstID(sourceInst);
    }

    public AsmInstruction getSourceInst() {
        return sourceInst;
    }

    static LifeTimeIndex getInstIn(LifeTimeController lifeTimeController, AsmInstruction sourceInst) {
        return new LifeTimeIndex(lifeTimeController, sourceInst, Type.IN);
    }

    static LifeTimeIndex getInstOut(LifeTimeController lifeTimeController, AsmInstruction sourceInst) {
        return new LifeTimeIndex(lifeTimeController, sourceInst, Type.OUT);
    }

    @Override
    public int compareTo(LifeTimeIndex o) {
        if (getInstID() != o.getInstID()) {
            return getInstID() - o.getInstID();
        }
        if (type != o.type) {
            return type == Type.IN ? -1 : 1;
        }
        return 0;
    }

    @Override
    public String toString() {
        return getInstID() + "." + getType();
    }
}
