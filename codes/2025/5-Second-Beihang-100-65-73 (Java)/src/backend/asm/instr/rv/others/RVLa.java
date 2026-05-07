package backend.asm.instr.rv.others;

import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.ASMValue;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMGlobalObject;

import java.util.List;

/**
 * la $t1,label            Load Address : Set $t1 to label's address
 */
public class RVLa extends RVIns implements PureIns {
    private ASMGlobalObject globalObject;

    public RVLa(ASMGlobalObject globalObject, ASMBasicBlock parentBB, Reg reg) {
        super(parentBB, reg);
        this.globalObject = globalObject;
    }

    public String getLabelName() {
        return globalObject.getLabelName();
    }

    public ASMGlobalObject getGlobalObject() {
        return globalObject;
    }

    public boolean isLoadingFloat() {
        return globalObject.isFloat();
    }

    public void setGlobalObject(ASMGlobalObject globalObject) {
        this.globalObject = globalObject;
    }

    @Override
    public List<ASMValue> getOperands() {
        return null;
    }
    
    @Override
    public void resetOperands(List<ASMValue> values) {
        throw new RuntimeException("LA 指令不接受操作数");
    }
    
    @Override
    protected String printIns() {
        return "la " + this.printAsOperand() + ", " + globalObject.getLabelName();
    }
}
