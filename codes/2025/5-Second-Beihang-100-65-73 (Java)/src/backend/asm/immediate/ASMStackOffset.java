package backend.asm.immediate;

import backend.asm.structure.ASMFunction;

/**
 * 用函数（的栈指针位置）和偏移量确定的立即数
 */
public class ASMStackOffset extends ASMImmediate {
    private final ASMFunction function;
    private final int offset;
    
    public ASMStackOffset(ASMFunction function, int offset) {
        super(0);   // 占位
        this.function = function;
        this.offset = offset;
    }
    
    @Override
    public int getImm() {
        return function.getStackSize() + offset;
    }
}
