package backend.asm.instr.rv.memory;

import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.StoreIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;

import java.util.Arrays;
import java.util.List;

/**
 * sw $t1,-100($t2)        Store word : Store contents of $t1 into effective memory word address
 */
public class RVStore extends RVIns implements StoreIns {
    private ASMImmediate offset;
    private ASMValue base;
    private ASMValue value;   // value to store
    private static int index = 0;
    private final int myIndex;
    private final boolean isDouble;
    private final boolean isFloat;

    public RVStore(ASMImmediate offset, ASMValue base, ASMValue value, ASMBasicBlock parentBB, boolean isDouble, boolean isFloat) {
        super(parentBB, RegStore.NA);
        myIndex = index++;
        this.offset = offset;
        this.base = base;
        this.value = value;
        this.isDouble = isDouble;
        this.isFloat = isFloat;
        
        if (isDouble && isFloat) {
            throw new RuntimeException("双字和浮点不可能同时为真");
        }

        addUsedVal(offset);
        addUsedVal(base);
        addUsedVal(value);
    }
    
    public RVStore(ASMValue ptr, ASMValue value, ASMBasicBlock parentBB, boolean isDouble, boolean isFloat) {
        super(parentBB, RegStore.NA);
        myIndex = index++;
        if (ptr instanceof ASMImmediate) {
            throw new RuntimeException("我觉得 store 指令的地址一定不是立即数");
        } else {
            this.offset = ASMImmediate.ZERO;
            this.base = ptr;
        }
        this.value = value;
        this.isDouble = isDouble;
        this.isFloat = isFloat;
        
        if (isDouble && isFloat) {
            throw new RuntimeException("双字和浮点不可能同时为真");
        }
        
        addUsedVal(offset);
        addUsedVal(base);
        addUsedVal(value);
    }
    
    public boolean isDouble() {
        return isDouble;
    }

    public boolean isFloat() {
        return isFloat;
    }

    public void setOffset(ASMImmediate offset) {
        this.offset = offset;
        addUsedVal(offset);
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
    public ASMValue getValue() {
        return value;
    }
    
    @Override
    public List<ASMValue> getOperands() {
        return Arrays.asList(base, value, offset);
    }
    
    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 3) {
            throw new RuntimeException("SW 指令接受且只接受三个操作数");
        }
        
        this.modifyUse(base, values.get(0));
        this.modifyUse(value, values.get(1));
        this.modifyUse(offset, values.get(2));
        this.base = values.get(0);
        this.value = values.get(1);
        this.offset = (ASMImmediate) values.get(2);
    }
    
    @Override
    protected String printIns() {
        StringBuilder sb = new StringBuilder();
        if (isFloat) {
            sb.append("f");
        }
        if (isDouble) {
            sb.append("sd ");
        } else {
            sb.append("sw ");
        }
        sb.append(value.printAsOperand());
        sb.append(", ");
        sb.append(offset.printAsOperand());
        sb.append("(");
        sb.append(base.printAsOperand());
        sb.append(")");
        return sb.toString();
    }
}
