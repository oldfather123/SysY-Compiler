package midend.optimism;

import Utils.LibFunction;
import frontend.AST.Func;
import midend.*;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.UndefinedType;
import midend.Module;
import midend.analysis.FunctionSideEffect;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

//针对以下情况进行优化
/*
while (i < 常数) {
    a = a + C  /// a = (a + C) % D
}
 */
public class LoopSimplify {
    private Module module;
    private boolean change = false;
    private boolean simplify;

    public LoopSimplify(Module module, boolean simplify2) {
        this.module = module;
        this.simplify = simplify2;
    }

    public void run() {
        FunctionSideEffect.sideEffectAnalysis(module);
        change = true;
        while (change) {
            change = false;
            for (Function function : module.getFunctions()) {
                LoopInfo.loopAnalysis(function);
                LoopInfo.getExitingBlocks();
                loopSimplify(function);
                if (simplify) {
                    loopSimplify2(function);
                }
            }
        }
    }

    public void loopSimplify2(Function function) {
        for (Loop loop : function.getLoops()) {
            if (loop.getBasicBlocks().size() != 2) {
                continue;
            }
            if (loop.getExitBlock().size() != 1) {
                continue;
            }
            BasicBlock header = loop.getHeader();
            BasicBlock exceu = loop.getBasicBlocks().get(1);
            if (exceu.getInstrList().size() != 4) {
                continue;
            }
            boolean slyw = true;
            CallInstr callInstr = null;
            for (Instruction instruction : exceu.getInstrList()) {
                if (instruction instanceof ALUInstr) {
                    continue;
                } else if (instruction instanceof BrInstr) {
                    continue;
                } else if (instruction instanceof CallInstr) {
                    if (notChangeGlobal(((CallInstr) instruction).getFunction())) {
                        callInstr = (CallInstr) instruction;
                        continue;
                    } else {
                        slyw = false;
                        break;
                    }
                } else {
                    slyw = false;
                    break;
                }
            }
            if (!slyw || callInstr == null) {
                continue;
            }
            CmpInstr cmpInstr = (CmpInstr) header.getInstrList().get(header.getInstrList().size() - 2);
            if (!cmpInstr.getOpStr().equals("<")) {
                continue;
            }
            PhiInstr itVar = (PhiInstr) cmpInstr.getLeft();
            ALUInstr itInstr = null;
            int change = -1;
            for (int count = 0; count < itVar.getPreBlockList().size(); count++) {
                BasicBlock block = itVar.getPreBlockList().get(count);
                if (loop.getBasicBlocks().contains(block)) {
                    itInstr = (ALUInstr) itVar.getValue(count);
                }
            }
            if (itInstr == null) {
                continue;
            }
            if (itInstr.getRight() instanceof ConstInt) {
                change = ((ConstInt) itInstr.getRight()).getValue();
            }
            if (change != 1 || !itInstr.getOpStr().equals("+")) {
                continue;
            }
            if (!(exceu.getInstrList().get(1) instanceof ALUInstr)) {
                continue;
            }
            ALUInstr resultAlu = (ALUInstr) exceu.getInstrList().get(1);
            if (!exceu.getInstrList().get(1).getValueList().contains(callInstr)) {
                continue;
            }
            PhiInstr resultPhi = (PhiInstr) ((ALUInstr) exceu.getInstrList().get(1)).getLeft();
            CallInstr newCall = new CallInstr(callInstr.getType(), callInstr.getFunction(), callInstr.getValues());
            BasicBlock block = new BasicBlock(UndefinedType.undefined, function);
            newCall.setBasicBlock(block);
//            header.getInstrList().add(header.getInstrList().size() - 2, newCall);
            block.addInstr(newCall);
            BrInstr brInstr = new BrInstr(header);
            brInstr.setBasicBlock(block);
            block.addInstr(brInstr);
            int index = function.getBlockList().indexOf(header);
            function.getBlockList().add(index, block);
            BasicBlock preBlock = null;
            for (Instruction instruction : header.getInstrList()) {
                if (instruction instanceof PhiInstr) {
                    if (preBlock == null) {
                        preBlock = ((PhiInstr) instruction).getPreBlock(0);
                    }
                    ((PhiInstr) instruction).modifyBlock(((PhiInstr) instruction).getPreBlock(0), block);
                }
            }
            ((BrInstr) preBlock.getInstrList().getLast()).modifyBlock(header, block);
            callInstr.replaceUse(newCall);
            callInstr.remove();

//            Value mulValue = cmpInstr.getRight();
//            if (resultAlu.getType().isFloat()) {
//                ConversionInstr conversionInstr = new ConversionInstr(FloatType.f32, InstrType.SITOFP, cmpInstr.getRight());
//                conversionInstr.setBasicBlock(header);
//                mulValue = conversionInstr;
//                header.getInstrList().add(header.getInstrList().size() - 2, conversionInstr);
//            }
//            ALUInstr mul = new ALUInstr(resultAlu.getType(), Arrays.asList(newCall, mulValue), (resultAlu.getType().isFloat()) ? InstrType.FMUL : InstrType.MUL, header);
//            header.getInstrList().add(header.getInstrList().size() - 2, mul);
//            resultPhi.replaceUse(mul);
//            ArrayList<Instruction> delete = new ArrayList<>();
//            for (Instruction instruction : exceu.getInstrList()) {
//                delete.add(instruction);
//            }
//            for (Instruction instruction : header.getInstrList()) {
//                if (instruction instanceof PhiInstr) {
//                    delete.add(instruction);
//                }
//            }
//            for (Instruction del : delete) {
//                del.remove();
//            }
//            BrInstr brInstr = new BrInstr(((BrInstr) header.getInstrList().getLast()).getIfFalseBlock());
//            ((BrInstr) header.getInstrList().getLast()).remove();
//            brInstr.setBasicBlock(header);
//            header.getInstrList().add(brInstr);
//            function.getBlockList().remove(exceu);
        }
    }

