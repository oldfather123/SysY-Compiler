package backend.asm.instr.arm.neon.memory;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * 基础向量加载，把内存中的连续数据加载到一个 NEON 向量寄存器中
 */
public class ARMVecLoad extends ARMIns {
    private Reg base;

    public ARMVecLoad(Reg base, ASMBasicBlock parentBlock, VirFReg register) {
        super(parentBlock, register);
        this.base = base;

        this.addUsedVal(base);
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(base);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("LD1 指令接受且只接受一个操作数");
        }

        modifyUse(base, values.getFirst());
        this.base = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        if (!(this.getRegister() instanceof VirFReg virFReg
                && virFReg.getPhyReg() instanceof PhyFReg fReg0)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }

        return "LD1 { " + fReg0.printAsFullVec() + " }, [" + base.printAsOperand() + "]";
    }
}
