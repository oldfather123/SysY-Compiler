package midend.optimism;

import midend.*;
import midend.LLVMType.UndefinedType;
import midend.Module;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.*;

import java.beans.beancontext.BeanContext;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

public class LoopExtract {
    private Module module;

    public LoopExtract(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            LoopInfo.loopAnalysis(function);
            LoopInfo.getExitingBlocks();
            runForLoopExtract(function);
        }
    }

    public void runForLoopExtract(Function function) {
        for (Loop loop : function.getLoops()) {
            if (loop.getSubLoops().size() != 0) {
                for (Loop subLoop : loop.getSubLoops()) {
                    BasicBlock subHeader = subLoop.getHeader();
                    if (!(subHeader.getInstrList().get(subHeader.getInstrList().size() - 2) instanceof CmpInstr)) {
                        continue;
                    }
                    CmpInstr cmp = (CmpInstr) subHeader.getInstrList().get(subHeader.getInstrList().size() - 2);
//                    int subCount = 0;
                    Value subCount = null;
                    PhiInstr phiInstrSub = null;
                    if (cmp.getOpStr().equals("<") && cmp.getLeft() instanceof PhiInstr) {
                        subCount = cmp.getRight();
                        phiInstrSub = (PhiInstr) cmp.getLeft();
                    } else { //目前只考虑while(i < n)
                        continue;
                    }
                    if (phiInstrSub.getValueList().size() != 2) {
                        continue;
                    }
                    int subCountChange = 0;
                    String op = null;
                    for (User user : phiInstrSub.getUserList()) {
                        if (user instanceof ALUInstr && !(user instanceof CmpInstr)) {
                            if (subLoop.getBasicBlocks().contains(((ALUInstr) user).getBasicBlock())) {
                                if (((ALUInstr) user).getLeft().equals(phiInstrSub) && ((ALUInstr) user).getRight() instanceof ConstInt) {
                                    subCountChange = ((ConstInt) ((ALUInstr) user).getRight()).getValue();
                                    op = ((ALUInstr) user).getOpStr();
                                } else if (((ALUInstr) user).getRight().equals(phiInstrSub) && ((ALUInstr) user).getLeft() instanceof ConstInt) {
                                    subCountChange = ((ConstInt) ((ALUInstr) user).getLeft()).getValue();
                                    op = ((ALUInstr) user).getOpStr();
                                }
                            } else {
                                continue; //迭代变量不能在其他中被赋值
                            }
                        }
                    }
                    if (subCountChange == 0 || op == null) {
                        continue;
                    }
                    Value itVar = null;
                    for (int count = 0; count < phiInstrSub.getPreBlockList().size(); count++) {
                        if (!subLoop.getBasicBlocks().contains(phiInstrSub.getPreBlockList().get(count))) {
                            itVar = phiInstrSub.getValue(count);
                        }
                    }
                    if (!(itVar instanceof PhiInstr)) {
                        continue;
                    }
                    if (((PhiInstr) itVar).getValueList().size() != 2) {
                        continue;
                    }
                    int subBeginValue = -1;
                    for (int count = 0; count < ((PhiInstr) itVar).getPreBlockList().size(); count++) {
                        if (!loop.getBasicBlocks().contains(((PhiInstr) itVar).getPreBlockList().get(count))) {
                            if (((PhiInstr) itVar).getValue(count) instanceof ConstInt) {
                                subBeginValue = ((ConstInt) ((PhiInstr) itVar).getValue(count)).getValue();
                            }
                        }
                    }
                    if (subBeginValue == -1) {
                        continue;
                    }
                    if (!loop.getBasicBlocks().contains(phiInstrSub.getBasicBlock())) {
                        continue;
                    }
                    //subloop中用到的变量也可以提取
                    if (subLoop.getExitBlock().size() != 1) {
                        continue;
                    }
                    if (subLoop.getBasicBlocks().size() != 2) {
                        continue;
                    }
                    if (subLoop.getBasicBlocks().get(1).getInstrList().size() != 10) {
                        continue;
                    }
                    //到这里就可以提取了
                    ArrayList<BasicBlock> addBlock = new ArrayList<>();
                    HashMap<Value, Value> valueMap = new HashMap<>();
                    ArrayList<BasicBlock> tihu = new ArrayList<>();
                    int begin = function.getBlockList().indexOf(loop.getHeader());
                    for (int count0 = begin; count0 < function.getBlockList().size(); count0++) {
                        BasicBlock block = function.getBlockList().get(count0);
                        if (!loop.getBasicBlocks().contains(block)) {
                            continue;
                        }
                        if (block.equals(loop.getHeader())) {
                            for (Instruction instruction : block.getInstrList()) {
                                if (instruction instanceof PhiInstr) {
                                    for (int count = 0; count < ((PhiInstr) instruction).getPreBlockList().size(); count++) {
                                        if (!loop.getBasicBlocks().contains(((PhiInstr) instruction).getPreBlock(count))) {
                                            valueMap.put(instruction, instruction.getValue(count));
                                        }
                                    }
                                }
                            }
                            continue;
                        }
                        if (block.equals(subLoop.getExitBlock().get(0))) {
                            break;
                        }
                        tihu.add(block);
                        BasicBlock basicBlock = null;
                        if (valueMap.containsKey(block)) {
                            basicBlock = (BasicBlock) valueMap.get(block);
                        } else {
                            basicBlock = new BasicBlock(UndefinedType.undefined, function);
                            valueMap.put(block, basicBlock);
                        }
                        for (Instruction instruction : block.getInstrList()) {
                            if (instruction instanceof CmpInstr) {
                                Value left = valueMap.getOrDefault(((ALUInstr) instruction).getLeft(), ((ALUInstr) instruction).getLeft());
                                Value right = valueMap.getOrDefault(((ALUInstr) instruction).getRight(), ((ALUInstr) instruction).getRight());
                                CmpInstr cmpInstr = new CmpInstr(((CmpInstr) instruction).getOpStr(), instruction.getType(), Arrays.asList(left, right), instruction.getInstrType(), basicBlock);
                                valueMap.put(instruction, cmpInstr);
                                basicBlock.addInstr(cmpInstr);
                            } else if (instruction instanceof ALUInstr) {
                                Value left = valueMap.getOrDefault(((ALUInstr) instruction).getLeft(), ((ALUInstr) instruction).getLeft());
                                Value right = valueMap.getOrDefault(((ALUInstr) instruction).getRight(), ((ALUInstr) instruction).getRight());
                                ALUInstr aluInstr = new ALUInstr(instruction.getType(), Arrays.asList(left, right), instruction.getInstrType(), basicBlock);
                                valueMap.put(instruction, aluInstr);
                                basicBlock.addInstr(aluInstr);
                            } else if (instruction instanceof BrInstr) {
                                if (((BrInstr) instruction).getIsIf()) {
                                    Value cond = valueMap.getOrDefault(((BrInstr) instruction).getValue(), ((BrInstr) instruction).getValue());
                                    BasicBlock ifTrue = null;
                                    BasicBlock ifFalse = null;
                                    if (valueMap.containsKey(((BrInstr) instruction).getIfTrueBlock())) {
                                        ifTrue = (BasicBlock) valueMap.get(((BrInstr) instruction).getIfTrueBlock());
                                    } else {
                                        ifTrue = new BasicBlock(UndefinedType.undefined, function);
                                        valueMap.put(((BrInstr) instruction).getIfTrueBlock(), ifTrue);
                                    }
                                    if (valueMap.containsKey(((BrInstr) instruction).getIfFalseBlock())) {
                                        ifFalse = (BasicBlock) valueMap.get(((BrInstr) instruction).getIfFalseBlock());
                                    } else {
                                        ifFalse = new BasicBlock(UndefinedType.undefined, function);
                                        valueMap.put(((BrInstr) instruction).getIfFalseBlock(), ifFalse);
                                    }
                                    BrInstr brInstr = new BrInstr(cond, Arrays.asList(ifTrue, ifFalse));
                                    brInstr.setBasicBlock(basicBlock);
                                    basicBlock.addInstr(brInstr);
                                } else {
                                    BasicBlock dest = null;
                                    if (valueMap.containsKey(((BrInstr) instruction).getDestBlock())) {
                                        dest = (BasicBlock) valueMap.get(((BrInstr) instruction).getDestBlock());
                                    } else {
                                        dest = new BasicBlock(UndefinedType.undefined, function);
                                        valueMap.put(((BrInstr) instruction).getDestBlock(), dest);
                                    }
                                    BrInstr brInstr = new BrInstr(dest);
                                    brInstr.setBasicBlock(basicBlock);
                                    basicBlock.addInstr(brInstr);
                                }
                            } else if (instruction instanceof PhiInstr) {
                                ArrayList<BasicBlock> basicBlocks = new ArrayList<>();
                                for (BasicBlock block1 : ((PhiInstr) instruction).getPreBlockList()) {
                                    BasicBlock block2 = null;
                                    if (valueMap.containsKey(block1)) {
                                        block2 = (BasicBlock) valueMap.get(block1);
                                    } else {
                                        block2 = new BasicBlock(UndefinedType.undefined, function);
                                        valueMap.put(block1, block2);
                                    }
                                    basicBlocks.add(block2);
                                }
                                PhiInstr phiInstr = new PhiInstr(instruction.getType(), basicBlocks);
                                for (int count = 0; count < instruction.getValueList().size(); count++) {
                                    Value value = instruction.getValue(count);
                                    Value value1 = valueMap.getOrDefault(value, value);
                                    phiInstr.addOption(value1, basicBlocks.get(count));
                                }
                                valueMap.put(instruction, phiInstr);
                                phiInstr.setBasicBlock(basicBlock);
                                basicBlock.addInstr(phiInstr);
                            } else if (instruction instanceof ConversionInstr) {
                                Value value = valueMap.getOrDefault(((ConversionInstr) instruction).getValue(), ((ConversionInstr) instruction).getValue());
                                ConversionInstr conversionInstr = new ConversionInstr(instruction.getType(), instruction.getInstrType(), value);
                                conversionInstr.setBasicBlock(basicBlock);
                                valueMap.put(instruction, conversionInstr);
                                basicBlock.addInstr(conversionInstr);
                            } else if (instruction instanceof GetElementPtrInstr) {
                                Value value = valueMap.getOrDefault(((GetElementPtrInstr) instruction).getTarget(), ((GetElementPtrInstr) instruction).getTarget());
                                ArrayList<Value> index = new ArrayList<>();
                                for (Value value1 : ((GetElementPtrInstr) instruction).getIndexes()) {
                                    index.add(valueMap.getOrDefault(value1, value1));
                                }
                                GetElementPtrInstr getElementPtrInstr = new GetElementPtrInstr(instruction.getType(), value, index);
                                getElementPtrInstr.setBasicBlock(basicBlock);
                                valueMap.put(instruction, getElementPtrInstr);
                                basicBlock.addInstr(getElementPtrInstr);
                            } else if (instruction instanceof StoreInstr) {
                                Value value = valueMap.getOrDefault(((StoreInstr) instruction).getValue(), ((StoreInstr) instruction).getValue());
                                Value pointer = valueMap.getOrDefault(((StoreInstr) instruction).getPointer(), ((StoreInstr) instruction).getPointer());
                                StoreInstr storeInstr = new StoreInstr(instruction.getType(), value, pointer, basicBlock);
                                basicBlock.addInstr(storeInstr);
                            }
                        }
                        addBlock.add(basicBlock);
                    }

                    //维护块块们
                    BasicBlock before = null;
                    for (BasicBlock block : loop.getHeader().getPreBlock()) {
                        if (!loop.getBasicBlocks().contains(block)) {
                            before = block;
                            break;
                        }
                    }
                    if (before == null) {
                        continue;
                    }
                    ((BrInstr) before.getInstrList().getLast()).modifyBlock(loop.getHeader(), addBlock.get(0));
                    BasicBlock enteringBlock = (BasicBlock) valueMap.get(subHeader);
                    ((BrInstr) enteringBlock.getInstrList().getLast()).modifyBlock(((BrInstr) enteringBlock.getInstrList().getLast()).getIfFalseBlock(), loop.getHeader());
                    for (Instruction instruction : loop.getHeader().getInstrList()) {
                        if (instruction instanceof PhiInstr) {
                            ((PhiInstr) instruction).modifyBlock(before, enteringBlock);
                        }
                    }

                    PhiInstr phiInstr = (PhiInstr) addBlock.get(addBlock.size() - 2).getInstrList().getFirst();
                    phiInstr.addOption(addBlock.get(addBlock.size() - 1).getInstrList().get(addBlock.get(addBlock.size() - 1).getInstrList().size() - 2), addBlock.get(addBlock.size() - 1));
                    PhiInstr phiInstrb = (PhiInstr) addBlock.get(2).getInstrList().getFirst();
                    phiInstrb.addOption(addBlock.get(3).getInstrList().getFirst(), addBlock.get(3));
                    PhiInstr phiInstra = (PhiInstr) addBlock.get(2).getInstrList().get(1);
                    phiInstra.addOption(addBlock.get(3).getInstrList().get(1), addBlock.get(3));

                    int index = function.getBlockList().indexOf(before);
                    for (int count = addBlock.size() - 1; count >= 0; count--) {
                        BasicBlock block = addBlock.get(count);
                        function.getBlockList().add(index + 1, block);
                    }
                    ArrayList<Instruction> delete = new ArrayList<>();
                    for (BasicBlock block : tihu) {
                        for (Instruction instruction : block.getInstrList()) {
                            delete.add(instruction);
                        }
                    }
                    for (Instruction del : delete) {
                        del.remove();
                    }
                    ((PhiInstr) itVar).remove();
                    for (BasicBlock block : tihu) {
                        function.getBlockList().remove(block);
                    }
                    ((BrInstr) loop.getHeader().getInstrList().getLast()).modifyBlock(tihu.get(0), subLoop.getExitBlock().get(0));
                    //获取loop的循环参数
//                    BasicBlock header = loop.getHeader();
//                    CmpInstr cmpInstr = (CmpInstr) header.getInstrList().get(header.getInstrList().size() - 2);
//                    int loopSubCount = -1;
//                    PhiInstr phiInstrLoop = null;
//                    if (cmpInstr.getRight() instanceof ConstInt) {
//                        loopSubCount = ((ConstInt) cmpInstr.getRight()).getValue();
//                        phiInstrLoop = (PhiInstr) cmpInstr.getLeft();
//                    } else {
//                        continue;
//                    }
//                    if (phiInstrLoop.getValueList().size() != 2) {
//                        continue;
//                    }
//                    int subCountChangeLoop = 0;
//                    String opLoop = null;
//                    int subBeginValueLoop = -1;
//                    if (phiInstrLoop.getValue(0) instanceof ConstInt) {
//                        subBeginValueLoop = ((ConstInt) phiInstrLoop.getValue(0)).getValue();
//                        for (User user : phiInstrLoop.getValue(1).getUserList()) {
//                            if (user instanceof ALUInstr) {
//                                if (((ALUInstr) user).getLeft() instanceof ConstInt) {
//                                    subCountChangeLoop = ((ConstInt) ((ALUInstr) user).getLeft()).getValue();
//                                    opLoop = ((ALUInstr) user).getOpStr();
//                                } else if (((ALUInstr) user).getRight() instanceof ConstInt) {
//                                    subCountChangeLoop = ((ConstInt) ((ALUInstr) user).getRight()).getValue();
//                                    opLoop = ((ALUInstr) user).getOpStr();
//                                }
//                            }
//                        }
//                    } else if (phiInstrLoop.getValue(1) instanceof ConstInt) {
//                        subBeginValueLoop = ((ConstInt) phiInstrLoop.getValue(1)).getValue();
//                        for (User user : phiInstrLoop.getValue(0).getUserList()) {
//                            if (user instanceof ALUInstr) {
//                                if (((ALUInstr) user).getLeft() instanceof ConstInt) {
//                                    subCountChangeLoop = ((ConstInt) ((ALUInstr) user).getLeft()).getValue();
//                                    opLoop = ((ALUInstr) user).getOpStr();
//                                } else if (((ALUInstr) user).getRight() instanceof ConstInt) {
//                                    subCountChangeLoop = ((ConstInt) ((ALUInstr) user).getRight()).getValue();
//                                    opLoop = ((ALUInstr) user).getOpStr();
//                                }
//                            }
//                        }
//                    }
//                    if (subBeginValueLoop == -1 || opLoop == null || subCountChangeLoop == 0) {
//                        continue;
//                    }
                    //判断是相同循环
//                    if (!(subBeginValue == subBeginValueLoop && subCount == loopSubCount && subCountChange == subCountChangeLoop && op.equals(opLoop))) {
//                        continue;
//                    }

                }
            }
        }
    }
}
