package midend.optimism;

import frontend.AST.Func;
import midend.*;
import midend.LLVMType.UndefinedType;
import midend.Module;
import midend.analysis.CFGBuilder;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.*;

import java.util.*;

public class LoopUnroll {
    private Module module;
    private boolean change = false;
    private HashMap<Value, Value> valueMap = new HashMap<>();
    private IRBuilder builder = new IRBuilder();

    public LoopUnroll(Module module) {
        this.module = module;
    }

    public void run() {
        change = true;
        while (change) {
            change = false;
            for (Function function : module.getFunctions()) {
                LoopInfo.loopAnalysis(function);
                LoopInfo.iteratorAnalysis(function);
                ArrayList<Loop> loops = new ArrayList<>();
                for (Loop loop : function.getLoops()) {
                    if (!loops.contains(loop) && canBeUnroll(loop))  {
                        loops.add(loop);
                        unrollLoop(loop, function);
                    }
                }
//                for (Loop loop : loops) {
//                    unrollLoop(loop, function);
//                }

            }
            new CFGBuilder(module).run();
        }
    }

    public boolean canBeUnroll(Loop loop) {
//        if (loop.isIter()) {
//            if (loop.getItBegin() instanceof ConstInt && loop.getItChange() instanceof ConstInt && loop.getItEnd() instanceof ConstInt) {
//                valueMap = new HashMap<>();
//                BasicBlock head = loop.getHeader();
//                for (Instruction instruction : head.getInstrList()) {
//                    if (instruction instanceof PhiInstr) {
//                        if (instruction.getValueList().size() != 2) {
//                            return false;
//                        }
//                        for (BasicBlock block : ((PhiInstr) instruction).getPreBlockList()) {
//                            if (!loop.getBasicBlocks().contains(block)) {
//                                valueMap.put(instruction, ((PhiInstr) instruction).getValueFrom(block));
//                            }
//                        }
//                    }
//                }
//                return true;
//            }
//        }
        if (!(loop.getSubLoops().isEmpty() && loop.getBasicBlocks().size() == 2)) {
            return false;
        } else {
            valueMap = new HashMap<>();
            BasicBlock head = loop.getHeader();
            for (Instruction instruction : head.getInstrList()) {
                if (instruction instanceof PhiInstr) {
                    if (instruction.getValueList().size() != 2) {
                        return false;
                    }
                    for (BasicBlock block : ((PhiInstr) instruction).getPreBlockList()) {
                        if (!loop.getBasicBlocks().contains(block)) {
                            valueMap.put(instruction, ((PhiInstr) instruction).getValueFrom(block));
                        }
                    }
                }
            }

            for (BasicBlock block : loop.getBasicBlocks()) {
                for (Instruction instruction : block.getInstrList()) {
                    if (instruction instanceof BrInstr) {
                        continue;
                    }
                    if (instruction instanceof StoreInstr) {
                        continue;
                    }
                    if (instruction instanceof GetElementPtrInstr) {
                        continue;
                    }
                    if (instruction instanceof LoadInstr) {
                        continue;
                    }
                    if (instruction instanceof ALUInstr) {
                        continue;
                    }
                    if (instruction instanceof PhiInstr) {
                        continue;
                    }
                    if (instruction instanceof ConversionInstr) {
                        continue;
                    }
                    if (instruction instanceof CallInstr) {
                        continue;
                    }
                    return false;
                }
            }
            if (head.getInstrList().getLast().getValueList().isEmpty() || head.getInstrList().getLast().getValue(0) instanceof ConstInt && (((ConstInt) head.getInstrList().getLast().getValue(0)).getValue() == 1 || ((ConstInt) head.getInstrList().getLast().getValue(0)).getValue() == 0)) {
                return false;
            }
            HashSet<Value> waitCheck = new HashSet<>();
            HashSet<Value> needKnown = new HashSet<>();
            BrInstr brInstr = (BrInstr) head.getInstrList().getLast();
            if (!brInstr.getIsIf()) {
                return false;
            }
            if (!(brInstr.getValue() instanceof ConstInt)) {
                needKnown.add(brInstr.getValue());
            }
            //TODO
            while (needKnown.size() != 0) {
                Value temp = needKnown.iterator().next();
                needKnown.remove(temp);
                waitCheck.add(temp);
                if (temp instanceof Param || temp instanceof GlobalVar) {
                    continue;
                }
                for (Value value : ((Instruction) temp).getValueList()) {
                    if (value instanceof ConstInt || value instanceof ConstFloat) {
                        continue;
                    }
                    if (!waitCheck.contains(value)) {
                        needKnown.add(value);
                    }
                }
            }
            boolean same;
            while (!waitCheck.isEmpty()) {
                same = true;
                ArrayList<Value> removeCheck = new ArrayList<>();
                for (Value value : waitCheck) {
                    if (valueMap.containsKey(value)) {
                        removeCheck.add(value);
                        continue;
                    }
                    boolean known = true;
                    if (!(value instanceof Param || value instanceof GlobalVar)) {
                        for (Value value1 : ((Instruction) value).getValueList()) {
                            if (!((valueMap.containsKey(value1) && (valueMap.get(value1) instanceof ConstInt || valueMap.get(value1) instanceof ConstFloat)) || (value1 instanceof ConstInt || value1 instanceof ConstFloat))) {
                                known = false;
                                break;
                            }
                        }
                    } else {
                        known = false;
                    }
                    if (known) {
                        removeCheck.add(value);
                    }
                }
                if (!removeCheck.isEmpty()) {
                    same = false;
                    for (Value value : removeCheck) {
                        waitCheck.remove(value);
                    }
                }
                if (same) {
                    break;
                }
            }
            if (!waitCheck.isEmpty()) {
                return false;
            }
            return true;
        }
    }

