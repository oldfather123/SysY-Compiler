package midend.DCE;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.CFG;

import java.util.ArrayList;
import java.util.List;


/**
 * 合并基本块
 * 依赖：ConstFold需要把分支折完，不然可能出问题  副作用：破坏CFG，破坏LoopInfo
 * PASS1: BRANCH MERGE
 * PASS2: DEL EMPTY BLOCK
 * PASS3: MERGE BLOCK
 * PASS4: BRANCH HOIST 强化剪枝
 * 建议：跑HOIST模式前要跑一下DOMEARLY模式的GCM，提高剪枝效率
 * 规避：不要在循环前面使用HOIST模式，否则循环结构可能变复杂导致无法优化
 */

public class BlockMerge {
    public enum MERGE_STRATEGY {
        NORMAL,
        HOIST,
    }


    public static void execute(List<Function> functions, MERGE_STRATEGY strategy) {
        for (Function function : functions) {
            fullLevelMerge(function, strategy);
        }
    }

    public static void executeForRange(List<Function> functions, MERGE_STRATEGY strategy) {
        for (Function function : functions) {
            mergeBlocksSimple(function, strategy);
        }
    }

    private static void fullLevelMerge(Function function, MERGE_STRATEGY strategy) {
        mergeExtraBranch(function);
        delEmptyBlocks(function, strategy);
        mergeBlocksSimple(function, strategy);
        if (strategy == MERGE_STRATEGY.HOIST) {
            CFG.executeOnFunc(function);
            branchHoist(function);
        }
    }


    /**
     * 理论上来讲不应该有A-> ->B，但是以防万一还是加上了
     */
    private static void mergeExtraBranch(Function function) {
        for (BasicBlock basicBlock : function.getBasicBlockList()) {
            if (basicBlock.getLastInstr() instanceof BranchInstr br) {
                br.simplify();
            }
        }
    }

    private static void delEmptyBlocks(Function function, MERGE_STRATEGY strategy) {
        for (BasicBlock block : function.getBasicBlockList()) {
            if (block.getInstrNum() == 1 && block.getSucs().size() == 1
                    && function.getFirstBlk() != block) {
                if (strategy == MERGE_STRATEGY.NORMAL) {
                    if (block.sisterLoopHeader != null) {
                        continue;
                    }
                }
                BasicBlock suc = block.getSucs().getFirst();
                List<BasicBlock> pres = block.getPres();
                boolean canDel = true;
                for (BasicBlock pre : pres) {
                    if (pre.getSucs().contains(suc)) {
                        canDel = false;
                        break;
                    }
                }
                if (!canDel) {
                    continue;
                }
                for (Instruction ins : suc.getInstrList()) {
                    if (ins instanceof PhiInstr phi) {
                        Value val = phi.getOperandMap().get(block);
                        phi.delOpPair(block);
                        for (BasicBlock pre : pres) {
                            phi.addOperand(pre, val);
                        }
                    }
                }

                for (BasicBlock pre : new ArrayList<>(pres)) {
                    pre.replaceJumpTarget(block, suc);
                }
                block.removeFromList();
            }
        }
    }

    private static void mergeBlocksSimple(Function function, MERGE_STRATEGY strategy) {
        for (BasicBlock block : function.getBasicBlockList()) {
            if (block.getLastInstr() instanceof ReturnInstr) {
                continue;
            }
            if (strategy == MERGE_STRATEGY.NORMAL) {
                if (block.sisterLoopHeader != null) {
                    continue;
                }
            }
            BasicBlock front = null;
            if (block.getPres().size() == 1) {
                front = block.getPres().getFirst();
            }
            if (front != null && front.getSucs().size() == 1) {
                BasicBlock behind = front.getSucs().getFirst();
                if (behind == block) {
                    mergeBlocks(front, block);
                }
            }
        }
    }

    private static void mergeBlocks(BasicBlock front, BasicBlock back) {
        Instruction jump = front.getLastInstr();
        for (Instruction mergedInstr : back.getInstrList()) {
            mergedInstr.liteRemoveFromList();
            mergedInstr.insertAfter(front.getLastInstr());
            if (mergedInstr instanceof Terminator t) {
                t.getUserList().remove(back);
                front.setUse(t);
                for (BasicBlock suc : back.getSucs()) {
                    suc.getPres().remove(back);
                    suc.getPres().add(front);
                    front.getSucs().add(suc);
                }
            }
        }
        if (!(jump instanceof JumpInstr)) {
            throw new RuntimeException("不应该出现条件跳转");
        } else {
            jump.forceRemoveFromList();
            jump.getParentBB().close();
        }
        back.replaceUseWith(front);
        back.removeFromList();
    }

    private static void branchHoist(Function function) {
        for (BasicBlock block : function.getBasicBlockList()) {
            if (block.getSucs().size() == 1 &&
                    block.getSucs().getFirst().getInstrNum() == 1 &&
                    block.getSucs().getFirst().getLastInstr() instanceof BranchInstr br) {

                if (br.getCondition() instanceof Instruction ins && !ins.getParentBB().getDomSet().contains(block)) {
                    continue;
                }

                BasicBlock toJump = block.getSucs().getFirst();
                block.getLastInstr().forceRemoveFromList();
                new BranchInstr(br.getCondition(), br.getThenBlk(), br.getElseBlk(), block);

                for (BasicBlock suc : toJump.getSucs()) {
                    for (Instruction instruction : suc.getInstrList()) {
                        if (instruction instanceof PhiInstr phi) {
                            phi.addOperand(block, phi.getOperandMap().get(toJump));
                        } else {
                            break;
                        }
                    }
                }

                for (BasicBlock suc : toJump.getSucs()) {
                    toJump.getDomSet().remove(suc);
                }
            }
        }
    }

}
