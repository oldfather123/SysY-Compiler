package backend.operand;

import tools.IrRegDispatcher;

public class RiscvLabel extends RiscvImm{
    public void setName(String name) {
        this.name = name;
    }

    protected String name;

    public RiscvLabel(IrRegDispatcher irRegDispatcher) {
        super();
        name = "Label" + irRegDispatcher.allocId("Label");
    }

    public RiscvLabel(String name) { //用于全局变量 以及函数
        super();
        this.name = name;
    }

    public RiscvLabel hi() {
        return new RiscvLabel("%hi(" + name + ")");
    }

    public RiscvLabel lo() {
        return new RiscvLabel("%lo(" + name + ")");
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return name;
    }

    public String toPrint() {
        return name + ":\n";
    }
}

