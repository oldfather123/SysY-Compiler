package backend.asm.instr.rv.others;

import backend.asm.ASMValue;
import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.PureIns;
import backend.asm.register.Reg;
import backend.asm.register.FReg;
import backend.asm.register.IReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

public class RVFtoi extends RVIns implements PureIns {
    private Reg src;

    public RVFtoi(Reg src, ASMBasicBlock block, Reg reg) {
        super(block, reg);
        this.src = src;

        this.addUsedVal(src);

        if (!(src instanceof FReg) || !(reg instanceof IReg)) {
            throw new IllegalArgumentException();
        }
    }

    @Override
    public List<ASMValue> getOperands() {
        return List.of(src);
    }

    public void resetOperands(List<ASMValue> values) {
        if (values.size() != 1 || !(values.getFirst() instanceof FReg)) {
            throw new RuntimeException("FTOI 指令的操作数必须是一个浮点寄存器");
        }
        modifyUse(src, values.getFirst());
        this.src = (Reg) values.getFirst();
    }

    @Override
    protected String printIns() {
        return "fcvt.w.s " + this.printAsOperand() + ", " + src.printAsOperand()+", rtz";
    }
}
