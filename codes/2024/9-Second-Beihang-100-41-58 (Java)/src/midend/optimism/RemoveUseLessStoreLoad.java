package midend.optimism;

import midend.*;
import midend.Module;
import midend.analysis.AliasAnalysis;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.*;

import java.util.ArrayList;

public class RemoveUseLessStoreLoad {
    private Module module;

    public RemoveUseLessStoreLoad(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            LoopInfo.loopAnalysis(function);
            instructions.clear();
            //initCallInstr(function);
            simplifyLoad(function.getBlockList().getFirst());
        }
    }

    public void initCallInstr(Function function) {
        for (BasicBlock block : function.getBlockList()) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof CallInstr) {
                    callInstrs.add((CallInstr) instruction);
                }
            }
        }
    }

    public boolean callSideEffect(Instruction instruction0, Value pointer, BasicBlock block) {
        if (pointer instanceof GetElementPtrInstr) {
            for (Loop loop : LoopInfo.topLevelLoop) {

                for (BasicBlock block1 : loop.getBasicBlocks()) {
                    for (Instruction instruction : block1.getInstrList()) {
                        if (instruction instanceof CallInstr &&
                                (instruction.getValueList().contains(pointer))) {
                            return true;
                        } else if (!instruction.equals(instruction0) && instruction instanceof StoreInstr
                                && AliasAnalysis.isSameRoot(pointer, ((StoreInstr) instruction).getPointer()) && AliasAnalysis.isSame(pointer, ((StoreInstr) instruction).getPointer())) {
                            instructions.remove(instruction0);
                            temp--;
                            return true;
                        }
                    }
                }
            }

        } else if (pointer instanceof GlobalVar) {
            return true;
        }
        return false;
    }

    private ArrayList<CallInstr> callInstrs = new ArrayList<>();
    private ArrayList<Instruction> instructions = new ArrayList<>();
    private int temp = 0;

    public void simplifyLoad(BasicBlock block) {
        ArrayList<Instruction> inserted = new ArrayList<>();
        for (Instruction instruction : block.getInstrList()) {
            if (instruction instanceof StoreInstr || instruction instanceof LoadInstr || instruction instanceof CallInstr) {
                instructions.add(instruction);
                inserted.add(instruction);
            }
        }
        for (int count = 0; count < instructions.size() - 1; count++) {
            int nowSize = instructions.size();
            Instruction instruction = instructions.get(count);
            temp = count;
            Instruction after = instructions.get(++temp);
            if (instruction instanceof StoreInstr) {
                while (temp < instructions.size() && !(after instanceof StoreInstr && ((StoreInstr) after).getPointer() == ((StoreInstr) instruction).getPointer())) {
                    if ((after instanceof CallInstr && after.getValueList().contains(((StoreInstr) instruction).getPointer())) || (after instanceof CallInstr && ((CallInstr) after).getFunction().isSideEffect())) {
                        break;
                    }
//                    if (after instanceof StoreInstr && ((StoreInstr) after).getPointer().getName().equals("%reg379") && ((StoreInstr) instruction).getPointer().getName().equals("%reg160")) {
//                        System.out.println("");
//                    }
//                    if (after.getName().equals("%reg511") && ((StoreInstr) instruction).getPointer().getName().equals("%reg1009")) {
//                        System.out.println("");
//                    }
                    if (after instanceof StoreInstr && AliasAnalysis.isSameRoot(((StoreInstr) after).getPointer(), ((StoreInstr) instruction).getPointer()) && AliasAnalysis.isSame(((StoreInstr) after).getPointer(), ((StoreInstr) instruction).getPointer())) {
                        break;
                    }
                    if (after instanceof LoadInstr && ((LoadInstr) after).getValue() == ((StoreInstr) instruction).getPointer()
                            && !callSideEffect(instruction, ((StoreInstr) instruction).getPointer(), after.getBasicBlock())) {
                        after.replaceUse(((StoreInstr) instruction).getValue());
                        after.remove();
                    }
                    if (temp == instructions.size() - 1) {
                        break;
                    }
                    after = instructions.get(++temp);
                }
                if (instructions.size() != nowSize) {
                    count -= nowSize - instructions.size();
                }
            }
        }

        for (BasicBlock block1 : block.getChildBlock()) {
            simplifyLoad(block1);
        }

        for (Instruction instruction : inserted) {
            instructions.remove(instruction);
        }
    }
}