    public void unrollLoop(Loop loop, Function function) {
        BasicBlock unrollBlock = new BasicBlock(UndefinedType.undefined, function);
        ArrayList<Instruction> instructions = new ArrayList<>();
        for (BasicBlock block : loop.getBasicBlocks()) {
            instructions.addAll(block.getInstrList());
        }
        int hold = 0;
        boolean firstLoop = true;
        boolean unrollSuccess = false;
        while (hold < 8010) {
            HashMap<Value, Value> phiParr = new HashMap<>();
            for (Instruction instruction : instructions) {
                hold++;
                if (instruction instanceof PhiInstr && !(firstLoop)) {
                    for (BasicBlock block : ((PhiInstr) instruction).getPreBlockList()) {
                        if (loop.getBasicBlocks().contains(block)) {
                            phiParr.put(instruction, valueMap.get(((PhiInstr) instruction).getValueFrom(block)));
                        }
                    }
                } else {
                    if (!phiParr.isEmpty()) {
                        for (Value value : phiParr.keySet()) {
                            valueMap.put(value, phiParr.get(value));
                        }
                    }
                    phiParr.clear();
                }

                if (instruction instanceof ALUInstr) {
                    LinkedList<Value> uses = new LinkedList<>();
                    for (Value value : instruction.getValueList()) {
                        if (valueMap.containsKey(value)) {
                            uses.add(valueMap.get(value));
                        } else {
                            uses.add(value);
                        }
                    }
                    boolean isConst = true;
                    for (Value value : uses) {
                        if (!(value instanceof ConstInt || value instanceof ConstFloat)) {
                            isConst = false;
                            break;
                        }
                    }
                    ALUInstr sameALU = new ALUInstr(instruction.getType(),
                            Arrays.asList(uses.get(0), uses.get(1)), instruction.getInstrType(), unrollBlock);
                    //TODO:sameBir.cmpType = instr.cmpType;
                    if (isConst && uses.get(0).getType().isInteger()) {
                        Constant constant = builder.buildNumber((Constant) uses.get(0), (Constant) uses.get(1), ((ALUInstr) instruction).getOpStr());
                        valueMap.put(instruction, constant);
                        sameALU.remove();
                    } else {
                        unrollBlock.addInstr(sameALU);
                        valueMap.put(instruction, sameALU);
                    }
                }

                if (instruction instanceof GetElementPtrInstr) {
                    Value target = null;
                    ArrayList<Value> uses = new ArrayList<>();
                    for (int count = 0; count < instruction.getValueList().size(); count++) {
                        Value value = instruction.getValue(count);
                        if (count == 0) {
                            if (valueMap.containsKey(value)) {
                                target = valueMap.get(value);
                            } else {
                                target = value;
                            }
                            continue;
                        }
                        if (valueMap.containsKey(value)) {
                            uses.add(valueMap.get(value));
                        } else {
                            uses.add(value);
                        }
                    }
                    GetElementPtrInstr newGep = new GetElementPtrInstr(instruction.getType(), target, uses);
                    newGep.setBasicBlock(unrollBlock);
                    unrollBlock.addInstr(newGep);
                    valueMap.put(instruction, newGep);
                }

                if (instruction instanceof LoadInstr) {
                    Value value = ((LoadInstr) instruction).getValue();
                    if (valueMap.containsKey(value)) {
                        value = valueMap.get(value);
                    }
                    LoadInstr newLoad = new LoadInstr(instruction.getType(), value, unrollBlock);
                    unrollBlock.addInstr(newLoad);
                    valueMap.put(instruction, newLoad);
                }

                if (instruction instanceof StoreInstr) {
                    Value value = ((StoreInstr) instruction).getValue();
                    if (valueMap.containsKey(value)) {
                        value = valueMap.get(value);
                    }
                    Value pointer = ((StoreInstr) instruction).getPointer();
                    if (valueMap.containsKey(pointer)) {
                        pointer = valueMap.get(pointer);
                    }
                    StoreInstr newStore = new StoreInstr(instruction.getType(), value, pointer, unrollBlock);
                    unrollBlock.addInstr(newStore);
                }

                if (instruction instanceof ConversionInstr) {
                    Value value = ((ConversionInstr) instruction).getValue();
                    if (valueMap.containsKey(value)) {
                        value = valueMap.get(value);
                    }
                    ConversionInstr newVcs = new ConversionInstr(instruction.getType(), instruction.getInstrType(), value);
                    newVcs.setBasicBlock(unrollBlock);
                    valueMap.put(instruction, newVcs);
                    unrollBlock.addInstr(newVcs);
                }

                if (instruction instanceof CallInstr) {
                    ArrayList<Value> values = new ArrayList<>(((CallInstr) instruction).getValues());
                    for (int count = 0; count < values.size(); count++) {
                        Value value = values.get(count);
                        if (valueMap.containsKey(value)) {
                            values.set(count, valueMap.get(value));
                        }
                    }
                    CallInstr newCall = new CallInstr(instruction.getType(), ((CallInstr) instruction).getFunction(), values);
                    newCall.setBasicBlock(unrollBlock);
                    valueMap.put(instruction, newCall);
                    unrollBlock.addInstr(newCall);
                }

                if (instruction instanceof BrInstr) {
                    if (((BrInstr) instruction).getIsIf()) {
                        Value cond = ((BrInstr) instruction).getValue();
                        if (((ConstInt) valueMap.get(cond)).getValue() == 0) {
                            unrollSuccess = true;
                            //替换block
                            for (BasicBlock basicBlock : function.getBlockList()) {
                                if (basicBlock.getInstrList().getLast() instanceof BrInstr) {
                                    ((BrInstr) basicBlock.getInstrList().getLast()).modifyBlock(loop.getHeader(), unrollBlock);
                                }
                                if (basicBlock.getInstrList().getFirst() instanceof PhiInstr) {
                                    Instruction instruction1 = basicBlock.getInstrList().get(0);
                                    int count0 = 0;
                                    while (instruction1 instanceof PhiInstr) {
                                        ((PhiInstr) instruction1).modifyBlock(loop.getHeader(), unrollBlock);
                                        instruction1 = basicBlock.getInstrList().get(++count0);
                                    }
                                }
                            }

                            for (Value oriValue : valueMap.keySet()) {
                                oriValue.replaceUse(valueMap.get(oriValue));
                                ((Instruction) oriValue).remove();
                            }
                            BrInstr brInstr = new BrInstr(((BrInstr) instruction).getIfFalseBlock());
                            brInstr.setBasicBlock(unrollBlock);
                            unrollBlock.addInstr(brInstr);
                            int count = function.getBlockList().indexOf(loop.getHeader());
                            function.getBlockList().add(count, unrollBlock);
                            function.getBlockList().removeAll(loop.getBasicBlocks());
                            //TODO:removedloops.add(loop)
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
            for (int count = unrollBlock.getInstrList().size() - 1; count >= 0; count--) {
                Instruction instruction = unrollBlock.getInstrList().get(count);
                instruction.remove();
            }
            return;
        }
        for (Instruction instruction : instructions) {
            instruction.remove();
        }
        change = true;
    }
}
