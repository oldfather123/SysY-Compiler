package backend.operand;

import backend.component.RiscvFunction;

public class StackFixer extends RiscvImm {
    private int offset;//参数偏移

    private RiscvFunction function;

    public StackFixer(RiscvFunction function, int extraOffset) {
        super();
        this.function = function;
        this.offset = extraOffset;
    }

    @Override
    public String toString() {
        if(function.getStackPos() == 0)return Integer.toString(offset);
        else return Integer.toString(function.getStackPos() - 1 -
                (function.getStackPos() - 1) % 16 + 16+offset);
    }
}
