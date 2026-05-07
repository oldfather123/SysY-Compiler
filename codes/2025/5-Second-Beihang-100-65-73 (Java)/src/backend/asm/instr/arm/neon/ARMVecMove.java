package backend.asm.instr.arm.neon;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

public class ARMVecMove extends ARMIns {
    private VirFReg src;

    public ARMVecMove(VirFReg src, ASMBasicBlock parentBlock, VirFReg register) {
        super(parentBlock, register);

        this.src = src;

        addUsedVal(src);
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(src);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("向量移动指令接受且只接受一个操作数");
        }

        this.modifyUse(src, values.getFirst());
        this.src = (VirFReg) values.getFirst();
    }

    @Override
    protected String printIns() {
        if (!(this.getRegister() instanceof VirFReg virFReg
                && virFReg.getPhyReg() instanceof PhyFReg fReg0)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }
        if (!(this.src.getPhyReg() instanceof PhyFReg fReg1)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }

        return "MOV " + fReg0.printAsFullByteVec() + ", " + fReg1.printAsFullByteVec();
    }
}
