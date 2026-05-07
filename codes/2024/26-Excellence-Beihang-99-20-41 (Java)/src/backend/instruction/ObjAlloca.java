package backend.instruction;

import backend.component.ObjInstr;
import ir.instr.AllocaInstr;

public class ObjAlloca extends ObjInstr {
    private AllocaInstr allocaInstr;

    public ObjAlloca(AllocaInstr allocaInstr) {
        super("alloca");
        this.allocaInstr = allocaInstr;
    }

    @Override
    public String toString() {
        //TODO:最终需要去除alloca
        return allocaInstr.toString();
    }
}
