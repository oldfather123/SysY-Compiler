package backend.asm.instr.rv.others;

import backend.asm.ASMValue;
import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.AtomIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

public class RVAtomAdd extends RVIns implements AtomIns {
    public ASMValue inc;
    public ASMValue base;
    public ASMValue dst;

    public RVAtomAdd(ASMValue inc, ASMValue base, ASMValue dst, ASMBasicBlock parentBB) {
        super(parentBB, RegStore.NA);
        this.inc = inc;
        this.base = base;
        this.dst = dst;

        addUsedVal(inc);
        addUsedVal(base);
        addUsedVal(dst);
    }

    public List<ASMValue> getOperands() {
        return List.of(inc, base, dst);
    }

    public void resetOperands(List<ASMValue> newOperands) {
        if (newOperands == null || newOperands.size() != 3) {
            throw new RuntimeException("RVAtomAdd 的操作数数量必须为 3");
        }
        this.modifyUse(inc, newOperands.getFirst());
        this.modifyUse(base, newOperands.get(1));
        this.modifyUse(dst, newOperands.get(2));
        this.inc = newOperands.getFirst();
        this.base = newOperands.get(1);
        this.dst = newOperands.get(2);
    }

    public String printIns() {
        return "amoadd.w.aqrl " + dst + ", " + inc + ", (" + base + ")";
    }

}
