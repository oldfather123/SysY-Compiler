package ir.pass.opt;

import ir.Value;
import ir.instr.BinaryInstr;
import ir.instr.Br;
import ir.instr.Call;
import ir.instr.InstType;
import ir.instr.Instr;
import ir.instr.Phi;
import ir.pass.analyze.DomAnalyzer;
import ir.pass.analyze.Loop;
import ir.pass.analyze.LoopAnalyzer;
import ir.pass.analyze.RemoveUselessUser;
import ir.type.IntType;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.user.Function;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedList;

//对于程序设计中常见的循环累加并取模的模式进行优化。
public class LoopCompute {
    static boolean changed = false;

    static int phiCount = 0;

    private static void initPhiCount() {
        phiCount = 0;
    }

    private static void addPhiCount() {
        phiCount++;
    }

    public static boolean run(Module module) {
        changed = false;
        RemoveUselessUser.run(module);
        DomAnalyzer.calDomForModule(module);
        LoopAnalyzer.analyzeLoop(module);
        module.functions.values().forEach(LoopCompute::runForFunction);
        return changed;
    }

    private static boolean loopContains(Loop loop, Value value) {
        for (BasicBlock block : loop.blocks) {
            if (block.getInsts().contains(value)) {
                return true;
            }
        }
        return false;
    }

