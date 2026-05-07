package backend.asm.instr.rv.memory;

import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.LoadIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;

import java.util.Arrays;
import java.util.List;

/**
 * lw $t1,-100($t2)        Load word : Set $t1 to contents of effective memory word address
 */
public class RVLoad extends RVIns implements LoadIns {
    private ASMImmediate offset;
    private ASMValue base;
    private static int index = 0;
    private final int myIndex;
    private final boolean isDouble;
    private final boolean isFloat;

    private boolean fltLi;  // 表示用于取浮点立即数，也就是目标地址存的值不会变

    public RVLoad(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg, boolean isDouble, boolean isFloat) {
        super(parentBB, reg);
        myIndex = index++;
        this.offset = offset;
        this.base = base;
        this.isDouble = isDouble;
        this.isFloat = isFloat;
        
        if (isDouble && isFloat) {
            throw new RuntimeException("双字和浮点不可能同时为真");
        }

        this.addUsedVal(offset);
        this.addUsedVal(base);
    }

    public RVLoad(ASMValue ptr, ASMBasicBlock parentBB, Reg reg, boolean isDouble, boolean isFloat) {
        super(parentBB, reg);
        myIndex = index++;
        if (ptr instanceof ASMImmediate) {
            throw new RuntimeException("我觉得 load 指令的地址一定不是立即数");
        } else {
            this.offset = ASMImmediate.ZERO;
            this.base = ptr;
        }
        this.isDouble = isDouble;
        this.isFloat = isFloat;
        
        if (isDouble && isFloat) {
            throw new RuntimeException("双字和浮点不可能同时为真");
        }
        
        addUsedVal(offset);
        this.addUsedVal(base);
    }

    public void markAsFltLi() {
        this.fltLi = true;
    }

    public boolean isFltLi() {
        return fltLi;
    }
    
    public boolean isDouble() {
        return isDouble;
    }

    public boolean isFloat() {
        return isFloat;
    }

    @Override
    public List<ASMValue> getOperands() {
        return Arrays.asList(base, offset);
    }
    
    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 2) {
            throw new RuntimeException("LW 指令接受且只接受两个操作数");
        }
        
        modifyUse(base, values.get(0));
        modifyUse(offset, values.get(1));
        this.base = values.get(0);
        this.offset = (ASMImmediate) values.get(1);
    }

    public void setOffset(ASMImmediate offset) {
        this.offset = offset;
        this.addUsedVal(offset);
    }

    @Override
    public ASMImmediate getOffset() {
        return offset;
    }

    @Override
    public ASMValue getBase() {
        return base;
    }

    @Override
    protected String printIns() {
        StringBuilder sb = new StringBuilder();
        if (isFloat) {
            sb.append("f");
        }
        if (isDouble) {
            sb.append("ld ");
        } else {
            sb.append("lw ");
        }
        sb.append(this.printAsOperand());
        sb.append(", ");
        sb.append(offset.printAsOperand());
        sb.append("(");
        sb.append(base.printAsOperand());
        sb.append(")");
        return sb.toString();
    }
}
