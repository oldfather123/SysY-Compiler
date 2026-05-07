package mid.Optimizer.RedundancyElim;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;

public class GVN {
    //类似于mem2reg，只需要做一个hash替换即可（先前的SSA以及后续的GCM会处理正确性问题）
    private final Module module;
    private final HashMap<String, Value> globalValueNumberMap = new HashMap<>();

    public GVN() {
        this.module = IRManager.getModule();
    }

    public void optimize() {
        for (Function function : module.getDecledFunctions()) {
            //全局编号是针对于函数而言的全局
            globalValueNumberMap.clear();
            numberForFunction(function);
        }
    }

    public void numberForFunction(Function function) {
        ArrayList<BasicBlock> dominTreePath = Optimizer.instance().bfsDominTreeArray(function.getEntranceBlock());
        for (BasicBlock block : dominTreePath) {
            ArrayList<Instruction> instructions = new ArrayList<>(block.getInstructionList());
            ArrayList<Instruction> replacedInstructions = new ArrayList<>();
            for (Instruction instruction : instructions) {
                ArrayList<String> hashs = instruction.GVNHash();
                if (hashs != null) {
                    boolean containsHash = false;
                    for (String hash : hashs) {
                        if (globalValueNumberMap.containsKey(hash)) {
                            containsHash = true;
                            //替换这个表达式，并维护use-def关系，类似于mem2reg时
                            instruction.beReplacedBy(globalValueNumberMap.get(hash));
                            instruction.destroy();
                            replacedInstructions.add(instruction);
                            break;
                        }
                    }
                    if (!containsHash) {
                        globalValueNumberMap.put(hashs.get(0), instruction);
                    }
                }
            }
            block.getInstructionList().removeAll(replacedInstructions);
            replacedInstructions.forEach(Instruction::destroy);
        }
    }
}
