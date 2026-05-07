package midend.optimism;

import midend.*;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.UndefinedType;
import midend.LLVMType.VoidType;
import midend.Module;
import midend.instr.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedList;

public class RemovePhi {
    private Module module;
    private boolean removeOnlyUseLess = false;

    public RemovePhi(Module module, boolean removeOnlyUseLess) {
        this.module = module;
        this.removeOnlyUseLess = removeOnlyUseLess;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            removeUseLessPhi(function);
            if (!removeOnlyUseLess) {
                setTail(function);
                phi2copy(function);
                copy2Move(function);
            }
        }
    }

    public void setTail(Function function) {
        if (function.isRecursive()) {
            for (BasicBlock block : function.getBlockList()) {
                for (int count = 0; count < block.getInstrList().size() - 1; count++) {
                    Instruction instruction = block.getInstrList().get(count);
                    Instruction next = block.getInstrList().get(count + 1);
                    if (instruction instanceof CallInstr && ((CallInstr) instruction).getFunction().equals(function)) {
                        if (next instanceof BrInstr && !((BrInstr) next).getIsIf() && ((BrInstr) next).getDestBlock().equals(function.getExitBlock())) {

                        } else {
                            return;
                        }
                    }
                }
            }
            function.setTail();
            return;
        }
        return;
    }

    public void removeUseLessPhi(Function function) {
        boolean change = true;
        while (change) {
            change = false;
            for (BasicBlock block : function.getBlockList()) {
                ArrayList<Instruction> delete = new ArrayList<>();
                for (int count0 = 0; count0 < block.getInstrList().size(); count0++) {
                    Instruction instruction = block.getInstrList().get(count0);
                    if (instruction instanceof PhiInstr) {
                        Instruction next = block.getInstrList().get(count0 + 1);
                        Value first = instruction.getValue(0);
                        boolean canBedelete = true;
                        for (int count = 1; count < instruction.getValueList().size(); count++) {
                            if (!instruction.getValue(count).equals(first)) {
                                canBedelete = false;
                                break;
                            }
                        }
                        if (canBedelete) {
                            instruction.replaceUse(first);
                            delete.add(instruction);
                            change = true;
                            continue;
                        }
                        if (next != null && next instanceof PhiInstr) {
                            if (next.getValueList().size() != instruction.getValueList().size()) {
                                continue;
                            }
                            boolean isEqual = true;
                            for (int count1 = 0; count1 < instruction.getValueList().size(); count1++) {
                                Value before = instruction.getValue(count1);
                                Value after = next.getValue(count1);
                                if (!before.equals(after) || !((PhiInstr) instruction).getPreBlock(count1).equals(((PhiInstr) next).getPreBlock(count1))) {
                                    isEqual = false;
                                    break;
                                }
                            }
                            if (isEqual) {
                                instruction.replaceUse(next);
                                delete.add(instruction);
                                change = true;
                            }
                        }
                    }
                }
                for (Instruction instruction : delete) {
                    instruction.remove();
                }
            }
        }
    }

    public void phi2copy(Function function) {
        ArrayList<BasicBlock> bbList = new ArrayList<>(function.getBlockList());
        for (BasicBlock block : bbList) {
            if (!(block.getInstrList().getFirst() instanceof PhiInstr)) {
                continue;
            }
            ArrayList<PCopyInstr> pCopyInstrs = new ArrayList<>();
            ArrayList<BasicBlock> preBlocks = new ArrayList<>(block.getPreBlock());
            for (int count = 0; count < preBlocks.size(); count++) {
                BasicBlock preBlock = preBlocks.get(count);
                PCopyInstr pCopy = new PCopyInstr();
                pCopyInstrs.add(pCopy);
                if (preBlock.getNextBlock().size() == 1) {
                    LinkedList<Instruction> instructions = preBlock.getInstrList();
                    instructions.add(instructions.size() - 1, pCopy);
                    pCopy.setBasicBlock(preBlock);
                } else {
                    BasicBlock midBlock = new BasicBlock(UndefinedType.undefined, function);
                    function.addBB(midBlock);
                    midBlock.addInstr(pCopy);
                    pCopy.setBasicBlock(midBlock);
                    //修改跳转指令
                    BrInstr brInstr = (BrInstr) preBlock.getInstrList().getLast();
                    BasicBlock ifTrueBlock = brInstr.getIfTrueBlock();
                    BasicBlock ifFalseBlock = brInstr.getIfFalseBlock();
                    if (block.equals(ifTrueBlock)) {
                        brInstr.setIfTrueBlock(midBlock);
                        BrInstr midBr = new BrInstr(block);
                        midBlock.addInstr(midBr);
                        midBr.setBasicBlock(midBlock);
                    } else {
                        brInstr.setIfFalseBlock(midBlock);
                        BrInstr midBr = new BrInstr(block);
                        midBlock.addInstr(midBr);
                        midBr.setBasicBlock(midBlock);
                    }
                    //维持前驱后继关系
                    preBlock.getNextBlock().remove(block);
                    preBlock.getNextBlock().add(midBlock);
                    block.getPreBlock().remove(preBlock);
                    block.getPreBlock().add(midBlock);
                    midBlock.setNextBlock(new ArrayList<>());
                    midBlock.setNextBlock(block);
                    midBlock.setPreBlock(new ArrayList<>());
                    midBlock.setPreBlock(preBlock);
                }
            }

            //已经完成了插入pcopy，删除phi
            ArrayList<Instruction> deletePhi = new ArrayList<>();
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof PhiInstr) {
                    PhiInstr phiInstr = (PhiInstr) instruction;
                    deletePhi.add(phiInstr);
                    for (int count = 0; count < phiInstr.getOptions().size(); count++) {
                        Value now = phiInstr.getValue(count);
                        PCopyInstr pCopyInstr = pCopyInstrs.get(count);
//                        if (now instanceof UndefinedValue) {
//                            throw new RuntimeException("phi has undefined value\n");
//                        } else {
                        pCopyInstr.addCopy(Arrays.asList(phiInstr, now));
                        //}
                    }
                }
            }
            for (Instruction instruction : deletePhi) {
                instruction.remove();
            }
        }
    }

    public void copy2Move(Function function) {
        for (BasicBlock block : function.getBlockList()) {
            LinkedList<Instruction> instructions = block.getInstrList();
            if (instructions.size() > 1 && instructions.get(instructions.size() - 2) instanceof PCopyInstr) {
                PCopyInstr pCopyInstr = (PCopyInstr) instructions.get(instructions.size() - 2);
                pCopyInstr.remove();
                LinkedList<MoveInstr> moveInstrs = getMove(pCopyInstr);
                for (MoveInstr moveInstr : moveInstrs) {
                    if (moveInstr.getSrc().getType() == IntegerType.i32 && moveInstr.getDst().getType() == FloatType.f32) {
                        ConversionInstr conversionInstr = new ConversionInstr(FloatType.f32, InstrType.SITOFP, moveInstr.getSrc());
                        instructions.add(instructions.size() - 1, conversionInstr);
                        conversionInstr.setBasicBlock(block);
                        moveInstr.setSrc(conversionInstr);
                    } else if (moveInstr.getSrc().getType() == FloatType.f32 && moveInstr.getDst().getType() == IntegerType.i32) {
                        ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, moveInstr.getSrc());
                        instructions.add(instructions.size() - 1, conversionInstr);
                        conversionInstr.setBasicBlock(block);
                        moveInstr.setSrc(conversionInstr);
                    }
                    instructions.add(instructions.size() - 1, moveInstr);
                    moveInstr.setBasicBlock(block);
                }
            }
        }
    }

    public LinkedList<MoveInstr> getMove(PCopyInstr pCopyInstr) {
        ArrayList<Value> src = pCopyInstr.getNow();
        ArrayList<Value> dst = pCopyInstr.getBefore();
        LinkedList<MoveInstr> moveInstrs = new LinkedList<>();
        for (int count = 0; count < src.size(); count++) {
            MoveInstr moveInstr = new MoveInstr(src.get(count), dst.get(count));
            moveInstrs.add(moveInstr);
        }
        ArrayList<MoveInstr> midValue = new ArrayList<>();
        HashSet<Value> visited = new HashSet<>(); //?
        for (int count = 0; count < moveInstrs.size(); count++) {
            Value value = moveInstrs.get(count).getDst();
            if (!visited.contains(value)) {
                visited.add(value);
                boolean isLoopAssign = false;
                for (int count1 = count + 1; count1 < moveInstrs.size(); count1++) {
                    if (moveInstrs.get(count1).getSrc().equals(value)) {
                        isLoopAssign = true;
                        break;
                    }
                }
                if (isLoopAssign) {
                    Value midVal = new Value(value.getType(), value.getName() + "_temp");
                    MoveInstr moveInstr = new MoveInstr(midVal, value);
                    midValue.add(moveInstr);
                    for (MoveInstr moveInstr1 : moveInstrs) {
                        if (moveInstr1.getSrc().equals(value)) {
                            moveInstr1.setSrc(midVal);
                        }
                    }
                }
            }
        }

        //TODO:move中的寄存器冲突
        for (MoveInstr moveInstr : midValue) {
            moveInstrs.addFirst(moveInstr);
        }
        return moveInstrs;
    }
}
