package midend;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LCSSA;

import java.util.*;

/**
 * 计划在 GVN 的同时做常数传播
 */
public class GVN {
    private static final HashMap<String, HashSet<Instruction>> valueClass = new HashMap<>();
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            valueClass.clear();
            BasicBlock headBlk = function.getFirstBlk();
//            dfsIdoms(headBlk, new HashMap<>());
            dfsIdomsRPO(headBlk, new HashMap<>());
            hoistSchedule();
            sinkSchedule();
        }
    }

    //fixme : 用RPO和DFS有区别吗 - 如果不做GCM的话似乎没有
//    private static void dfsIdoms(BasicBlock basicBlock, Map<String, Instruction> instrMap) {
//        Instruction ins = basicBlock.getFirstInstr();
//        while (ins != null) {
//            if (instrMap.containsKey(ins.myHash())) {
//                Instruction already = instrMap.get(ins.myHash());
//                if (already != ins) {
//                    ins.replaceUseWith(already);
//                    ins.removeFromList();
//                }
//            } else {
//                instrMap.put(ins.myHash(), ins);
//
//            }
//            ins = (Instruction) ins.getNext();
//        }
//
//        for (BasicBlock next : basicBlock.getIDomSet()) {
//            dfsIdoms(next, new HashMap<>(instrMap));
//        }
//    }

    private static void dfsIdomsRPO(BasicBlock basicBlock, Map<String, Instruction> instrMap) {
        List<String> addedKeys = new ArrayList<>();  // 记录本块新增 key，用于退出时清理

        Instruction ins = basicBlock.getFirstInstr();
        while (ins != null) {
            String hash = ins.myHash();
            if (instrMap.containsKey(hash)) {
                Instruction already = instrMap.get(hash);
                if (already != ins) {
                    ins.replaceUseWith(already);
                    ins.removeFromList();
                }
            } else {
                instrMap.put(hash, ins);
                addedKeys.add(hash);
                if (valueClass.containsKey(hash)) {
                    valueClass.get(hash).add(ins);
                } else {
                    HashSet<Instruction> set = new HashSet<>();
                    set.add(ins);
                    valueClass.put(hash, set);
                }
            }
            ins = (Instruction) ins.getNext();
        }

        for (BasicBlock next : basicBlock.getIDomSet()) {
            dfsIdomsRPO(next, instrMap);
        }

        for (String key : addedKeys) {
            instrMap.remove(key);
        }
    }

    private static void hoistSchedule() {
        for (String key : valueClass.keySet()) {
            HashSet<Instruction> set = valueClass.get(key);
            if (set.size() > 1) {
                Instruction first = set.iterator().next();
                BasicBlock bb = first.getParentBB().getIDom();
                boolean canHoist = true;
                for (Instruction ins : set) {
                    if (ins.getParentBB().getIDom() != bb) {
                        canHoist = false;
                        break;
                    }
                }
                for (Value op : first.getUsedList()) {
                    if (!(op instanceof Instruction ins && bb.getDomSet().contains(ins.getParentBB()))) {
                        canHoist = false;
                        break;
                    }
                }
                if (canHoist) {
                    Iterator<Instruction> it = set.iterator();
                    first = it.next();
                    first.liteRemoveFromList();
                    first.insertBefore(bb.getLastInstr());
                    while (it.hasNext()) {
                        Instruction ins = it.next();
                        ins.replaceUseWith(first);
                        ins.removeFromList();
                    }
                }
            }
        }
    }

    private static void sinkSchedule() {
        for (String key : valueClass.keySet()) {
            HashSet<Instruction> set = valueClass.get(key);
            if (set.size() > 1) {
                ArrayList<Set<BasicBlock>> dfSets = new ArrayList<>();
                for (Instruction ins : set) {
                    dfSets.add(ins.getParentBB().getDominanceFrontier());
                }
                Set<BasicBlock> commDF = LCSSA.intersectAll(dfSets);
                if (commDF.size() != 1) {
                    continue;
                }
                BasicBlock commBlk = commDF.iterator().next();
                boolean canSink = true;
                for (Instruction ins : set) {
                    if (canSink) {
                        for (Value user : ins.getUserList()) {
                            if (user instanceof PhiInstr) {
                                for (Value op : ((PhiInstr) user).getOperandMap().values()) {
                                    if (!(set.contains(op))) {
                                        canSink = false;
                                        break;
                                    }
                                }
                            } else if (user instanceof Instruction in) {
                                if (!commBlk.getDomSet().contains(in.getParentBB())) {
                                    canSink = false;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (canSink) {
                    Iterator<Instruction> it = set.iterator();
                    Instruction first = it.next();
                    first.liteRemoveFromList();
                    first.insertBefore(commBlk.getLastInstr());
                    while (it.hasNext()) {
                        Instruction ins = it.next();
                        List<Value> users = new ArrayList<>(ins.getUserList());
                        ins.replaceUseWith(first);
                        for (Value user : users) {
                            if (user instanceof PhiInstr) {
                                ((PhiInstr) user).simplify();
                            }
                        }
                        ins.removeFromList();
                    }
                }
            }
        }
    }
}
