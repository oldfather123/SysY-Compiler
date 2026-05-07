package backend.component;

import backend.operand.RiscvLabel;

import java.lang.reflect.Array;
import java.util.ArrayList;

public class RiscvGlobalVariable extends RiscvLabel implements RiscvGlobalElement  {
    private final boolean isInit;
    private final int size;
    private final ArrayList<RiscvGlobalElement> riscvGlobalElements;

    public ArrayList<RiscvGlobalElement> getRiscvGlobalElements() {
        return riscvGlobalElements;
    }

    public RiscvGlobalVariable(String name, boolean isInit, int size, ArrayList<RiscvGlobalElement> riscvGlobalElements) {
        super(name);
        this.isInit = isInit;
        this.size = size;
        this.riscvGlobalElements = riscvGlobalElements;
    }

    public String getName() {
        return name;
    }

    public boolean isInit() {
        return isInit;
    }

    public int getSize() {
        return size;
    }

    public String decl() {
        StringBuilder sb = new StringBuilder();
        sb.append(".global\t").append(name).append("\n");
        sb.append(name).append(":\n");
        if(isInit) {
            for (var riscvGlobalElement : riscvGlobalElements) {
                sb.append(riscvGlobalElement);
            }
        } else{
            sb.append("\t.zero\t").append(size).append("\n");
        }
        return sb.toString();
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (var riscvGlobalElement : riscvGlobalElements) {
            sb.append(riscvGlobalElement);
        }
        return sb.toString();
    }
}
