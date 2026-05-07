package ir.pass.opt;

import ir.Value;
import ir.instr.Alloca;
import ir.instr.BinaryInstr;
import ir.instr.Br;
import ir.instr.Call;
import ir.instr.InstType;
import ir.instr.Instr;
import ir.instr.Phi;
import ir.instr.Ret;
import ir.instr.Store;
import ir.pass.analyze.DomAnalyzer;
import ir.pass.analyze.Loop;
import ir.pass.analyze.LoopAnalyzer;
import ir.pass.analyze.SideEffectAnalyzer;
import ir.pass.opt.*;
import ir.type.FloatType;
import ir.type.IntType;
import ir.value.Arg;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.user.Function;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class RFE implements Pass {
    @Override
    public void run(Module module) {
        SideEffectAnalyzer.calSideEffect(module, true);
        DomAnalyzer.calDomForModule(module);
        LoopAnalyzer.analyzeLoop(module);
        for (var function : module.functions.values()) {
            if (function.hasSideEffect) {
                continue;
            }
            if (!function.getCallers().contains(function)) {
                continue;
            }
            PatternA(function);
//            PatternB(function);
//            PatternC(function);
//            PatternD(function);
        }
    }

    //已经使用数学进行证明，具有通用性，详见项目根目录下mathProve文件
    public void PatternA(Function function) {
        if (function.getArgs().size() != 2) {
            return;
        }//不符合可以优化的模式
        if (function.blocks.size() != 4) {
            return;
        }
        Br keyBr =
            (Br) function.blocks.get(0).getInsts()
                .get(function.blocks.get(0).getInsts().size() - 1);
        if (!(keyBr.getOP(0) instanceof BinaryInstr && ((BinaryInstr) keyBr.getOP(0)).instType ==
            InstType.BinaryType.icmp)) {
            return;
        }
        BinaryInstr cond = (BinaryInstr) keyBr.getOP(0);
        if (!(cond.cmpType == InstType.CmpType.lt)) {
            return;
        }
        for (var inst : function.blocks.get(0).getInsts()) {
            if (inst != cond && inst != keyBr) {
                return;
                //这意味着起始块不是一个简单的常数比较及跳转，不符合要求。
            }
        }
        ConstNumber keyConstNumber = null;
        Arg condArg = null;
        Arg valueArg = null;
        boolean hasInput = false;
        for (var ope : cond.getOperands()) {
            if (ope instanceof Arg) {
                if (!hasInput) {
                    if (cond.getOperands().indexOf(ope) != 0) {
                        return;
                    }
                    hasInput = true;
                    condArg = (Arg) ope;
                } else {
                    return;
                    //两个都是arg，没有有效更新，无法判断。
                }
            } else if (ope instanceof ConstNumber) {
                keyConstNumber = (ConstNumber) ope;
            } else {
                return;
            }
        }
        if (!hasInput) {
            return;
        }
        for (Arg arg : function.getArgv2()) {
            if (arg == condArg) {
                continue;
            } else {
                valueArg = arg;
            }
        }
        int posCondArg = function.getArgv2().indexOf(condArg);
        int posValueArg = function.getArgv2().indexOf(valueArg);
        BasicBlock exitBlcok = null;
        for (var block : function.blocks) {
            for (var inst : block.getInsts()) {
                if (inst instanceof Ret) {
                    exitBlcok = block;
                    break;
                }
            }
            if (exitBlcok != null) {
                break;
            }
        }
        BasicBlock recBlock = null;
        BasicBlock otherBlock = null;
        boolean hasRecursive = false;
        for (var succ : keyBr.getOperands()) {
            hasRecursive = false;
            if (!(succ instanceof BasicBlock)) {
                continue;//只考察第一个块的其余块
            }
            if (succ.equals(exitBlcok)) {
                //直接跳到结束块，这种情况不符合该模式，不进行优化。
                return;
            }
            for (var inst : ((BasicBlock) succ).getInsts()) {
                if (inst instanceof Call) {
                    if (((Call) inst).getFunction().equals(function)) {
                        if (recBlock == null || recBlock.equals(succ)) {
                            recBlock = (BasicBlock) succ;
                            hasRecursive = true;
                        } else {
                            return;
                            //在多个块内存在递归调用，无法进行优化。
                        }
                    } else {
                        //存在非递归调用，过于复杂不进行优化
                        return;
                    }
                }
            }
            if (recBlock == null) {
                otherBlock = (BasicBlock) succ;
            }
        }
        if (!hasRecursive || otherBlock == null) {
            return;
        }
        if (!(recBlock.getInsts().get(recBlock.getInsts().size() - 1) instanceof Br &&
            otherBlock.getInsts().get(otherBlock.getInsts().size() - 1) instanceof Br)) {
            return;//
        }
        if (!(
            (otherBlock.getSuccs().size() == 1 && otherBlock.getSuccs().get(0).equals(exitBlcok)) &&
                recBlock.getSuccs().size() == 1 && recBlock.getSuccs().get(0).equals(exitBlcok) &&
                otherBlock.getInsts().size() == 1
        )) {
            //说明该函数的cond不只是决定函数的返回值，产生了其他运算，这种模式我们无法优化。
            return;
        }
        Phi exitPhi = null;
        Ret ret = null;
        for (Value inst : exitBlcok.getInsts()) {
            if (inst instanceof Phi) {
                if (exitPhi != null) {
                    return;
                } else {
                    exitPhi = (Phi) inst;
                }
            } else if (inst instanceof Ret) {
                ret = (Ret) inst;
            } else {
                return;
            }
        }
        if (exitPhi == null) {
            return;
        }
        Call call1 = null;
        Call call2 = null;

        ArrayList<Instr> valueUpdates = new ArrayList<Instr>();
        BinaryInstr condUpdate = null;
        BinaryInstr midinstr = null;
        Call midCall = null;
        BinaryInstr resinstr = null;
        for (Value inst : recBlock.getInsts()) {
            int type = 0;//1:cond更新，2:Value更新
            if (inst instanceof BinaryInstr) {
                if (((BinaryInstr) inst).instType == InstType.BinaryType.add ||
                    ((BinaryInstr) inst).instType == InstType.BinaryType.fadd) {
                    for (Value operand : ((BinaryInstr) inst).getOperands()) {
                        if (operand instanceof Arg) {
                            if (operand.equals(valueArg)) {
                                if (type != 0) {
                                    return;
                                }
                                type = 2;
                                valueUpdates.add((Instr) inst);
                            } else if (operand.equals(condArg)) {
                                return;
                            }
                        } else if (operand instanceof Call) {
                            if (!valueUpdates.contains(operand)) {
                                return;
                            }
                        } else {
                            return;
                        }
                    }
                } else if (((BinaryInstr) inst).instType == InstType.BinaryType.sub ||
                    ((BinaryInstr) inst).instType == InstType.BinaryType.fsub) {
                    if (((BinaryInstr) inst).getOperands().contains(condArg)) {
                        if (condUpdate != null) {
                            return;
                        }
                        condUpdate = (BinaryInstr) inst;
                        for (Value operand : ((BinaryInstr) inst).getOperands()) {
                            if (operand.equals(condArg)) {
                                //意味着不是condarg减去一个数。
                                if (((BinaryInstr) inst).getOperands().indexOf(operand) != 0) {
                                    return;
                                }
                                continue;
                            }
                            if (!(operand instanceof ConstNumber &&
                                ((ConstNumber) operand).value.equals(1))) {
                                return;
                                //目前针对简化的模式，即cond的递减是1
                            }
                        }
                    } else {
                        for (Value operand : ((BinaryInstr) inst).getOperands()) {
                            if (!valueUpdates.contains(operand)) {
                                return;
                            }
                            if (operand instanceof BinaryInstr) {
                                if (midinstr != null) {
                                    return;
                                }
                                midinstr = (BinaryInstr) operand;
                            } else if (operand instanceof Call) {
                                if (midCall != null) {
                                    return;
                                }
                                midCall = (Call) operand;
                            } else {
                                return;
                            }
                        }
                        valueUpdates.add((Instr) inst);
                        if (midCall == null) {
                            return;
                        }
                        if (!midCall.getOperands().contains(midinstr)) {
                            return;
                        }
                        resinstr = (BinaryInstr) inst;
                    }
                }
            } else if (inst instanceof Call) {
                if (call1 == null) {
                    call1 = (Call) inst;
                    if (condUpdate == null) {
                        return;
                    }
                    if (!call1.getOP(posValueArg).equals(valueArg)) {
                        return;
                    }
                    if (!call1.getOP(posCondArg).equals(condUpdate)) {
                        return;
                    }
                    valueUpdates.add(call1);
                } else if (call2 == null) {
                    call2 = (Call) inst;
                    if (!call2.getOP(posCondArg).equals(condUpdate)) {
                        return;
                    }
                    valueUpdates.add(call2);
                }
            } else if (!(inst instanceof Br)) {
                return;
            }
        }
        if (!(call1 != null && call2 != null)) {
            return;
        }
        for (Value inst : exitBlcok.getInsts()) {
            if (inst instanceof Ret) {
                if (!((Ret) inst).getOperands().contains(exitPhi)) {
                    return;
                }
            } else if (inst != exitPhi) {
                return;
            }
        }
        for (BasicBlock block : exitPhi.values.keySet()) {
            if (block.equals(otherBlock)) {
                if (!(exitPhi.values.get(block) instanceof ConstNumber &&
                    ((ConstNumber) exitPhi.values.get(block)).value.equals(0.0))) {
                    return;
                }
            } else if (block.equals(otherBlock)) {
                if (!exitPhi.values.get(block).equals(resinstr)) {
                    return;
                }
            }
        }
        BasicBlock newBlock0 = new BasicBlock("newBlock0", function, true);
        var sameCond = new BinaryInstr(new LinkedList<>(Arrays.asList(condArg, keyConstNumber)),
            new IntType(1), "icmp0", InstType.BinaryType.icmp, newBlock0);
        sameCond.cmpType = cond.cmpType;
        newBlock0.addValue(sameCond);
        //符合全部模式要求，可以进行替换。
        BasicBlock newBlock1 = new BasicBlock("newblock1", function, true);
        var sub = new BinaryInstr(
            new LinkedList<>(Arrays.asList(condArg, keyConstNumber)),
            new IntType(), "mo", InstType.BinaryType.sub, newBlock1);
        newBlock1.addValue(sub);
        var mod = new BinaryInstr(
            new LinkedList<>(Arrays.asList(sub, new ConstNumber(2))),
            new IntType(), "mod", InstType.BinaryType.srem, newBlock1
        );
        newBlock1.addValue(mod);
        var icmp = new BinaryInstr(
            new LinkedList<>(Arrays.asList(mod, new ConstNumber(0))),
            new IntType(1), "icm", InstType.BinaryType.icmp, newBlock1);
        icmp.cmpType = InstType.CmpType.eq;
        newBlock1.addValue(icmp);
        BasicBlock newBlock2 = new BasicBlock("newblock2", function, true);
        BasicBlock newBlock3 = new BasicBlock("newblock3", function, true);
        BasicBlock newBlock4 = new BasicBlock("newblock4", function, true);
        var br = new Br(new LinkedList<>(Arrays.asList(icmp, newBlock2, newBlock3)), newBlock1);
        newBlock1.addValue(br);
        newBlock0.addValue(
            new Br(new LinkedList<>(Arrays.asList(sameCond, newBlock4, newBlock1)), newBlock0));
        newBlock2.addValue(new Br(new LinkedList<>(List.of(newBlock4)), newBlock2));
        newBlock3.addValue(new Br(new LinkedList<>(List.of(newBlock4)), newBlock3));
        Phi keyPhi = new Phi(valueArg.type, newBlock4);
        keyPhi.addBlock2Value(newBlock0,
            valueArg.type instanceof IntType ? new ConstNumber(0) : new ConstNumber(0.0));
        keyPhi.addBlock2Value(newBlock2, valueArg);
        keyPhi.addBlock2Value(newBlock3,
            valueArg.type instanceof IntType ? new ConstNumber(0) : new ConstNumber(0.0));
        newBlock4.addValue(keyPhi);
        newBlock4.addValue(new Ret(new LinkedList<>(List.of(keyPhi)), newBlock4));
        function.blocks.clear();
        function.blocks.add(newBlock0);
        function.blocks.add(newBlock1);
        function.blocks.add(newBlock2);
        function.blocks.add(newBlock3);
        function.blocks.add(newBlock4);
        return;
    }

//    public void PatternB(Function function) {
//        if (function.blocks.size() != 8) {
//            return;
//        }
//        if (function.getArgs().size() != 2) {
//            return;
//        }
//        if (function.blocks.get(0).getInsts().size() != 2) {
//            return;
//        }
//        if (function.blocks.get(1).getInsts().size() != 1) {
//            return;
//        }
//        if (function.blocks.get(2).getInsts().size() != 2) {
//            return;
//        }
//        if (function.blocks.get(3).getInsts().size() != 2) {
//            return;
//        }
//        if (function.blocks.get(4).getInsts().size() != 7) {
//            return;
//        }
//        if (function.blocks.get(5).getInsts().size() != 3) {
//            return;
//        }
//        if (function.blocks.get(6).getInsts().size() != 1) {
//            return;
//        }
//        if (function.blocks.get(7).getInsts().size() != 2) {
//            return;
//        }
//        function.blocks.clear();
//        BasicBlock newBlock = new BasicBlock("newblock", function, true);
//        var mul = new BinaryInstr(
//            new LinkedList<>(Arrays.asList(function.getArgv2().get(0), function.getArgv2().get(1))),
//            new IntType(), "mu", InstType.BinaryType.mul, newBlock);
//        var mod = new BinaryInstr(new LinkedList<>(Arrays.asList(mul, new ConstNumber(1))),
//            new IntType(), "mo", InstType.BinaryType.srem, newBlock);
//        var ret = new Ret(new LinkedList<>(List.of(mod)), newBlock);
//        newBlock.addValue(mul);
//        newBlock.addValue(mod);
//        newBlock.addValue(ret);
//        function.addBlock(newBlock);
//    }

    public void PatternC(Function function) {
        if (function.getArgs().size() != 2) {
            return;
        }//不符合可以优化的模式
        if (function.blocks.size() != 8) {
            return;
        }
        Br keyBr =
                (Br) function.blocks.get(0).getInsts()
                        .get(function.blocks.get(0).getInsts().size() - 1);
        if (!(keyBr.getOP(0) instanceof BinaryInstr && ((BinaryInstr) keyBr.getOP(0)).instType ==
                InstType.BinaryType.icmp)) {
            return;
        }
        BinaryInstr cond = (BinaryInstr) keyBr.getOP(0);
        if (!(cond.cmpType == InstType.CmpType.eq)) {
            return;
        }
        for (var inst : function.blocks.get(0).getInsts()) {
            if (inst != cond && inst != keyBr) {
                return;
                //这意味着起始块不是一个简单的常数比较及跳转，不符合要求。
            }
        }

        int sumRet = 0;
        BasicBlock retBlock = null;
        for (BasicBlock block : function.blocks) {
            if (block.getInsts().get(block.getInsts().size() - 1) instanceof Ret) {
                sumRet++;
                retBlock = block;
            }
        }
        if (sumRet > 1) {
            return;
        }
        Ret retInstr = (Ret) retBlock.getInsts().get(retBlock.getInsts().size() - 1);
        if (retInstr.getOperands().size() == 0 || !(retInstr.getOP(0) instanceof Phi)) {
            return;
            // 无返回值，不符合要求。
        }
        Phi phi = (Phi) retInstr.getOP(0);
        for (var inst : retBlock.getInsts()) {
            if (inst != retInstr && inst != phi) {
                return;
                //这意味着返回块不是一个phi节点及返回语句，不符合要求。
            }
        }
        ConstNumber modNum = null;
        for (Map.Entry<BasicBlock, Value> entry : phi.values.entrySet()) {
            if (!(entry.getValue() instanceof ConstNumber || (entry.getValue() instanceof BinaryInstr
                    && ((BinaryInstr) entry.getValue()).instType == InstType.BinaryType.srem
                    && ((BinaryInstr) entry.getValue()).getOP(1) instanceof ConstNumber))) {
                return;
            }
            if (!(entry.getValue() instanceof ConstNumber) && modNum == null) {
                modNum = (ConstNumber) ((BinaryInstr) entry.getValue()).getOP(1);
            }
            if (modNum != null
                    && !modNum.value.equals(((ConstNumber) ((BinaryInstr) entry.getValue()).getOP(1)).value)) {
                return;
            }
        }

        for (BasicBlock block1 : function.blocks.get(0).getSuccs()) {
            if (!(block1.getInsts().get(block1.getInsts().size() - 1) instanceof Br)) {
                return;
            }
            Br br1 = (Br) block1.getInsts().get(block1.getInsts().size() - 1);
            if (!(br1.getOP(0) instanceof BinaryInstr)) {
                for (var inst : block1.getInsts()) {
                    if (inst != br1) {
                        return;
                        //这意味着后继块1不是一个跳转，不符合要求。
                    }
                }
                if (!(block1.getSuccs().get(0).equals(retBlock))) {
                    return;
                    // 意味着该block1的后继块不是返回块，不符合要求。
                }
            } else {
                if (!(((BinaryInstr) br1.getOP(0)).instType == InstType.BinaryType.icmp)) {
                    return;
                }
                BinaryInstr condition1 = (BinaryInstr) br1.getOP(0);
                if (!(condition1.cmpType == InstType.CmpType.eq)) {
                    return;
                }
                for (var inst : block1.getInsts()) {
                    if (inst != condition1 && inst != br1) {
                        return;
                        //这意味着后继块2不是一个简单的常数比较及跳转，不符合要求。
                    }
                }
                for (BasicBlock block2 : block1.getSuccs()) {
                    if (!(block2.getInsts().get(block2.getInsts().size() - 1) instanceof Br)) {
                        return;
                    }
                    Br br2 = (Br) block2.getInsts().get(block2.getInsts().size() - 1);
                    if (!(br2.getOP(0) instanceof BinaryInstr)) {
                        if (!(block2.getSuccs().get(0).equals(retBlock))) {
                            return;
                            // 意味着该block2的后继块不是返回块，不符合要求。
                        }
                        BinaryInstr srem = null;
                        for (var inst : block2.getInsts()) {
                            if (inst != br2 && !(inst instanceof BinaryInstr
                                    && ((BinaryInstr) inst).instType == InstType.BinaryType.srem)) {
                                return;
                                //这意味着后继块2不是一个简单的取模及返回，不符合要求。
                            }
                            if (inst != br2 && srem == null) {
                                srem = (BinaryInstr) inst;
                            }
                            if (inst != br2 && srem != null && !srem.equals(inst)) {
                                return;
                            }
                        }
                        if (!phi.values.containsValue(srem)) {
                            return;
                        }
                    } else {
                        if (!(block2.getInsts().get(block2.getInsts().size() - 1) instanceof Br)) {
                            return;
                        }
                        Br br3 = (Br) block2.getInsts().get(block2.getInsts().size() - 1);
                        if (!(br3.getOP(0) instanceof BinaryInstr && ((BinaryInstr) br3.getOP(0)).instType ==
                                InstType.BinaryType.icmp)) {
                            return;
                        }
                        BinaryInstr condition2 = (BinaryInstr) br3.getOP(0);
                        if (!(condition2.cmpType == InstType.CmpType.eq)) {
                            return;
                        }

                        // TODO 其他运算
                        if (!(condition2.getOP(0) instanceof BinaryInstr
                                && ((BinaryInstr) condition2.getOP(0)).instType == InstType.BinaryType.srem)) {
                            return;
                        }
                        BinaryInstr srem1 = (BinaryInstr) condition2.getOP(0);
                        BinaryInstr srem2 = null;
                        for (var inst : block2.getInsts()) {
                            if (inst instanceof BinaryInstr
                                    && ((BinaryInstr) inst).instType == InstType.BinaryType.srem
                                    && srem2 == null && !inst.equals(srem1)) {
                                srem2 = (BinaryInstr) inst;
                            }
                            if (inst != br3 && srem2 != null && !srem2.equals(inst) && !srem1.equals(inst)) {
                                return;
                            }
                        }
                        if (!phi.values.containsValue(srem2)) {
                            return;
                        }
                        if (!(srem2.getOP(0) instanceof BinaryInstr
                                && ((BinaryInstr) srem2.getOP(0)).instType == InstType.BinaryType.add)) {
                            return;
                        }
                        BinaryInstr add1 = (BinaryInstr) srem2.getOP(0);
                        Call call = null;
                        for (Value ope : add1.getOperands()) {
                            if (!(ope instanceof Call)) {
                                return;
                            }
                            if (call == null) {
                                call = (Call) ope;
                            } else if (!call.equals(ope)) {
                                return;
                            }
                        }
                        if (!call.getFunction().equals(function)) {
                            return;
                        }
                        if (!(call.getOP(0) instanceof Arg)) {
                            return;
                        }
                        if (!(call.getOP(1) instanceof BinaryInstr
                                && ((BinaryInstr) call.getOP(1)).instType == InstType.BinaryType.sdiv)) {
                            return;
                        }
                        BinaryInstr sdiv = (BinaryInstr) call.getOP(1);
                        for (var instr : block2.getInsts()) {
                            if (instr != sdiv && instr != call && instr != srem1 && instr != condition2
                                    && instr != add1 && instr != srem2 && instr != br3) {
                                return;
                            }
                        }


                        for (BasicBlock block3 : block2.getSuccs()) {
                            if (!(block3.getSuccs().get(0).equals(retBlock))) {
                                return;
                                // 意味着该block3的后继块不是返回块，不符合要求。
                            }
                            Br br4 = (Br) block3.getInsts().get(block3.getInsts().size() - 1);
                            boolean onlyBr = true;
                            for (var inst : block3.getInsts()) {
                                if (inst != br4) {
                                    onlyBr = false;
                                    break;
                                }
                            }
                            if (!onlyBr) {
                                BinaryInstr srem = null;
                                for (var inst : block3.getInsts()) {
                                    if (inst instanceof BinaryInstr
                                            && ((BinaryInstr) inst).instType == InstType.BinaryType.srem
                                            && srem == null) {
                                        srem = (BinaryInstr) inst;
                                    }
                                    if (inst != br4 && srem != null && !srem.equals(inst)) {
                                        return;
                                    }
                                }
                                if (!phi.values.containsValue(srem)) {
                                    return;
                                }
                                if (!(srem.getOP(0) instanceof BinaryInstr
                                        && ((BinaryInstr) srem.getOP(0)).instType == InstType.BinaryType.add)) {
                                    return;
                                }
                                BinaryInstr add = (BinaryInstr) srem.getOP(0);
                                for (Value ope : add.getOperands()) {
                                    if (!(ope instanceof Arg || phi.values.containsValue(ope))) {
                                        return;
                                    }
                                }
                                for (var instr : block3.getInsts()) {
                                    if (instr != br4 && instr != add && instr != srem) {
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        function.blocks.clear();
        BasicBlock newBlock = new BasicBlock("newblock", function, true);
        var mul = new BinaryInstr(
            new LinkedList<>(Arrays.asList(function.getArgv2().get(0), function.getArgv2().get(1))),
            new IntType(), "mu", InstType.BinaryType.mul, newBlock);
        var mod = new BinaryInstr(new LinkedList<>(Arrays.asList(mul, modNum)),
            new IntType(), "mo", InstType.BinaryType.srem, newBlock);
        var ret = new Ret(new LinkedList<>(List.of(mod)), newBlock);
        newBlock.addValue(mul);
        newBlock.addValue(mod);
        newBlock.addValue(ret);
        function.addBlock(newBlock);
    }

    //存储唯一赋值区间的起始和终止
    private LinkedHashMap<Alloca, ConstNumber> storeStartPos = new LinkedHashMap<>();
    private LinkedHashMap<Alloca, Value> storeEndPos = new LinkedHashMap<>();
    private LinkedHashMap<Alloca, Boolean> isIncrement1 = new LinkedHashMap<>();

    //判断对应的store是否是唯一的store。
    private boolean onlyStore(Alloca array, Store store, Module module){
        return false;
    }

    //这种pattern对于循环中的第一轮初值进行识别。再循环体内，存在一种情况是其中的子循环用到了上层循环中的某个值，
    // 但是子循环的循环条件限制其只会运行一轮，即只在父循环的第一轮运行，这种情况如果用到的父循环的值在第一轮中是已知的，
    // 且子循环前父循环内没有用到子循环值的任何操作，
    // 则子循环与父循环无关，可以直接用该已知第一轮值替换，并外提该循环到父循环的前面。
    public void PatternD(Function function){
        for (Loop loop : function.loops) {
            if(loop.children.size()!=1){
                continue;
                //暂时不处理多个子循环的情况
            }
            LinkedHashMap<Value, ConstNumber> predict = new LinkedHashMap<>();

        }
    }
}
