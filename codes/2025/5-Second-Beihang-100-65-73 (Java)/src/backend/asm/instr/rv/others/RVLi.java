package backend.asm.instr.rv.others;

import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.LiIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.ASMValue;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.immediate.ASMImmediate;

import java.util.List;

public class RVLi extends RVIns implements LiIns, PureIns {
    private final ASMImmediate imm;
    
    public RVLi(ASMImmediate imm, ASMBasicBlock parentBlock, Reg reg) {
        super(parentBlock, reg);
        this.imm = imm;
        
        this.addUsedVal(imm);
    }

    @Override
    public ASMImmediate getImmediate() {
        return imm;
    }
    
    @Override
    public List<ASMValue> getOperands() {
        return null;
    }
    
    @Override
    public void resetOperands(List<ASMValue> values) {
        throw new RuntimeException("LI 指令不接受操作数");
    }
    
    @Override
    protected String printIns() {
        return "li " + this.printAsOperand() + ", " + imm.printAsOperand();
    }
}
