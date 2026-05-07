package backend.asm.instr.arm.memory;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.LoadIns;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public class ARMLoad extends ARMIns implements LoadIns {
    private ASMImmediate offset;
    private ASMValue base;

    private static int index = 0;
    private final int myIndex;

    private boolean fltLi;  // 表示用于取浮点立即数，也就是目标地址存的值不会变

    public ARMLoad(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg) {
        super(parentBB, reg);
        myIndex = index++;
        this.offset = offset;
        this.base = base;

        this.addUsedVal(offset);
        this.addUsedVal(base);
    }

    public ARMLoad(ASMValue ptr, ASMBasicBlock parentBB, Reg reg) {
        super(parentBB, reg);
        myIndex = index++;
        if (ptr instanceof ASMImmediate) {
            throw new RuntimeException("我觉得 load 指令的地址一定不是立即数");
        } else {
            this.offset = ASMImmediate.ZERO;
            this.base = ptr;
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
        String dstReg;
        if (this.getRegister() instanceof VirFReg virFReg && virFReg.isVector()) {
            dstReg = ((PhyFReg) virFReg.getPhyReg()).printAsWholeScalar();
        } else {
            dstReg = this.getRegister().printAsOperand();
        }
        return "LDR " + dstReg + ", [" + base.printAsOperand() + ", " +  offset.printAsOperand() + "]";
    }
}
