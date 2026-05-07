package riscv.value;

import util.nodelist.NodeList;

public class AsmBasicBlock {
    public final String name;
    public final NodeList<AsmInstr> instrs = new NodeList<>();

    public AsmBasicBlock(String name) {
        this.name = name;
    }
}
