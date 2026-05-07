package midend.value.instrs;

import midend.value.BasicBlock;

public class Reg extends Instr {

    public Reg(BasicBlock parent) {
        super(parent);
        setName("%reg" + REG_NUM++);
    }
}
