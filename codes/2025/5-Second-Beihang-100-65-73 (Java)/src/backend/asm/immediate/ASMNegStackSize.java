package backend.asm.immediate;

import backend.asm.structure.ASMFunction;

public class ASMNegStackSize extends ASMImmediate {
    private final ASMFunction function;
    
    public ASMNegStackSize(ASMFunction function) {
        super(0); // 占位
        this.function = function;
    }
    
    @Override
    public int getImm() {
        return function.getStackSize() * -1;
    }
}
