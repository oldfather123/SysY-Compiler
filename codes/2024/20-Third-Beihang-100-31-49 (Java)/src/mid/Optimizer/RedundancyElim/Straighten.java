package mid.Optimizer.RedundancyElim;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.User;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashSet;

public class Straighten {
    private final Module module;

    public Straighten() {
        module = IRManager.getModule();
    }

    public void optimize() {
        optimizeBr();        // 合并连续br, A只有一个后继B, B只有一个前驱A
        optimizePhi();       // A为单独Br, 只有一个前驱, 一个后继B; B有多个前驱，且B的前驱不包含A的前驱
        Optimizer.instance().dominAnalyze();
    }

    //对每个块不断进行块合并，直到该块不能被优化
    public void optimizeBr() {
        (new ConstFold()).optimize();
        ArrayList<Function> decledFunctions = module.getDecledFunctions();
        for (Function f : decledFunctions) {
            ArrayList<BasicBlock> blocks = new ArrayList<>(f.getBlocks());
            for (int i = 0; i < blocks.size(); i++) {
                BasicBlock b = blocks.get(i);
                ArrayList<BasicBlock> blocksToMerge = new ArrayList<>();    // 所有可以被合并到b中的block
                BasicBlock index = b;
                while (beMerged(index)) {
                    BasicBlock next = Optimizer.instance().getCFG().getChildren(index).iterator().next();
                    blocksToMerge.add(next);
                    index = next;
                }
                for (BasicBlock block : blocksToMerge) {
                    // 去除br
                    b.getLastInstruction().destroy();
                    b.deleteLastInstruction();
                    for (Instruction inst : block.getInstructionList()) {
                        b.addInstruction(inst);
                    }
                    Function func = b.getFunction();
                    func.removeBlock(block);
                    block.beReplacedBy(b);
                }
                Optimizer.instance().getCFG().analyze(f);
                blocks = new ArrayList<>(f.getBlocks());
            }
        }
    }

    private boolean beMerged(BasicBlock a) {
        Instruction lastInstruction = a.getLastInstruction();
        if (!(lastInstruction instanceof Br)) return false;
        HashSet<BasicBlock> children = Optimizer.instance().getCFG().getChildren(a);
        if (!(children != null && children.size() == 1)) return false;
        BasicBlock b = children.iterator().next();
        HashSet<BasicBlock> parents = Optimizer.instance().getCFG().getParents(b);
        if (!(parents != null && parents.size() == 1)) return false;
        return a.getFunction() == b.getFunction();
    }

    public void optimizePhi() {
        (new ConstFold()).optimize();
        ArrayList<Function> decledFunctions = module.getDecledFunctions();
        for (Function f : decledFunctions) {
            ArrayList<BasicBlock> blocks = new ArrayList<>(f.getBlocks());
            ArrayList<BasicBlock> blocksToDelete = new ArrayList<>();
            for (int i = 0; i < blocks.size(); i++) {
                BasicBlock b = blocks.get(i);
                if (beMergedPhi(b)) {
                    BasicBlock pre = Optimizer.instance().getCFG().getParents(b).iterator().next();
                    BasicBlock next = Optimizer.instance().getCFG().getChildren(b).iterator().next();
                    for (User user : b.getUserList()) {
                        if (user instanceof Phi phi) {
                            phi.redirectFrom(b, pre);
                        } else if (user instanceof Br br) {
                            br.redirectTo(b, next);
                        }
                    }
                    blocksToDelete.add(b);
                }
                Optimizer.instance().getCFG().analyze(f);
                blocks = new ArrayList<>(f.getBlocks());
            }
            for (BasicBlock block : blocksToDelete) {
                block.getFunction().removeBlock(block);
                block.destroy();
            }
        }
    }

    private boolean beMergedPhi(BasicBlock a) {
        if (a.getInstructionList().size() != 1) return false;
        Instruction lastInstruction = a.getLastInstruction();
        if (!(lastInstruction instanceof Br)) return false;                 // a为单独br
        HashSet<BasicBlock> parentsA = Optimizer.instance().getCFG().getParents(a);
        if (!(parentsA != null && parentsA.size() == 1)) return false;      // a只有一个前驱
        BasicBlock parentA = parentsA.iterator().next();
        HashSet<BasicBlock> children = Optimizer.instance().getCFG().getChildren(a);
        if (!(children != null && children.size() == 1)) return false;      // a只有一个后继
        BasicBlock b = children.iterator().next();
        HashSet<BasicBlock> parentsB = Optimizer.instance().getCFG().getParents(b);
        if (!(parentsB != null && parentsB.size() != 1)) return false;      // b有多个前驱
        for (BasicBlock parentB : parentsB) {
            if (parentB == parentA) return false;                           // b的前驱不包含a的前驱
        }
        return a.getFunction() == b.getFunction();
    }
}