    public void loopSimplify(Function function) {
        ArrayList<Loop> loops = new ArrayList<>(function.getLoopInfo().values());
        for (Loop loop : loops) {
            if (loop.getBasicBlocks().size() != 2 || !loop.getSubLoops().isEmpty()) {
                continue;
            }
            int phiCount = 0;
            for (BasicBlock block : loop.getBasicBlocks()) {
                for (Instruction instruction : block.getInstrList()) {
                    if (instruction instanceof PhiInstr) {
                        phiCount++;
                    }
                }
            }
            if (phiCount != 2) {
                continue;
            }
            HashMap<Value, Value> phiMap = new HashMap<>();
            BasicBlock head = loop.getHeader();
            if (head.getPreBlock().size() != 2) {
                continue;
            }
            boolean match = true;
            for (Instruction instruction : head.getInstrList()) {
                if (instruction instanceof PhiInstr) {
                    if (((PhiInstr) instruction).getPreBlockList().size() != 2) {
                        match = false;
                        break;
                    }
                    for (BasicBlock block : ((PhiInstr) instruction).getPreBlockList()) {
                        if (!(loop.getBasicBlocks().contains(block))) {
                            phiMap.put(instruction, ((PhiInstr) instruction).getValueFrom(block));
                        }
                    }
                }
            }
            if (!match) {
                continue;
            }
            BasicBlock exitBlock = loop.getExitBlock().get(0);
            BrInstr brInstr = (BrInstr) head.getInstrList().getLast();
            if (!brInstr.getIsIf()) {
                continue;
            }
            Value cond = brInstr.getValue();
            if (!(cond instanceof ALUInstr)) {
                continue;
            }
            if (!(cond instanceof CmpInstr && ((CmpInstr) cond).getOp() == OP.SLT)) {
                continue;
            }
            PhiInstr condPhi = null; //条件判断的phi
            Value loopNumber = null; //循环次数
            for (Value value : ((CmpInstr) cond).getValueList()) {
                if (!(value instanceof Constant || head.getInstrList().contains(value) || loop.getBasicBlocks().get(1).getInstrList().contains(value) || phiMap.containsKey(value) || (value instanceof CallInstr && ((CallInstr) value).getFunction().equals(LibFunction.findFunc("getint"))))) {
                    match = false;
                    break;
                }
                if (phiMap.containsKey(value)) {
                    if (!(value instanceof PhiInstr)) {
                        match = false;
                        break;
                    }
                    if (condPhi == null) {
                        condPhi = (PhiInstr) value;
                    } else {
                        match = false;
                        break;
                    }
                    for (BasicBlock block : ((PhiInstr) value).getPreBlockList()) {
                        if (((PhiInstr) value).getValueFrom(block) instanceof ALUInstr) {
                            ALUInstr aluInstr = (ALUInstr) ((PhiInstr) value).getValueFrom(block);
                            if (!aluInstr.getValueList().contains(value) || !aluInstr.getOpStr().equals("+")) {
                                match = false;
                                break;
                            }
                            ArrayList<Value> values = new ArrayList<>(aluInstr.getValueList());
                            values.remove(value);
                            for (Value value1 : values) {
                                if (value1 instanceof Constant) {
                                    if (!(value1 instanceof ConstInt && ((ConstInt) value1).getValue() == 1)) {
                                        match = false;
                                        break;
                                    }
                                } else {
                                    match = false;
                                    break;
                                }
                            }
                        } else if (((PhiInstr) value).getValueFrom(block) instanceof Constant) {
                            continue;
                        } else {
                            match = false;
                            break;
                        }
                        if (!match) {
                            break;
                        }
                    }
                } else {
                    loopNumber = value;
                }
                if (!match) {
                    break;
                }
            }
            if (!match) {
                continue;
            }
            PhiInstr calPhi = null; //储存变量的phi
            for (Instruction instruction : head.getInstrList()) {
                if (instruction instanceof PhiInstr && !instruction.equals(condPhi)) {
                    calPhi = (PhiInstr) instruction;
                }
            }
            if (calPhi == null) {
                continue;
            }

            Value condValue = null; //递归变量
            Value calValue = null; //求值变量
            for (BasicBlock block : calPhi.getPreBlockList()) {
                if (loop.getBasicBlocks().contains(block)) {
                    condValue = calPhi.getValueFrom(block);
                } else {
                    calValue = calPhi.getValueFrom(block);
                }
            }
            if (!(condValue instanceof ALUInstr)) {
                continue;
            }
            if (((ALUInstr) condValue).getOpStr().equals("+")) {
                if (head.getInstrList().size() +
                        loop.getBasicBlocks().get(1).getInstrList().size() != 7) {
                    continue;
                }
                if (!(((ALUInstr) condValue).getValueList().contains(calPhi))) {
                    continue;
                }
                Value loopAdd = null;
                for (Value value : ((ALUInstr) condValue).getValueList()) {
                    if (value.equals(calPhi)) {
                        continue;
                    }
                    boolean have = false;
                    for (BasicBlock block : loop.getBasicBlocks()) {
                        if (block.getInstrList().contains(value)) {
                            have = true;
                            break;
                        }
                    }
                    if (!have) {
                        loopAdd = value;
                    } else {
                        match = false;
                        break;
                    }
                }
                if (!match) {
                    continue;
                }
                if (calValue.getType().isFloat()) {
                    continue;
                }
                BasicBlock insertBlock = new BasicBlock(UndefinedType.undefined, function);
                ALUInstr mul = new ALUInstr(IntegerType.i32, Arrays.asList(loopAdd, loopNumber), InstrType.MUL, insertBlock);
                insertBlock.addInstr(mul);
                ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(calValue, mul), InstrType.ADD, insertBlock);
                insertBlock.addInstr(add);
                calPhi.replaceUse(add);
                //替换block
                for (BasicBlock basicBlock : function.getBlockList()) {
                    if (basicBlock.getInstrList().getLast() instanceof BrInstr) {
                        ((BrInstr) basicBlock.getInstrList().getLast()).modifyBlock(head, insertBlock);
                    }
                    if (basicBlock.getInstrList().getFirst() instanceof PhiInstr) {
                        Instruction instruction1 = basicBlock.getInstrList().get(0);
                        int count0 = 0;
                        while (instruction1 instanceof PhiInstr) {
                            ((PhiInstr) instruction1).modifyBlock(head, insertBlock);
                            instruction1 = basicBlock.getInstrList().get(++count0);
                        }
                    }
                }
                for (BasicBlock block : loop.getBasicBlocks()) {
                    for (int count = block.getInstrList().size() - 1; count >= 0; count--) {
                        Instruction instruction = block.getInstrList().get(count);
                        instruction.remove();
                    }
                }
                BrInstr brInstr1 = new BrInstr(exitBlock);
                brInstr1.setBasicBlock(insertBlock);
                insertBlock.addInstr(brInstr1);
                int count = function.getBlockList().indexOf(head);
                function.getBlockList().add(count, insertBlock);
                function.getBlockList().removeAll(loop.getBasicBlocks());
                change = true;
                return;
            } else if (((ALUInstr) condValue).getOpStr().equals("%")) {
                if (head.getInstrList().size() +
                        loop.getBasicBlocks().get(1).getInstrList().size() != 8) {
                    continue;
                }
                ConstInt mod = null;
                if (((ALUInstr) condValue).getValue(1) instanceof ConstInt) {
                    mod = ((ConstInt) ((ALUInstr) condValue).getValue(1));
                } else {
                    continue;
                }
                if (mod.getValue() < 0) {
                    continue;
                }
                //TODO

                if (!(((ALUInstr) condValue).getValue(0) instanceof ALUInstr)) {
                    continue;
                }
                ALUInstr aluInstr = (ALUInstr) ((ALUInstr) condValue).getValue(0);
                if (!aluInstr.getValueList().contains(calPhi)) {
                    continue;
                }
                if (!aluInstr.getOpStr().equals("+")) {
                    continue;
                }
                Value loopAdd = null;
                for (Value value : ((ALUInstr) aluInstr).getValueList()) {
                    if (value.equals(calPhi)) {
                        continue;
                    }
                    boolean have = false;
                    for (BasicBlock block : loop.getBasicBlocks()) {
                        if (block.getInstrList().contains(value)) {
                            have = true;
                            break;
                        }
                    }
                    if (!have) {
                        loopAdd = value;
                    } else {
                        match = false;
                        break;
                    }
                }
                if (!match) {
                    continue;
                }
                ConstInt mod2 = mod;
                if (loopAdd instanceof ConstInt) {
                    if (((ConstInt) loopAdd).getValue() < 0) {
                        continue;
                    }
                    mod2 = ((ConstInt) loopAdd);
                }
                if (mod.getValue() * mod2.getValue() > Integer.MAX_VALUE) {
                    continue;
                }
                if (calValue.getType().isFloat()) {
                    continue;
                }
                BasicBlock insertBlock = new BasicBlock(UndefinedType.undefined, function);
                ALUInstr mod1 = new ALUInstr(IntegerType.i32, Arrays.asList(loopNumber, mod), InstrType.MOD, insertBlock);
                insertBlock.addInstr(mod1);
                ALUInstr mod02 = new ALUInstr(IntegerType.i32, Arrays.asList(loopAdd, mod), InstrType.MOD, insertBlock);
                insertBlock.addInstr(mod02);
                ALUInstr mul = new ALUInstr(IntegerType.i32, Arrays.asList(mod1, mod02), InstrType.MUL, insertBlock);
                insertBlock.addInstr(mul);
                ALUInstr mod3 = new ALUInstr(IntegerType.i32, Arrays.asList(mul, mod), InstrType.MOD, insertBlock);
                insertBlock.addInstr(mod3);
                ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(calValue, mod3), InstrType.ADD, insertBlock);
                insertBlock.addInstr(add);
                ALUInstr mod4 = new ALUInstr(IntegerType.i32, Arrays.asList(add, mod), InstrType.MOD, insertBlock);
                insertBlock.addInstr(mod4);

                calPhi.replaceUse(mod4);
                //替换block
                for (BasicBlock basicBlock : function.getBlockList()) {
                    if (basicBlock.getInstrList().getLast() instanceof BrInstr) {
                        ((BrInstr) basicBlock.getInstrList().getLast()).modifyBlock(head, insertBlock);
                    }
                    if (basicBlock.getInstrList().getFirst() instanceof PhiInstr) {
                        Instruction instruction1 = basicBlock.getInstrList().get(0);
                        int count0 = 0;
                        while (instruction1 instanceof PhiInstr) {
                            ((PhiInstr) instruction1).modifyBlock(head, insertBlock);
                            instruction1 = basicBlock.getInstrList().get(++count0);
                        }
                    }
                }
                for (BasicBlock block : loop.getBasicBlocks()) {
                    for (int count = block.getInstrList().size() - 1; count >= 0; count--) {
                        Instruction instruction = block.getInstrList().get(count);
                        instruction.remove();
                    }
                }
                BrInstr brInstr1 = new BrInstr(exitBlock);
                brInstr1.setBasicBlock(insertBlock);
                insertBlock.addInstr(brInstr1);
                int count = function.getBlockList().indexOf(head);
                function.getBlockList().add(count, insertBlock);
                function.getBlockList().removeAll(loop.getBasicBlocks());
                change = true;
                return;
            }
        }
    }

    public boolean notChangeGlobal(Function function) {
        for (BasicBlock block : function.getBlockList()) {
            if (block.equals(function.getBlockList().get(0))) {
                continue;
            }
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof StoreInstr) {
                    return false;
                }
            }
        }
        return true;
    }
}
