package cn.edu.bit.newnewcc.backend.asm;

import cn.edu.bit.newnewcc.backend.asm.operand.Label;

public class AsmConstantFloat {
    private final String constantName;
    private final ValueDirective directive;
    public AsmConstantFloat(float value) {
        constantName = "constant_float_" + "_" + Integer.toUnsignedString(Float.floatToIntBits(value));
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
