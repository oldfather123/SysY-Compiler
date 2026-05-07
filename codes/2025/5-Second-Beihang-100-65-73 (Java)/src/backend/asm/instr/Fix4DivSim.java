package backend.asm.instr;

import backend.asm.register.IReg;
import backend.asm.ASMValue;
import backend.asm.register.Reg;
import backend.asm.register.store.RegStore;

import java.util.List;

/**
 * todo: 这是当时为了修正除法优化写的组合指令，里面全是写死的 MIPS 指令，需要修改
 */
public class Fix4DivSim extends ASMInstruction {
    private static int INDEX = 0;
    private final Reg res;
    private final int myIndex;
    
    public Fix4DivSim(Reg res) {
        super(null, RegStore.NA);
        this.res = res;
        this.myIndex = INDEX++;
    }
    
    @Override
    public List<ASMValue> getOperands() {
        return null;
    }
    
    @Override
    public void resetOperands(List<ASMValue> values) {
    
    }
    
    @Override
    protected String printIns() {
        return "bgez " + res.printAsOperand() +", " + "divSimLabel" + myIndex + "\n" +
                "\t\tadd " + res.printAsOperand() +", " + res.printAsOperand() +", 1\n" +
                "\tdivSimLabel" + myIndex + ":";
    }
}
