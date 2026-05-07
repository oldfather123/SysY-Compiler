package midend.value.instrs;

import midend.value.BasicBlock;

public class Loc extends Instr {

    public Loc(BasicBlock parent) {
        super(parent);
        setName("%loc" + LOC_NUM++);
    }
}
