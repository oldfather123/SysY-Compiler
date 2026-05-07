package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class LCSSA {
    private final HashMap<BasicBlock, Phi> exitToPhi = new HashMap<>();

    public void optimize() {
        // 从外层开始进行Lcssa，否则可能出问题
        ArrayList<Loop> loops = Optimizer.instance().getLoopAnalyze().bfsLoops();
        loops.forEach(this::formLCSSA);
    }

    private void formLCSSA(Loop loop) {
        // 选择需要用Phi代换的的指令
        HashSet<Instruction> openInstrs = new HashSet<>();
        HashSet<BasicBlock> blocksInLoop = loop.getBlocksInLoop();
        for (BasicBlock block : blocksInLoop) {
            for (Instruction instruction : block.getInstructionList()) {
                for (User user : instruction.getUserList()) {
                    if (!blocksInLoop.contains(user.getBlock()) &&
                            !(loop.getExits().contains(user.getBlock()) && user instanceof Phi)) {
                        openInstrs.add(instruction);
                        break;
                    }
                }
            }
        }

        // 对这些指令进行替换
        for (Instruction instruction : openInstrs) {
            exitToPhi.clear();
            insertPhiFor(instruction, loop);
            rename(instruction, loop);
        }
    }

    private void insertPhiFor(Instruction instr, Loop loop) {
        // 为所有exit插入Phi
        HashSet<BasicBlock> exits = loop.getExits();
        BasicBlock sourceBlock = instr.getBlock();
        for (BasicBlock exit : exits) {
            Phi phi = new Phi(instr.getType(), IRManager.getInstance().declareTempVar());
            for (BasicBlock parent : Optimizer.instance().getCFG().getParents(exit)) {
                if (Optimizer.instance().getDominAnalyzer().getDominer(parent).contains(sourceBlock)) {
                    phi.addCond(instr, parent);
                } else {
                    phi.addCond(new Value("0", ValueType.NULL), parent);
                }
            }
            exit.addInstrAtEntry(phi);
            exitToPhi.put(exit, phi);
        }
    }

    private void rename(Instruction instr, Loop loop) {
        HashSet<BasicBlock> blocksInLoop = loop.getBlocksInLoop();
        ArrayList<User> users = new ArrayList<>(instr.getUserList());
        for (User user : users) {
            if (!(user instanceof Instruction insUser)) {
                continue;
            }
            BasicBlock userBlock = user.getBlock();
            if (!blocksInLoop.contains(userBlock) &&
                    !(loop.getExits().contains(userBlock) && user instanceof Phi)) {
                if (insUser instanceof Phi phi) {
                    ArrayList<BasicBlock> sourceBlocks = phi.getSrcBlockWhen(instr);
                    sourceBlocks.removeAll(loop.getBlocksInLoop());
                    for (BasicBlock src : sourceBlocks) {
                        phi.replaceValueFrom(src,
                                getValueFrom(src, loop, insUser.getType(), new HashMap<>()));
                    }
                } else {
                    user.replaceOperand(instr,
                            getValueFrom(user.getBlock(), loop, insUser.getType(), new HashMap<>()));
                }
            }
        }
    }

    private Phi getValueFrom(BasicBlock userBlock, Loop loop, ValueType vType,
                             HashMap<BasicBlock, Phi> blockToPhi) {
        HashSet<BasicBlock> exits = new HashSet<>(loop.getExits());
        HashSet<BasicBlock> dominers = Optimizer.instance().getDominAnalyzer().getDominer(userBlock);
        exits.retainAll(dominers);
        if (exits.size() > 0) {
            for (BasicBlock dominExit : exits) {
                Phi phi = exitToPhi.get(dominExit);
                if (!(phi.isConst() && phi.getOperandList().get(0).getType() == ValueType.NULL)) {
                    blockToPhi.put(userBlock, phi);
                    return phi;
                }
            }
        }

        if (blockToPhi.containsKey(userBlock)) {
            return blockToPhi.get(userBlock);
        }

        // 否则，考虑所有前驱块
        Phi phi = new Phi(vType, IRManager.getInstance().declareTempVar());
        userBlock.addInstrAtEntry(phi);
        blockToPhi.put(userBlock, phi);
        HashSet<BasicBlock> parents = Optimizer.instance().getCFG().getParents(userBlock);
//        if (parents == null) {
//            return
//        }
        for (BasicBlock parent : parents) {
            if (parent.equals(userBlock)) {
                phi.addCond(phi, parent);
            } else {
                phi.addCond(getValueFrom(parent, loop, vType, blockToPhi), parent);
            }
        }
        return phi;
    }
}
