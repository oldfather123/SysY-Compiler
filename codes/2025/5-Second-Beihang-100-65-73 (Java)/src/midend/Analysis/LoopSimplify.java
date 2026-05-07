// 化简Loop为LLVM要求的Simplify格式
// pre-header唯一
// 回边唯一
// 从循环出去到达的块的前继节点都来源于循环当中
// 追加：循环的header不能是另一个循环的pre-header
package midend.Analysis;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import java.util.*;

public class LoopSimplify {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            for (LoopInfo loop : function.getAncientLoopInfo()) {
                processExit(loop);
            }
            for (LoopInfo loop : function.getAncientLoopInfo()) {
                processLatch(loop);
            }
            for (LoopInfo loop : function.getAncientLoopInfo()) {
                processPreHeader(loop);
            }
        }
    }

    public static void executePreHeaderOnly(List<Function> functions) {
        for (Function function : functions) {
            for (LoopInfo loop : function.getAncientLoopInfo()) {
                processPreHeader(loop);
            }
        }
    }

    // 做了块融合会出现这样的情况
    public static void processPreHeader(LoopInfo loop) {
        for(LoopInfo subLoop : loop.getChildLoop()) {
            processPreHeader(subLoop);
        }

        HashSet<BasicBlock> enterFroms = loop.getEnterFroms();
        if (enterFroms.isEmpty()) {
            throw new RuntimeException("There are no enterFroms in the loop");
        }
        if (enterFroms.size() == 1) {
            loop.setPreHeader(enterFroms.iterator().next());
        } else {
            BasicBlock preHeader = new BasicBlock(loop.getFunction().getAndAddBlkIdx());
            BasicBlock header = loop.getLoopHeader();
            // insert before header
            preHeader.insertBefore(header);
            preHeader.setParentFunc(header.getParentFunc());

            // set pre / suc
            //replace会处理enterings（原）、header与preheader的关系
            // set enter from to preHeader
            for (BasicBlock enterFrom : enterFroms) {
                enterFrom.replaceJumpTarget(header, preHeader);
            }
            // set jump to header
            new JumpInstr(header, preHeader);
            // set phi
            for (Instruction instr : header.getInstrList()) {
                HashMap<BasicBlock, Value> valueMap = new HashMap<>();
                if (instr instanceof PhiInstr phi) {
                    PhiInstr newPhi = new PhiInstr(phi.getType(),
                            header.getParentFunc().getAndAddRegIdx(), preHeader);
                    for (BasicBlock enterFrom : enterFroms) {
                        valueMap.put(enterFrom, phi.getOperandMap().get(enterFrom));
                        phi.delOpPair(enterFrom);
                    }
                    phi.addOperand(preHeader, newPhi);
                    for (Map.Entry<BasicBlock, Value> entry : valueMap.entrySet()) {
                        newPhi.addOperand(entry.getKey(), entry.getValue());
                    }
                    newPhi.insertBefore(preHeader.getFirstInstr());
                    newPhi.simplify();
                } else {
                    break;
                }
            }
            // set loop info
            loop.setPreHeader(preHeader);
            for (LoopInfo exitOfLoop : new HashSet<>(header.isExitOflLoop)) {
                preHeader.isExitOflLoop.add(exitOfLoop);
                header.isExitOflLoop.remove(exitOfLoop);
                exitOfLoop.getExitTargets().remove(header);
                exitOfLoop.getExitTargets().add(preHeader);
            }
            if (loop.getParentLoop() != null) {
                LoopInfo parentLoop = loop.getParentLoop();
                preHeader.setLoopBelonged(parentLoop);
                parentLoop.addBodyBlock(preHeader);
            }
        }
    }

    public static void processLatch(LoopInfo loop) {
        for(LoopInfo subLoop : loop.getChildLoop()) {
            processLatch(subLoop);
        }

        if (loop.getLatchBlocks().size() > 1) {
            BasicBlock sumLatch = new BasicBlock(loop.getFunction().getAndAddBlkIdx());
            // insert after original first latch
            BasicBlock firstLatch = loop.getLatchBlocks().iterator().next();
            BasicBlock header = loop.getLoopHeader();
            sumLatch.insertAfter(firstLatch);
            sumLatch.setParentFunc(firstLatch.getParentFunc());
            // set pre / suc & set latch to sumLatch
            for (BasicBlock latch : loop.getLatchBlocks()) {
                latch.replaceJumpTarget(header, sumLatch);
            }
            // set sumLatch to header
            new JumpInstr(header, sumLatch);
            // set phi
            for (Instruction instr : header.getInstrList()) {
                HashMap<BasicBlock, Value> valueMap = new HashMap<>();
                if (instr instanceof PhiInstr phi) {
                    PhiInstr newPhi = new PhiInstr(phi.getType(),
                            header.getParentFunc().getAndAddRegIdx(), sumLatch);
                    newPhi.insertBefore(sumLatch.getFirstInstr());
                    for (BasicBlock latch : loop.getLatchBlocks()) {
                        valueMap.put(latch, phi.getOperandMap().get(latch));
                        phi.delOpPair(latch);
                    }
                    phi.addOperand(sumLatch, newPhi);
                    for (Map.Entry<BasicBlock, Value> entry : valueMap.entrySet()) {
                        newPhi.addOperand(entry.getKey(), entry.getValue());
                    }
                    newPhi.simplify();
                } else {
                    break;
                }
            }

            // set loop info
            loop.getLatchBlocks().clear();
            loop.getLatchBlocks().add(sumLatch);
            sumLatch.setLoopBelonged(loop);
            loop.getBodyBlocks().add(sumLatch);
        }

    }

    public static void processExit(LoopInfo loop) {
        for(LoopInfo subLoop : loop.getChildLoop()) {
            processExit(subLoop);
        }

        HashSet<BasicBlock> notDomedExits = new HashSet<>();
        Set<BasicBlock> headerDomSet = loop.getLoopHeader().getDomSet();
        for (BasicBlock exit : loop.getExitTargets()) {
            if (!headerDomSet.contains(exit)) {
                notDomedExits.add(exit);
            } else if (exit.getLoopBelonged() != null && exit.getLoopBelonged().getLoopHeader() == exit) {
                if (!loop.isParentOfThis(exit.getLoopBelonged())) {
                    notDomedExits.add(exit);
                }
            }
        }

        BasicBlock header = loop.getLoopHeader();

        for (BasicBlock exit : notDomedExits) {
            BasicBlock newExit = new BasicBlock(loop.getFunction().getAndAddBlkIdx());

            // insert after header
            newExit.insertAfter(header);
            newExit.setParentFunc(header.getParentFunc());
            // set newExit to exit
            new JumpInstr(exit, newExit);
            for (BasicBlock exiting : loop.getExiting()) {
                if (exiting.getSucs().contains(exit)) {
                    exiting.replaceJumpTarget(exit, newExit);
                }
            }

            for (Instruction instr : exit.getInstrList()) {
                if (instr instanceof PhiInstr phi) {
                    PhiInstr newPhi = new PhiInstr(phi.getType(),
                            header.getParentFunc().getAndAddRegIdx(), newExit);
                    newPhi.insertBefore(newExit.getLastInstr());
                    for (BasicBlock exiting : loop.getExiting()) {
                        if (phi.getOperandMap().get(exiting) != null) {
                            newPhi.addOperand(exiting, phi.getOperandMap().get(exiting));
                            phi.delOpPair(exiting);
                        }
                    }
                    phi.addOperand(newExit, newPhi);
                } else {
                    break;
                }
            }


            // set loop info
            loop.getExitTargets().remove(exit);
            loop.getExitTargets().add(newExit);
            newExit.isExitOflLoop.add(loop);
            exit.isExitOflLoop.remove(loop);
            if (loop.getParentLoop() != null) {
                LoopInfo parentLoop = loop.getParentLoop();
                newExit.setLoopBelonged(parentLoop);
                parentLoop.addBodyBlock(newExit);
            }
            if (exit.getLoopBelonged() != null && exit.getLoopBelonged().getLoopHeader() == exit) {
                if (loop.isParentOfThis(exit.getLoopBelonged())) {
                    LoopInfo parentLoop = loop.getParentLoop();
                    for (BasicBlock exiting : loop.getExiting()) {
                        if (exiting.getSucs().contains(newExit)) {
                            parentLoop.getLatchBlocks().remove(exiting);
                        }
                    }
                    parentLoop.getLatchBlocks().add(newExit);
                } else {
                    for (BasicBlock originalPre : newExit.getPres()) {
                        exit.getLoopBelonged().getEnterFroms().remove(originalPre);
                    }
                    exit.getLoopBelonged().getEnterFroms().add(newExit);
                }

            }
        }

    }
}
