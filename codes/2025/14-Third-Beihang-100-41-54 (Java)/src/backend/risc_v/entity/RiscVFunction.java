package backend.risc_v.entity;

import java.util.ArrayList;

public class RiscVFunction {

    private final String name;
    private final ArrayList<RiscVBasicBlock> basicBlocks;

    public RiscVFunction(String name) {
        this.name = name;
        this.basicBlocks = new ArrayList<>();
    }

    public String getName() {
        return name;
    }

    public void addBasicBlock(RiscVBasicBlock block) {
        basicBlocks.add(block);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (RiscVBasicBlock block : basicBlocks) {
            sb.append(block.toString());
        }
        return sb.toString();
    }


}
