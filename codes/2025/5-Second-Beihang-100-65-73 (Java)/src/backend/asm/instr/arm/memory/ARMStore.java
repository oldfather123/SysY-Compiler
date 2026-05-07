package backend.asm.instr.arm.memory;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.StoreIns;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public class ARMStore extends ARMIns implements StoreIns {
    private ASMImmediate offset;
    private ASMValue base;
    private ASMValue value;   // value to store

    private static int index = 0;
    private final int myIndex;

    public ARMStore(ASMImmediate offset, ASMValue base, ASMValue value, ASMBasicBlock parentBB) {
        super(parentBB, RegStore.NA);
        myIndex = index++;
        this.offset = offset;
        this.base = base;
        this.value = value;

        addUsedVal(offset);
        addUsedVal(base);
        addUsedVal(value);
    }

    public ARMStore(ASMValue ptr, ASMValue value, ASMBasicBlock parentBB) {
        super(parentBB, RegStore.NA);
        myIndex = index++;
        if (ptr instanceof ASMImmediate) {
            throw new RuntimeException("我觉得 store 指令的地址一定不是立即数");
        } else {
            this.offset = ASMImmediate.ZERO;
            this.base = ptr;
        }
        this.value = value;
        
        addUsedVal(offset);
        addUsedVal(base);
        addUsedVal(value);
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
        String valueToStore;
        if (this.value instanceof VirFReg virFReg && virFReg.isVector()) {
            valueToStore = ((PhyFReg) virFReg.getPhyReg()).printAsWholeScalar();
        } else {
            valueToStore = this.value.printAsOperand();
        }
        return "STR " + valueToStore + ", [" + base.printAsOperand() + ", " +  offset.printAsOperand() + "]";
    }
}
