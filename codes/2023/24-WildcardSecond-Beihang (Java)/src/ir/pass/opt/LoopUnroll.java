package ir.pass.opt;

import ir.Value;
import ir.instr.BinaryInstr;
import ir.instr.Br;
import ir.instr.GetElementPtrInst;
import ir.instr.Instr;
import ir.instr.Load;
import ir.instr.Phi;
import ir.instr.Store;
import ir.pass.analyze.DomAnalyzer;
import ir.pass.analyze.Loop;
import ir.pass.analyze.LoopAnalyzer;
import ir.type.IntType;
import ir.value.Arg;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.User;
import ir.value.user.Function;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;

public class LoopUnroll {
    public static boolean needRepeat;

    public static boolean run(Module module) {
        needRepeat = false;
        DomAnalyzer.calDomForModule(module);
        LoopAnalyzer.analyzeLoop(module);
        for (var func : module.functions.values()) {
            loopUnroll(func);
        }
        return needRepeat;
    }

    private static void loopUnroll(Function func) {
        ArrayList<Loop> removedLoops = new ArrayList<>();
        for (var loop : func.loops) {
            if (!removedLoops.isEmpty()) {
                break;
            }
            if (loop.children.isEmpty() && loop.blocks.size() == 2) {
                LinkedHashMap<Value, Value> valuemap = new LinkedHashMap<>();
                Boolean canUnroll = true;
                BasicBlock head = ((Br) loop.blocks.get(0).getInsts().get(
                    loop.blocks.get(0).getInsts().size() - 1
                )).getOperands().size() > 1 ? loop.blocks.get(0) : loop.blocks.get(1);
                //检查phi的所有来源都在循环内部可知
                for (var value : head.getInsts()) {
                    if (value instanceof Phi) {
                        if (((Phi) value).values.size() > 2) {
                            canUnroll = false;
                            break;
                        }
                        for (var comeBlock : ((Phi) value).values.keySet()) {
                            if (!(loop.blocks.contains(comeBlock))) {
                                valuemap.put(value,
                                    ((Phi) value).values.get(comeBlock));
                            }
                        }
                    }
                }
                if (!canUnroll) {
                    continue;
                }
                //检查所有指令都是br、Binary、Phi、Store、Load、GetElementPtr指令
                for (var block : loop.blocks) {
                    for (var inst : block.getInsts()) {
                        if (inst instanceof Phi) {
                            continue;
                        }
                        if (inst instanceof Br) {
                            continue;
                        }
                        if (inst instanceof GetElementPtrInst) {
                            continue;
                        }
                        if (inst instanceof Store) {
                            continue;
                        }
                        if (inst instanceof Load) {
                            continue;
                        }
                        if (!(inst instanceof BinaryInstr)) {
                            canUnroll = false;
                            break;
                        }
                    }
                    if (!canUnroll) {
                        break;
                    }
                }
                if (!canUnroll) {
                    continue;
                }
                //检查所有比较运算数来源是否可知
                LinkedHashSet<Value> needKnown = new LinkedHashSet<Value>();
                LinkedHashSet<Value> waitCheck = new LinkedHashSet<Value>();
                Br keyBr = (Br) head.getInsts().get(head.getInsts().size() - 1);
                assert (keyBr.getOperands().size() > 1);
                if (!(keyBr.getOP(0) instanceof ConstNumber)) {
                    needKnown.add(keyBr.getOP(0));
                }

                while (needKnown.size() != 0) {
                    var tmp = needKnown.iterator().next();
                    needKnown.remove(tmp);
                    waitCheck.add(tmp);
                    if (tmp instanceof Arg) {
                        continue;
                    }
                    for (var ope : ((Instr) tmp).getOperands()) {
                        if (ope instanceof ConstNumber) {
                            continue;
                        }
                        if (!waitCheck.contains(ope)) {
                            needKnown.add(ope);
                        }
                    }
                }
                boolean same = true;
                while (!waitCheck.isEmpty()) {
                    same = true;
                    ArrayList<Value> removedCheck = new ArrayList<Value>();
                    for (var checker : waitCheck) {
                        if (valuemap.containsKey(checker)) {
                            removedCheck.add(checker);
                            continue;
                        }
                        boolean known = true;
                        if (!(checker instanceof Arg)) {
                            for (var op : ((Instr) checker).getOperands()) {
                                if (!((valuemap.containsKey(op) &&
                                    valuemap.get(op) instanceof ConstNumber)
                                    || op instanceof ConstNumber)) {
                                    known = false;
                                    break;
                                }
                            }
                        } else {
                            known = false;
                        }
                        if (known) {
                            removedCheck.add(checker);
                        }
                    }
                    if (removedCheck.size() > 0) {
                        same = false;
                        for (var knowed : removedCheck) {
                            waitCheck.remove(knowed);
                        }
                    }
                    if (same) {
                        break;
                    }
                }
                if (waitCheck.size() > 0) {
                    continue;
                }
                //检查除phi外是否所有指令都不会被循环外的value用到
//                for (var block : loop.blocks) {
//                    for (var inst : block.getInsts()) {
//                        if (inst instanceof Phi) {
//                            continue;
//                        }
//                        for (var user : inst.users) {
//                            if (!valuemap.containsKey(user)) {
//                                canUnroll = false;
//                            }
//                        }
//                        if (!canUnroll) {
//                            break;
//                        }
//                    }
//                    if (!canUnroll) {
//                        break;
//                    }
//                }
//                if (!canUnroll) {
//                    continue;
//                }
//                while (true) {
//
//                }
                BasicBlock targetBlock = new BasicBlock("unroll", func, true);
                ArrayList<Value> allisnts = new ArrayList<>();
                allisnts.addAll(head.getInsts());
                for (var block : loop.blocks) {
                    if (!block.equals(head)) {
                        allisnts.addAll(block.getInsts());
                    }
                }
                int hold = 0;
                boolean unrollSuccess = false;// TODO: 2023/8/10 标记loop防止多次尝试展开
                boolean firstLoop = true;
                while (hold < 10000) {
                    LinkedHashMap<Value, Value> phiParr = new LinkedHashMap<>();
                    for (var insvalue : allisnts) {
                        hold++;
                        var instr = (Instr) insvalue;
                        if (instr instanceof Phi && !(firstLoop)) {
                            for (var comeBlock : ((Phi) instr).values.keySet()) {
                                if (loop.blocks.contains(comeBlock)) {
                                    assert
                                        valuemap.containsKey(((Phi) instr).values.get(comeBlock));
                                    phiParr.put(instr,
                                        valuemap.get(((Phi) instr).values.get(comeBlock)));
                                }
                            }
                        } else {
                            if (!phiParr.isEmpty()) {
                                for (var key : phiParr.keySet()) {
                                    valuemap.put(key, phiParr.get(key));
                                }
                            }
                            phiParr.clear();
                        }
                        if (instr instanceof BinaryInstr) {
                            LinkedList<Value> uses = new LinkedList<Value>();
                            for (var ope : instr.getOperands()) {
                                uses.add(valuemap.getOrDefault(ope, ope));
                            }
                            boolean allConst = true;
                            for (var ope : uses) {
                                if (!(ope instanceof ConstNumber)) {
                                    allConst = false;
                                    break;
                                }
                            }
                            BinaryInstr sameBir = new BinaryInstr(uses, instr.type, instr.name,
                                ((BinaryInstr) instr).instType,
                                targetBlock);
                            sameBir.cmpType = ((BinaryInstr) instr).cmpType;
                            if (allConst && uses.get(0).type instanceof IntType) {
                                var constRes = sameBir.computeConstResult();
                                valuemap.put(instr, constRes);
                                sameBir.delete();
                            } else {
                                targetBlock.addValue(sameBir);
                                valuemap.put(instr, sameBir);
                            }
                        }
                        if (instr instanceof GetElementPtrInst) {
                            LinkedList<Value> uses = new LinkedList<>();
                            for (var ope : instr.getOperands()) {
                                uses.add(valuemap.getOrDefault(ope, ope));
                            }
                            GetElementPtrInst newGetelementPtr =
                                new GetElementPtrInst(uses, instr.type, instr.name,
                                    targetBlock);
                            targetBlock.addValue(newGetelementPtr);
                            valuemap.put(instr, newGetelementPtr);
                        }
                        if (instr instanceof Load) {
                            LinkedList<Value> uses = new LinkedList<>();
                            for (var ope : instr.getOperands()) {
                                uses.add(valuemap.getOrDefault(ope, ope));
                            }
                            Load newLoad = new Load(uses, instr.name, targetBlock);
                            targetBlock.addValue(newLoad);
                            valuemap.put(instr, newLoad);
                        }
                        if (instr instanceof Store) {
                            Value dest = valuemap.getOrDefault(((Store) instr).getDest(),
                                ((Store) instr).getDest());
                            Value val = valuemap.getOrDefault(((Store) instr).getVal(), ((Store) instr).getVal());
                            Store newStore = new Store(dest, val, targetBlock);
                            targetBlock.addValue(newStore);
                        }
                        if (instr instanceof Br) {
                            if (instr.getOperands().size() > 1) {
                                Value cond = instr.getOP(0);

                                assert valuemap.containsKey(cond);
                                assert valuemap.get(cond) instanceof ConstNumber;


                                if (((ConstNumber) valuemap.get(cond)).value.equals(0)) {
                                    unrollSuccess = true;
                                    head.replaceAllUsers(targetBlock);
                                    for (var oriValue : valuemap.keySet()) {
                                        oriValue.replaceAllUsers(valuemap.get(oriValue));
                                        ((User) oriValue).delete();
                                    }
                                    targetBlock.addValue(
                                        new Br(new LinkedList<>(
                                            Collections.singletonList(instr.getOP(2))),
                                            targetBlock)
                                    );
                                    func.insertBefore(head, targetBlock);
                                    for (var block : loop.blocks) {
                                        func.blocks.remove(block);
                                    }
                                    removedLoops.add(loop);
                                    break;
                                }
                            }
                        }
                    }
                    firstLoop = false;
                    if (unrollSuccess) {
                        break;
                    }
                }
                if (!unrollSuccess) {
                    for (var inst : targetBlock.getInsts()) {
                        ((Instr) inst).delete();
                    }
                }
            }
        }
        func.loops.removeAll(removedLoops);
        if (removedLoops.size() > 0) {
            needRepeat = true;
        }
    }
}