    private static void runForFunction(Function function) {
        ArrayList<Loop> loops = new ArrayList<Loop>(function.loops);
        for (Loop loop : loops) {
            if (loop.blocks.size() > 2 || !loop.children.isEmpty()) {
                continue;
            }
            initPhiCount();
            loop.blocks.forEach(block -> block.getInsts().forEach(
                i -> {
                    if (i instanceof Phi) {
                        addPhiCount();
                    }
                }
            ));
            if (phiCount != 2) {
                continue;
            }
            boolean canAnalyze = true;
            LinkedHashMap<Value, Value> valuemap = new LinkedHashMap<>();
            BasicBlock head = loop.blocks.get(0);
            if (head.getPreds().size() != 2) {
                continue;
            }
            for (var value : head.getInsts()) {
                if (value instanceof Phi) {
                    if (((Phi) value).values.size() > 2) {
                        canAnalyze = false;
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
            if (!canAnalyze) {
                continue;
            }
            Br keyBr = (Br) head.getInsts().get(head.getInsts().size() - 1);
            BasicBlock exitBlock = null;
            for (Value operand : keyBr.getOperands()) {
                if (operand instanceof BasicBlock && !(loop.blocks.contains(operand))) {
                    exitBlock = (BasicBlock) operand;
                }
            }
            if (keyBr.getOperands().size() != 3) {
                continue;
            }
            var cond = keyBr.getOP(0);
            if (!(cond instanceof BinaryInstr)) {
                continue;
            }
            if (!(((BinaryInstr) cond).instType == InstType.BinaryType.icmp &&
                ((BinaryInstr) cond).cmpType ==
                    InstType.CmpType.lt)) {
                continue;
            }
            Phi keyphi = null;
            Value loopNumber = null;
            for (var ope : ((BinaryInstr) cond).getOperands()) {
                if (!(valuemap.containsKey(ope) || !(loop.blocks.get(0).getInsts().contains(ope) ||
                    loop.blocks.get(1).getInsts().contains(ope)))) {
                    canAnalyze = false;
                    break;
                }
                if (valuemap.containsKey(ope)) {
                    assert ope instanceof Phi;
                    if (keyphi == null) {
                        keyphi = (Phi) ope;
                    } else {
                        canAnalyze = false;
                        break;
                    }
                    for (var comeblock : ((Phi) ope).values.keySet()) {
                        if (((Phi) ope).values.get(comeblock) instanceof ConstNumber) {
                            continue;
                        }
                        if (((Phi) ope).values.get(comeblock) instanceof BinaryInstr binaryInstr) {
                            if (!binaryInstr.getOperands().contains(ope)) {
                                canAnalyze = false;
                                break;
                            }
                            if (binaryInstr.instType != InstType.BinaryType.add) {
                                canAnalyze = false;
                                break;
                            }
                            var tmp = new ArrayList<Value>(binaryInstr.getOperands());
                            tmp.remove(ope);
                            for (var subope : tmp) {
                                if (!(subope instanceof ConstNumber)) {
                                    canAnalyze = false;
                                    break;
                                } else {
                                    if (((Integer) ((ConstNumber) subope).value) != 1) {
                                        canAnalyze = false;
                                        break;
                                    }
                                }
                            }
                        } else {
                            canAnalyze = false;
                            break;
                        }
                        if (!canAnalyze) {
                            break;
                        }
                    }
                } else {
                    loopNumber = ope;
                }
                if (!canAnalyze) {
                    break;
                }
            }
            if (!canAnalyze) {
                continue;
            }
            Phi valuePhi = null;
            for (var value : head.getInsts()) {
                if (value instanceof Phi) {
                    if (value != keyphi) {
                        valuePhi = (Phi) value;
                    }
                }
            }
            Value oriValue = null;
            Value recreValue = null;
            assert valuePhi != null;
            for (var comeblock : valuePhi.values.keySet()) {
                if (!loop.blocks.contains(comeblock)) {
                    oriValue = valuePhi.values.get(comeblock);
                } else {
                    recreValue = valuePhi.values.get(comeblock);
                }
            }
            if (!(recreValue instanceof BinaryInstr)) {
                continue;
            }
            int instnumber = 0;
            for(var block : loop.blocks){
                instnumber += block.getInsts().size();
            }
            // TODO: 2023/8/19 肯定有bug，万一次数是负数就寄了
            if (((BinaryInstr) recreValue).instType == InstType.BinaryType.add) {

                if(instnumber != 7){
                    continue;
                }
                if (!((BinaryInstr) recreValue).getOperands().contains(valuePhi)) {
                    continue;
                }
                Value loopAddConst = null;
                for (Value operand : ((BinaryInstr) recreValue).getOperands()) {
                    if (operand.equals(valuePhi)) {
                        continue;
                    }
                    if (!loopContains(loop, operand)) {
                        loopAddConst = operand;
                    } else {
                        canAnalyze = false;
                        break;
                    }
                }
                if (!canAnalyze) {
                    continue;
                }
                BasicBlock newBlock = new BasicBlock("newblock", function, true);
                var mul = new BinaryInstr(new LinkedList<>(Arrays.asList(loopNumber, loopAddConst)),
                    new IntType(), "tmp", InstType.BinaryType.mul, newBlock);
                newBlock.addValue(mul);
                var add = new BinaryInstr(new LinkedList<>(Arrays.asList(mul, oriValue)),
                    new IntType(),"tmp1", InstType.BinaryType.add, newBlock);
                newBlock.addValue(add);
                valuePhi.replaceAllUsers(add);
                loop.blocks.forEach(block -> block.getInsts().forEach(inst->((Instr)inst).delete()));
                head.replaceAllUsers(newBlock);
                newBlock.addValue(new Br(new LinkedList<>(Collections.singletonList(exitBlock)), newBlock));
                function.insertBefore(head, newBlock);
                function.blocks.removeAll(loop.blocks);
                changed = true;
            } else if (((BinaryInstr) recreValue).instType == InstType.BinaryType.srem) {
                if(instnumber!=8){
                    continue;
                }
                ConstNumber modNumber = null;
                if(((BinaryInstr) recreValue).getOP(1) instanceof ConstNumber){
                    modNumber = (ConstNumber) ((BinaryInstr) recreValue).getOP(1);
                }else{
                    continue;
                }
                if((Integer)(modNumber.value)<0){
                    continue;
                }
                long mod1min = ((long)(int)(Integer)(modNumber.value));
                if(loopNumber instanceof ConstNumber){
                    if(loopNumber.getUpperBound()<0){
                        continue;
                    }
                    mod1min = Math.min(mod1min, loopNumber.getUpperBound());
                }
                if(!(((BinaryInstr) recreValue).getOP(0)instanceof BinaryInstr)){
                    continue;
                }
                BinaryInstr secondInstr = (BinaryInstr) ((BinaryInstr) recreValue).getOP(0);
                if(!(secondInstr.instType == InstType.BinaryType.add)){
                    continue;
                }
                if(!secondInstr.getOperands().contains(valuePhi)){
                    continue;
                }
                Value loopAddConst = null;
                for (Value operand : ((BinaryInstr) secondInstr).getOperands()) {
                    if (operand.equals(valuePhi)) {
                        continue;
                    }
                    if (!loopContains(loop, operand)) {
                        loopAddConst = operand;
                    } else {
                        canAnalyze = false;
                        break;
                    }
                }
                long mod2min = (long)(int)(Integer)(modNumber.value);
                if(loopAddConst instanceof ConstNumber){
                    if(loopAddConst.getUpperBound()<0){
                        continue;
                    }
                    mod2min = Math.min(mod2min, loopAddConst.getUpperBound());
                }
                if(mod1min*mod2min>Integer.MAX_VALUE){
                    continue;
                }
                BasicBlock newBlock = new BasicBlock("newblock", function, true);
                var mod1 = new BinaryInstr(new LinkedList<>(Arrays.asList(loopNumber, modNumber)),
                    new IntType(), "mod1", InstType.BinaryType.srem,newBlock);
                newBlock.addValue(mod1);
                var mod2 = new BinaryInstr(new LinkedList<>(Arrays.asList(loopAddConst, modNumber)),
                    new IntType(), "mod2", InstType.BinaryType.srem, newBlock);
                newBlock.addValue(mod2);
                var mul = new BinaryInstr(new LinkedList<>(Arrays.asList(mod1, mod2)),
                    new IntType(), "tmp", InstType.BinaryType.mul, newBlock);
                newBlock.addValue(mul);
                var mod3 = new BinaryInstr(new LinkedList<>(Arrays.asList(mul, modNumber)),
                    new IntType(), "mod3", InstType.BinaryType.srem, newBlock);
                newBlock.addValue(mod3);
                BinaryInstr add = null;
                BinaryInstr mod4 = null;
                if(!(oriValue instanceof ConstNumber && ((ConstNumber) oriValue).value.equals(0))){
                    add = new BinaryInstr(new LinkedList<>(Arrays.asList(mod3, oriValue)),
                        new IntType(),"tmp1", InstType.BinaryType.add, newBlock);
                    newBlock.addValue(add);
                    mod4 = new BinaryInstr(new LinkedList<>(Arrays.asList(add==null ? mod3:add, modNumber)),
                        new IntType(), "mod4", InstType.BinaryType.srem, newBlock);
                    newBlock.addValue(mod4);
                }
                valuePhi.replaceAllUsers(mod4==null?mod3:mod4);
                loop.blocks.forEach(block -> block.getInsts().forEach(inst->((Instr)inst).delete()));
                head.replaceAllUsers(newBlock);
                newBlock.addValue(new Br(new LinkedList<>(Collections.singletonList(exitBlock)), newBlock));
                function.insertBefore(head, newBlock);
                function.blocks.removeAll(loop.blocks);
                changed = true;
            }
        }
    }
}