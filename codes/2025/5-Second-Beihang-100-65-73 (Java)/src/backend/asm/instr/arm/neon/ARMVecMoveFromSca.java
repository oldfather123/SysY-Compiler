package backend.asm.instr.arm.neon;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

public class ARMVecMoveFromSca extends ARMIns {
    private Reg src0;
    private final int index;

    public ARMVecMoveFromSca(Reg src0, int index, ASMBasicBlock parentBlock, VirFReg register) {
        super(parentBlock, register);

        this.src0 = src0;
        this.index = index;

        addUsedVal(src0);
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(src0);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("加载标量到向量指令接受且只接受一个操作数");
        }

        this.modifyUse(src0, values.getFirst());
        this.src0 = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        if (!(this.getRegister() instanceof VirFReg virFReg
                && virFReg.getPhyReg() instanceof PhyFReg fReg)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }

        return "MOV " + fReg.printAsPartNOfVec(this.index) + ", " + src0.printAsOperand();
    }
}
