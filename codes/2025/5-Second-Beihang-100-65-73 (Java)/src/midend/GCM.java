package midend;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.*;

public class GCM {
    private static final HashSet<Instruction> visited = new HashSet<>();
    private static STRATEGY globalStrategy;

    public enum STRATEGY {
        DOMEARLY, ASLATE
    }

    public static void execute(List<Function> functions, STRATEGY strategy) {
        globalStrategy = strategy;
        for (Function function : functions) {
            codeMotion(function);
        }
    }

    private static void codeMotion(Function function) {
        visited.clear();
        ArrayList<Instruction> instructions = new ArrayList<>();

        List<BasicBlock> post = CFG.getDomPostOrder(function);
        Collections.reverse(post);

        for (BasicBlock block : post) {
            instructions.addAll(block.getInstrList());
        }
        for (Instruction instruction : instructions) {
            scheduleEarly(instruction, function);
        }
        visited.clear();
        Collections.reverse(instructions);
        for (Instruction instruction : instructions) {
            scheduleLate(instruction);
        }

    }


    public static void scheduleEarly(Instruction instr, Function currentFunction) {
        if (visited.contains(instr) || instr.isPinned()) {
            return;
        }
        visited.add(instr);
        instr.liteRemoveFromList();
        instr.insertBefore(currentFunction.getFirstBlk().getLastInstr());

        for (Value op : instr.getUsedList()) {
            if (op instanceof Instruction opInstr) {
                if (instr.getParentBB().getDomLV() < opInstr.getParentBB().getDomLV()) {
                    instr.liteRemoveFromList();
                    instr.insertBefore(opInstr.getParentBB().getLastInstr());
                }
            }
        }

    }


    public static void scheduleLate(Instruction instr) {
        if (visited.contains(instr) || instr.isPinned()) {
            return;
        }
        visited.add(instr);
        BasicBlock lca = null;
        for (Value userValue : instr.getUserList()) {
            if (userValue instanceof Instruction user) {
                scheduleLate(user);
                BasicBlock bb;
                if (user instanceof PhiInstr phiNode) {
                    HashSet<BasicBlock> bbset = phiNode.getKeyByValueMulti(instr);
                    if (bbset.size() == 1) {
                        bb = bbset.iterator().next();
                    } else {
                        BasicBlock pHeader = null;
                        for (BasicBlock b : bbset) {
                            pHeader = findLCA(b, pHeader);
                        }
                        bb = pHeader;
                    }
                } else {
                    bb = user.getParentBB();
                }
                lca = findLCA(lca, bb);
                if (lca == null) {
                    throw new RuntimeException("lca is null");
                }
            }
        }

        if (!instr.getUserList().isEmpty()) {
            BasicBlock best = lca;
            int bestLoopDepth = best.getLoopDepth();
            int bestDomLV = best.getDomLV();
            while (lca != instr.getParentBB()) {
                lca = lca.getIDom();
                if (lca == null) {
                    System.out.println(instr);
                    throw new RuntimeException("lca is null");
                }
                if (globalStrategy == STRATEGY.DOMEARLY) {
                    if (lca.getLoopDepth() < bestLoopDepth) {
                        best = lca;
                        bestLoopDepth = best.getLoopDepth();
                        bestDomLV = best.getDomLV();
                    } else if (lca.getLoopDepth() == bestLoopDepth && lca.getDomLV() < bestDomLV) {
                        best = lca;
                        bestDomLV = best.getDomLV();
                    }
                } else {
                    if (lca.getLoopDepth() < bestLoopDepth ||
                            (lca.getLoopDepth() == bestLoopDepth && lca.getDomLV() > bestDomLV)) {
                        best = lca;
                        bestLoopDepth = best.getLoopDepth();
                        bestDomLV = best.getDomLV();
                    }
                }

            }
            instr.liteRemoveFromList();
            instr.insertBefore(best.getLastInstr());
        }

        BasicBlock best = instr.getParentBB();
        for (Instruction i : best.getInstrList()) {
            if (i != instr) {
                if (!(i instanceof PhiInstr) && i.getUsedList().contains(instr)) {
                    instr.liteRemoveFromList();
                    instr.insertBefore(i);
                    break;
                }
            }
        }

    }

    private static BasicBlock findLCA(BasicBlock A, BasicBlock B) {
        if (A == null) {
            return B;
        }
        if (B == null) {
            return A;
        }
        while (A.getDomLV() < B.getDomLV()) {
            B = B.getIDom();
        }
        while (B.getDomLV() < A.getDomLV()) {
            A = A.getIDom();
        }
        while (A != B) {
            if (A.getIDom() == null) {
                System.out.println(A);
                return null;
            }
            A = A.getIDom();
            B = B.getIDom();
        }
        return A;
    }
}
