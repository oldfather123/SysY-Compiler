package backend.asm.instr.arm.memory;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.LoadIns;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Collections;
import java.util.List;

public class ARMLoadPair extends ARMIns {
    private final ASMImmediate offset;
    private final Reg anotherReg;
    private ASMValue base;

    private static int index = 0;
    private final int myIndex;

    public ARMLoadPair(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg, Reg anotherReg) {
        super(parentBB, reg);
        myIndex = index++;
        this.offset = offset;
        this.anotherReg = anotherReg;
        this.base = base;

        this.addUsedVal(offset);
        this.addUsedVal(base);
    }

    @Override
    public List<ASMValue> getOperands() {
        return Collections.singletonList(base);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("LDP 指令接受且只接受一个操作数");
        }

        modifyUse(base, values.getFirst());
        this.base = values.getFirst();
    }

    @Override
    protected String printIns() {
        String dstReg1;
        if (this.getRegister() instanceof VirFReg virFReg && virFReg.isVector()) {
            dstReg1 = ((PhyFReg) virFReg.getPhyReg()).printAsWholeScalar();
        } else {
            dstReg1 = this.getRegister().printAsOperand();
        }
        String dstReg2;
        if (this.anotherReg instanceof VirFReg virFReg && virFReg.isVector()) {
            dstReg2 = ((PhyFReg) virFReg.getPhyReg()).printAsWholeScalar();
        } else {
            dstReg2 = this.anotherReg.printAsOperand();
        }
        return "LDP " + dstReg1 + ", " + dstReg2 +
                ", [" + base.printAsOperand() + ", " +  offset.printAsOperand() + "]";
    }
}
