package backend.asm.instr.arm.others;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.MoveIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.FReg;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyIReg;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Collections;
import java.util.List;

public class ARMMove extends ARMIns implements MoveIns {
    private Reg src;

    public ARMMove(Reg src, ASMBasicBlock parentBlock, Reg reg) {
        super(parentBlock, reg);
        this.src = src;

        this.addUsedVal(src);
    }

    @Override
    public boolean isFrozen() {
        return false;   // fixme: 再说吧
    }

    @Override
    public Reg getSrc() {
        return src;
    }

    @Override
    public List<ASMValue> getOperands() {
        return Collections.singletonList(src);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("MOVE 指令接受且只接受一个操作数");
        }
        modifyUse(src, values.getFirst());
        this.src = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        if (src instanceof FReg) {
            return "FMOV " + this.printAsOperand() + ", " + src.printAsOperand();
        } else {
            if (this.getRegister() instanceof PhyIReg phyIReg && this.src instanceof VirIReg virIReg) {
                if (virIReg.isDouble()) {
                    return "MOV " + phyIReg.printAsDouble() + ", " + src.printAsOperand();
                }
            } else if (this.getRegister() instanceof VirIReg virIReg && this.src instanceof PhyIReg phyIReg) {
                if (virIReg.isDouble()) {
                    return "MOV " + this.printAsOperand() + ", " + phyIReg.printAsDouble();
                }
            } else if (this.getRegister() instanceof VirIReg virIRegDst && this.src instanceof VirIReg virIRegSrc
                    && !virIRegDst.isDouble() && virIRegSrc.isDouble()) {
                return  "MOV " + this.printAsOperand() + ", " + virIRegSrc.printAsWord();
            }
            return "MOV " + this.printAsOperand() + ", " + src.printAsOperand();
        }
    }
}
