package ir.pass.opt;

import ir.instr.BinaryInstr;
import ir.instr.InstType;
import ir.type.IntType;
import ir.value.BasicBlock;
import ir.value.Module;
import tools.ModuleFlattener;

import java.util.LinkedHashSet;

public class InstrComb implements Pass {
    private LinkedHashSet<BasicBlock> visited;

    @Override
    public void run(Module module) {
        for (var func : module.functions.values()) {
            if (func.blocks.size() > 0) {
                visitBlock(func.blocks.get(0));
            }
        }
        ModuleFlattener.flattenModule(module);
        DeadCodeEmit deadCodeEmit = new DeadCodeEmit();
        deadCodeEmit.run(module);
    }

    private long shrink(long value){
        if(value > Integer.MAX_VALUE){
            return Integer.MAX_VALUE;
        }else if(value < Integer.MIN_VALUE){
            return Integer.MIN_VALUE;
        }
        return value;
    }

    private void visitBlock(BasicBlock block) {
        if (visited.contains(block)) {
            return;
        }
        visited.add(block);
        for (var inst : block.getInsts()) {
            if (!(inst instanceof BinaryInstr && inst.type instanceof IntType)) {
                continue;
            }
            BinaryInstr instr = (BinaryInstr) inst;
            if (instr.instType == InstType.BinaryType.icmp ||
                instr.instType == InstType.BinaryType.fcmp) {
                inst.setBound(1, 0);
            } else {
                long upper1 = instr.getOP(0).getUpperBound();
                long upper2 = instr.getOP(1).getUpperBound();
                long lower1 = instr.getOP(0).getLowerBound();
                long lower2 = instr.getOP(1).getLowerBound();
                if (instr.instType == InstType.BinaryType.add) {
                    long predictupper = shrink(upper1+upper2);
                    long predictlower = shrink(lower1+lower2);
                    inst.setBound((int) predictupper, (int) predictlower);
                }else if(instr.instType == InstType.BinaryType.sub){
                    long predictupper = shrink(upper1-lower2);
                    long predictlower = shrink(lower1 - upper2);
                    inst.setBound((int)predictupper, (int)predictlower);
                }
            }
        }
        for (var suc : block.getSuccs()) {
            visitBlock(suc);
        }
    }
}
