package cn.edu.bit.newnewcc.backend.asm;

import cn.edu.bit.newnewcc.backend.asm.operand.Label;

public class AsmConstantLong {
    private final String constantName;
    private final ValueDirective directive;

    private static int counter = 0;

    public AsmConstantLong(long value) {
        constantName = "constant_long_" + "_" + counter;
        counter += 1;
        directive = new ValueDirective(value);
    }
    public Label getConstantLabel() {
        return new Label(constantName, false);
    }
    public String emit() {
        return ".align 2\n"
            + getConstantLabel().emit() + ":\n"
            + directive.emit();
    }
}
