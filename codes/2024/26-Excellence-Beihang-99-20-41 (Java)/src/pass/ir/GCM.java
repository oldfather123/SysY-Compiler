package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.instr.PhiInstr;
import ir.value.BasicBlock;
import ir.value.Function;
import ir.value.Value;
import pass.Pass;
import pass.utils.DataStreamInstrMap;
import pass.utils.DominatorTree;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class GCM implements Pass.IrPass {
    private HashMap<IrInstr,String> instrToBlock;
    private HashSet<IrInstr> visited;
    private HashMap<IrInstr,String> instrToNewBlock;
    HashMap<String, IrInstr> valueToInstr;
    LinkedList<BasicBlock> basicBlocks;
    HashMap<String, BasicBlock> blockMap;
    HashMap<String,HashSet<String>> dominatorSons;
    String root;

    @Override
    public void run(ir.IrModule module) {
        passStepOne(module);
        passStepTwo(module);
    }

    private void passStepOne(IrModule module) {
        for (Function func : module.getFunctions()) {
            for (BasicBlock block : func.getBasicBlocks()) {
                for (IrInstr instr : block.getInstrs()) {
                    instrToBlock.put(instr, block.getName());
                }
            }
        }
    }

    private void passStepTwo(IrModule module) {
        for (ir.value.Function func : module.getFunctions()) {
            DataStreamInstrMap.initialize(new HashMap<>(), valueToInstr, basicBlocks, blockMap);
            DominatorTree.run(func, dominatorSons);
            tackleFuncEarly(func);
        }
    }

    private void tackleFuncEarly(Function func) {
        HashMap<String, HashSet<String>> dominatorSons = new HashMap<>();
        DominatorTree.run(func, dominatorSons);
        root = func.getBasicBlocks().get(0).getName();
        for (BasicBlock block : func.getBasicBlocks()) {
            for (IrInstr instr : block.getInstrs()) {
                if (isPinned(instr)) {
                    visited.add(instr);
                    for (Value value : instr.getDependentValues()) {
                        if (valueToInstr.containsKey(value.getName())) {
                            scheduleEarly(valueToInstr.get(value.getName()));
                        }
                    }
                }
            }
        }
    }

    private void scheduleEarly(IrInstr instr) {
        if (visited.contains(instr)) {
            return;
        }
        visited.add(instr);
        instrToNewBlock.put(instr, root);
        for (Value value : instr.getDependentValues()) {
            if (valueToInstr.containsKey(value.getName())) {
                IrInstr dependentInstr = valueToInstr.get(value.getName());
                scheduleEarly(dependentInstr);
                if (dominatorSons.get(instrToNewBlock.get(dependentInstr)).
                        contains(instrToNewBlock.get(instr))) {
                    instrToNewBlock.put(instr, instrToNewBlock.get(dependentInstr));
                }
            }
        }
    }

    private boolean isPinned(IrInstr instr) {
        return instr instanceof ir.instr.TermInstr || instr instanceof PhiInstr;
    }

    @Override
    public String getName() {
        return "GCM";
    }
}
