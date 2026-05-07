package midend.loop;

import Utils.CustomList;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;
import midend.SSA.DFG;
import midend.SSA.RemoveUseLessPhi;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class LoopRotate {
    public static void execute(ArrayList<Function> functions) {
        AnalysisLoop.execute(functions);
        for (Function function : functions) {
            for (Loop loop : function.getOuterLoop()) {
                dfs4rotate(loop);
//                for (Loop in : loop.getInnerLoops()) {
//                    loop4doWhile(in);
//                }
            }
        }
        AnalysisLoop.execute(functions);
    }

    private static void dfs4rotate(Loop loop) {
        for (Loop in : loop.getInnerLoops()) {
            dfs4rotate(in);
        }
        loop4doWhile(loop);
    }

    private static void loop4doWhile(Loop loop) {
//        loop.LoopPrint();
        BasicBlock header = loop.getHeader();
        BasicBlock exiting;
        if (loop.getExitings().size() == 1) {
            exiting = loop.getExitings().get(0);
        } else {
            return;
        }
        /*
         *   anon:
         *    1.header = cond1
         *    2.从header到exiting，全部克隆到latch的后面
         *    3.放在bbs中最后的那个latch块之后
         *    4.
         *
         * */
        Procedure procedure = (Procedure) header.getParent().getOwner();
        BasicBlock latch = header;
//        while (loop.getBlks().contains((BasicBlock) latch.getNext())) {
//            latch = (BasicBlock)latch.getNext();
//        }
        if (loop.getLatchs().size() == 1) {
            latch = loop.getLatchs().get(0);
        } else {
            return;
        }
        if (!loop.getLatchs().contains(latch)) {
//            loop.LoopPrint();
            throw new RuntimeException("latch: " + latch);
        }
        BasicBlock body = (BasicBlock) exiting.getNext();
        BasicBlock newBlk = new BasicBlock(body.getLoopDepth(), procedure.getAndAddBlkIndex());
        HashMap<Value, Value> old2new = new HashMap<>();

        /*
         * anon:将条件块中的phi变到body中
         *      先把值存起来
         * */
        HashMap<PhiInstr, Value> phiInHeader = new HashMap<>();
        CustomList.Node phi = header.getInstructions().getHead();
        while (phi instanceof PhiInstr) {
            Value value;
            for (BasicBlock pre : header.getPres()) {
                int index = ((PhiInstr) phi).getPrtBlks().indexOf(pre);
                value = ((PhiInstr) phi).getValues().get(index);
                phiInHeader.put((PhiInstr) phi, value);
            }
            phi = phi.getNext();
        }


        BasicBlock newLatch = clone4cond(header, body, latch, newBlk, old2new, procedure, loop);
        //修改cond块中对于phi的使用
        BasicBlock tmp = header;
        HashSet<BasicBlock> condBlks = new HashSet<>();
        while (tmp != body) {
            condBlks.add(tmp);
            loop.getBlks().remove(tmp);
            tmp = (BasicBlock) tmp.getNext();
        }

        for (PhiInstr phiInstr : phiInHeader.keySet()) {
            phiInstr.replaceUseInRange(condBlks, phiInHeader.get(phiInstr));
        }

        exiting.getEndInstr().removeFromList();
        exiting.getEndInstr().getParentBB().setRet(false);
        exiting.addInstruction(new JumpInstr(body));
//        System.out.println(latch.getEndInstr().print());
        latch.getEndInstr().modifyUse(header, newBlk);

//        int phiCnt = procedure.getPhiIndex();
        for (PhiInstr ins : phiInHeader.keySet()) {
            ArrayList<Value> values = new ArrayList<>();
            ArrayList<BasicBlock> prtBlks = new ArrayList<>();
            for (BasicBlock pre : body.getPres()) {
                Value phiValue = null;
                if (pre.equals(header)) {
                    for (BasicBlock blk : header.getPres()) {
                        phiValue = ins.getValues().get(ins.getPrtBlks().indexOf(blk));
                    }
                } else if (pre.equals(newBlk)) {
                    for (BasicBlock blk : newBlk.getPres()) {
                        PhiInstr newIns = (PhiInstr) old2new.get(ins);
                        phiValue = newIns.getValues().get(newIns.getPrtBlks().indexOf(blk));
                    }
                }
                if (phiValue == null) {
                    throw new RuntimeException("manMadePhi: body: " + body + " pre: " + pre + " header: " + header + " newBlk: " + newBlk);
                }
                values.add(phiValue);
                prtBlks.add(pre);
            }
            PhiInstr newPhi = new PhiInstr(procedure.getAndAddPhiIndex(), values.get(0).getDataType(), values, prtBlks);
            body.addInstrToHead(newPhi);
            ins.replaceUseTo(newPhi);
        }

        /*anon：所有在body的出现的phi，克隆一份去exit*/
        ArrayList<PhiInstr> phiInBody = new ArrayList<>();
        CustomList.Node node = body.getInstructions().getHead();
        while (node instanceof PhiInstr) {
            phiInBody.add((PhiInstr) node);
            node = node.getNext();
        }
        if (loop.getExits().size() != 1) {
            throw new RuntimeException("one exiting and more exits?");
        }
        tmp = body;
        HashSet<BasicBlock> loopBody = new HashSet<>();
        while (tmp != newLatch.getNext()) {
            loopBody.add(tmp);
            tmp = (BasicBlock) tmp.getNext();
        }
        BasicBlock exit = loop.getExits().get(0);
        for (PhiInstr phiInstr : phiInBody) {
            PhiInstr newPhi = (PhiInstr) phiInstr.cloneShell(procedure);
            phiInstr.replaceUseTo(newPhi);
            newPhi.replaceUseInRange(loopBody, phiInstr);
            exit.addInstrToHead(newPhi);
        }
        RemoveUseLessPhi.clear4branchModify(header);
        RemoveUseLessPhi.clear4branchModify(newBlk);
        DFG.execute4func(procedure.getParentFunc());
        AnalysisLoop.execute4func(procedure.getParentFunc());
        LCSSA.execute4func(procedure.getParentFunc());
    }

    /*  anon: 从cond1到stopBlk的前一个  */
    private static BasicBlock clone4cond(BasicBlock cond1Blk, BasicBlock stopBlk, BasicBlock latch, BasicBlock cond2Blk,
                                         HashMap<Value, Value> old2new, Procedure procedure, Loop loop) {
        BasicBlock curBB = cond1Blk;
        ArrayList<BasicBlock> newBlks = new ArrayList<>();
        while (curBB != stopBlk) {
            BasicBlock newBB = curBB == cond1Blk ? cond2Blk : new BasicBlock(curBB.getLoopDepth(), procedure.getAndAddBlkIndex());
            old2new.put(curBB, newBB);
            newBlks.add(newBB);

            Instruction curIns = (Instruction) curBB.getInstructions().getHead();
            while (curIns != null) {
                Instruction newIns = curIns.cloneShell(procedure);
                newBB.addInstruction(newIns);
                old2new.put(curIns, newIns);
                curIns = (Instruction) curIns.getNext();
            }

            curBB = (BasicBlock) curBB.getNext();
        }

        BasicBlock last = latch;

        for (BasicBlock newBB : newBlks) {
            Instruction newIns = (Instruction) newBB.getInstructions().getHead();
            while (newIns != null) {
                if (newIns instanceof PhiInstr) {
                    ((PhiInstr) newIns).renewBlocks(old2new);
                }

                ArrayList<Value> usedValues = new ArrayList<>(newIns.getUseValueList());
                for (Value toReplace : usedValues) {
                    if (!old2new.containsKey(toReplace)) {
                        if (newIns instanceof ReturnInstr && toReplace == newBB) {
                            continue;
                        }
//                        if (!(toReplace instanceof ConstValue) && !(toReplace instanceof GlobalObject)) {
//                            throw new RuntimeException("使用了未曾设想的 value " + toReplace.toString());
//                        }
                    } else {
                        newIns.modifyUse(toReplace, old2new.get(toReplace));
                    }
                }
                newIns = (Instruction) newIns.getNext();
            }
            loop.addBlk(newBB);
            newBB.insertAfter(last);
            last = newBB;
        }
        return last;
    }
}
