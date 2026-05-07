package riscv.value;

import midend.value.BasicBlock;
import riscv.util.AllocTable;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;

public class AsmFunction {
    public final String funcName;

    public AsmFunction(String funcName) {
        this.funcName = funcName;
    }

    public final ArrayList<AsmBasicBlock> bbList = new ArrayList<>();
    private final Map<BasicBlock, AsmBasicBlock> bbMap = new LinkedHashMap<>();

    public final AllocTable allocTable = new AllocTable();

    private AsmBasicBlock createBB(BasicBlock bb) {
        AsmBasicBlock asmBB = new AsmBasicBlock(bb.getName());
        bbMap.put(bb, asmBB);
        bbList.add(asmBB);
        return asmBB;
    }

    public AsmBasicBlock getOrCreateBB(BasicBlock bb) {
        if (bbMap.containsKey(bb)) {
            return bbMap.get(bb);
        }
        else {
            return createBB(bb);
        }
    }
}
