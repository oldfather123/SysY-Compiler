package ir.pass.analyze;

import ir.Value;
import ir.instr.BinaryInstr;
import ir.instr.Br;
import ir.instr.GetElementPtrInst;
import ir.instr.Instr;
import ir.instr.Load;
import ir.instr.Phi;
import ir.instr.Store;
import ir.type.IntType;
import ir.value.Arg;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.User;
import tools.BlockAnalyzer;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.Stack;

public class LoopAnalyzer {
    private static ArrayList<Loop> currentLoops;
    private static LinkedHashMap<BasicBlock, Boolean> visited = new LinkedHashMap<>();

    //必须先运行domAnalyzer，获得支配树
    public static void analyzeLoop(Module module) {
        for (var func : module.functions.values()) {
            func.loops.clear();
        }
        for (var func : module.functions.values()) {
            for (var block : func.blocks) {
                block.setLoopDepth(0);
                block.loop = null;
            }
        }

        for (var func : module.functions.values()) {
            currentLoops = func.loops;
            if (func.blocks.size() == 0) {
                continue;
            }

            visit(func.blocks.get(0));
            for (Loop loop : currentLoops) {
                loop.updateLevel();
            }
        }
    }

    public static void analyzeLoopWithTimes(Module module) {
        analyzeLoop(module);
        LinkedHashSet<Loop> visited = new LinkedHashSet<>();
        for (Loop currentLoop : currentLoops) {
            analyzeLoopTime(currentLoop, visited);
        }
    }

    private static void analyzeLoopTime(Loop loop, LinkedHashSet<Loop> visited) {
        if (visited.contains(loop)) {
            return;
        }
        if (loop.father != null && !visited.contains(loop.father)) {
            analyzeLoopTime(loop.father, visited);
        }
        long time = loop.father != null ? loop.father.times : 1;
        loop.times = time*10;
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
                return;
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
                return;
            }
            //检查所有比较运算数来源是否可知
            LinkedHashSet<Value> needKnown = new LinkedHashSet<Value>();
            LinkedHashSet<Value> waitCheck = new LinkedHashSet<Value>();
            Br keyBr = (Br) head.getInsts().get(head.getInsts().size() - 1);
            assert (keyBr.getOperands().size() > 1);
            if (!(keyBr.getOP(0) instanceof ConstNumber)) {
                needKnown.add(keyBr.getOP(0));
            }

            while (!needKnown.isEmpty()) {
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
            LinkedHashSet<Value> keyInstrs = new LinkedHashSet<>(waitCheck);
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
                if (!removedCheck.isEmpty()) {
                    same = false;
                    for (var knowed : removedCheck) {
                        waitCheck.remove(knowed);
                    }
                }
                if (same) {
                    break;
                }
            }
            if (!waitCheck.isEmpty()) {
                return;
            }
            ArrayList<Value> allisnts = new ArrayList<>();
            for (Value inst : head.getInsts()) {
                if(inst instanceof Br || keyInstrs.contains(inst)){
                    allisnts.add(inst);
                }
            }
            for (var block : loop.blocks) {
                if (!block.equals(head)) {
                    for (Value inst : block.getInsts()) {
                        if(inst instanceof Br || keyInstrs.contains(inst)){
                            allisnts.add(inst);
                        }
                    }
                }
            }
            boolean unrollSuccess = false;// TODO: 2023/8/10 标记loop防止多次尝试展开
            boolean firstLoop = true;
            int thistime = 0;
            do {
                LinkedHashMap<Value, Value> phiParr = new LinkedHashMap<>();
                for (var insvalue : allisnts) {
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
                            null);
                        sameBir.cmpType = ((BinaryInstr) instr).cmpType;
                        if (allConst && uses.get(0).type instanceof IntType) {
                            var constRes = sameBir.computeConstResult();
                            valuemap.put(instr, constRes);
                            sameBir.delete();
                        } else {
                            assert false;
                        }
                    }
                    if (instr instanceof GetElementPtrInst) {
                        assert false;
                    }
                    if (instr instanceof Load) {
                        assert false;
                    }
                    if (instr instanceof Store) {
                        assert false;
                    }
                    if (instr instanceof Br) {
                        if (instr.getOperands().size() > 1) {
                            Value cond = instr.getOP(0);

                            assert valuemap.containsKey(cond);
                            assert valuemap.get(cond) instanceof ConstNumber;
                            if (((ConstNumber) valuemap.get(cond)).value.equals(0)) {
                                loop.times = thistime*time;
                                return;
                            }else{
                                thistime++;
                            }
                        }
                    }
                }
                firstLoop = false;
            } while (true);
        }
    }

    private static void visit(BasicBlock block) {
        for (var subb : block.getIdominate()) {
            visit(subb);
        }
        Stack<BasicBlock> keyEdges = new Stack<>();
        for (var pred : block.getPreds()) {
            if (block.getDominate().contains(pred)) {
                keyEdges.add(pred);
            }
        }
        if (keyEdges.size() == 0) {
            return;
        }
        Loop loop = new Loop();
        currentLoops.add(loop);
        visited = new LinkedHashMap<>();
        visited.put(block, true);
        block.loop = loop;
        loop.blocks.add(block);
        while (keyEdges.size() > 0) {
            var tmp = keyEdges.pop();
            if (visited.containsKey(tmp)) {
                continue;
            }
            visited.put(tmp, true);
            if (tmp.loop == null) {
                for (var pre : tmp.getPreds()) {
                    if (visited.containsKey(pre)) {
                        continue;
                    }
                    keyEdges.add(pre);
                }
                tmp.loop = loop;
                loop.blocks.add(tmp);
            } else {
                for (var pre : tmp.getPreds()) {
                    if (visited.containsKey(pre) || tmp.loop.blocks.contains(pre)) {
                        continue;
                    }
                    keyEdges.add(pre);
                }
                tmp.loop.father = loop;
                loop.children.add(tmp.loop);
            }
        }
    }
}
