package backend.asm.instr.arm.memory;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public class ARMStorePair extends ARMIns {
    private final ASMImmediate offset;  // updatePointer 为 true 时表示每次迭代变化的量，否则表示相对 base 的偏移量
    private ASMValue base;
    private ASMValue value1;
    private ASMValue value2;   // value to store
    private final boolean updatePointer;

    private static int index = 0;
    private final int myIndex;

    public ARMStorePair(ASMImmediate offset, ASMValue base, ASMValue value1, ASMValue value2, ASMBasicBlock parentBB, boolean updatePointer) {
        super(parentBB, RegStore.NA);
        myIndex = index++;
        this.offset = offset;
        this.base = base;
        this.value1 = value1;
        this.value2 = value2;
        this.updatePointer = updatePointer;

        addUsedVal(offset);
        addUsedVal(base);
        addUsedVal(value1);
        addUsedVal(value2);
    }

    @Override
    public List<ASMValue> getOperands() {
        return Arrays.asList(base, value1, value2);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 3) {
            throw new RuntimeException("STP 指令接受且只接受三个操作数");
        }

        this.modifyUse(base, values.get(0));
        this.modifyUse(value1, values.get(1));
        this.modifyUse(value2, values.get(2));
        this.base = values.get(0);
        this.value1 = values.get(1);
        this.value2 = values.get(2);
    }

    @Override
    protected String printIns() {
        String valueToStore1;
        String valueToStore2;
        if (this.value1 instanceof VirFReg virFReg && virFReg.isVector()) {
            valueToStore1 = ((PhyFReg) virFReg.getPhyReg()).printAsWholeScalar();
        } else {
            valueToStore1 = this.value1.printAsOperand();
        }
        if (this.value2 instanceof VirFReg virFReg && virFReg.isVector()) {
            valueToStore2 = ((PhyFReg) virFReg.getPhyReg()).printAsWholeScalar();
        } else {
            valueToStore2 = this.value2.printAsOperand();
        }
        if (this.updatePointer) {
            return "STP " + valueToStore1 + ", " + valueToStore2 +
                    ", [" + base.printAsOperand() + "]" + ", " +  offset.printAsOperand();
        } else {
            return "STP " + valueToStore1 + ", " + valueToStore2 +
                    ", [" + base.printAsOperand() + ", " +  offset.printAsOperand() + "]";
        }
    }
}
