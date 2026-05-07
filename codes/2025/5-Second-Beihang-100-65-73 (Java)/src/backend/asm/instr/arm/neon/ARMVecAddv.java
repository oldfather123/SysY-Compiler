package backend.asm.instr.arm.neon;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * 水平归约
 * `ADDV s1, v0.4s` <==> `s1 = v0[0] + v0[1] + v0[2] + v0[3]`
 * 只能用浮点寄存器接受结果，后续再移动到整数通用寄存器里去
 */
public class ARMVecAddv extends ARMIns {
    private VirFReg srcVec;

    public ARMVecAddv(VirFReg srcVec, ASMBasicBlock parentBlock, VirFReg register) {
        super(parentBlock, register);

        this.srcVec = srcVec;

        this.addUsedVal(srcVec);
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(srcVec);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("ADDV 指令接受且只接受一个操作数");
        }

        modifyUse(srcVec, values.getFirst());
        this.srcVec = (VirFReg) values.getFirst();
    }

    @Override
    protected String printIns() {
        return "ADDV " + this.printAsOperand() + ", " + ((PhyFReg) this.srcVec.getPhyReg()).printAsFullVec();
    }
}
