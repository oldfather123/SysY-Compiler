package midend;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.objecttype.arithmetic.ArithmeticType;
import frontend.ir.objecttype.derived.TArray;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.*;
import java.util.List;

public class Mem2Reg {
    private static final HashMap<PhiInstr, AllocaInstr> newPhis = new HashMap<>();
    private static final HashSet<AllocaInstr> corrAllocaSet = new HashSet<>();    // corresponding alloca instr set
    
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            placePhi(function);
            rename(function);
            removeCorrespondingAlloca();
        }
    }
    
    /**
     * 删去本次处理掉的 alloca
     * 这个行为其实可以在 rename 环节处理掉，但是由于设计了一个指令必须删去所有使用者才能删去本身，在那边删不方便
     */
    private static void removeCorrespondingAlloca() {
        for (AllocaInstr alloca : corrAllocaSet) {
            alloca.removeFromList();
        }
    }
    
    private static void rename(Function function) {
        Set<BasicBlock> visited = new HashSet<>();
        Queue<BasicBlock> blkQueue = new LinkedList<>();    // 为下面那个 map 服务的，map 不方便每次去一个这种行为
        Map<BasicBlock, Map<AllocaInstr, Value>> blk2incomingValMap = new HashMap<>();
        
        blkQueue.add(function.getFirstBlk());
        blk2incomingValMap.put(function.getFirstBlk(), new HashMap<>());
        while (!blkQueue.isEmpty()) {
            BasicBlock curBasicBlock = blkQueue.poll();
            Map<AllocaInstr, Value> incomingValMap = blk2incomingValMap.get(curBasicBlock);
            blk2incomingValMap.remove(curBasicBlock);
            
            if (visited.contains(curBasicBlock)) {
                continue;
            }
            visited.add(curBasicBlock);
            
            Instruction instruction = curBasicBlock.getFirstInstr();
            while (instruction != null) {
                if (instruction instanceof LoadInstr load) {
                    if (load.getPointer() instanceof AllocaInstr ptr && corrAllocaSet.contains(ptr)) {
                        Value curVal = incomingValMap.get(ptr);
                        if (ptr.getReferencedType() instanceof ArithmeticType arithmeticType) {
                            instruction.replaceUseWith(
                                    curVal == null ? arithmeticType.getDefaultValue() : curVal
                            );
                        } else if (ptr.getReferencedType() instanceof TPointer pointerType){
                            instruction.replaceUseWith(
                                    curVal == null ? pointerType.getDefaultValue() : curVal);
                        }

                        instruction.removeFromList();
                    }
                } else if (instruction instanceof StoreInstr store) {
                    if (store.getPointer() instanceof AllocaInstr ptr && corrAllocaSet.contains(ptr)) {
                        incomingValMap.put(ptr, store.getValueToStore());
                        instruction.removeFromList();
                    }
                } else if (instruction instanceof PhiInstr phi) {
                    if (newPhis.containsKey(phi)) {
                        incomingValMap.put(newPhis.get(phi), phi);
                    }
                }
                instruction = (Instruction) instruction.getNext();
            }
            
            // update phis in successors of cbb
            for (BasicBlock suc : curBasicBlock.getSucs()) {
                if (blk2incomingValMap.containsKey(suc)) {
                    blk2incomingValMap.get(suc).putAll(incomingValMap);
                } else {
                    blkQueue.add(suc);
                    blk2incomingValMap.put(suc, new HashMap<>(incomingValMap));
                }
                
                Instruction ins = suc.getFirstInstr();
                while (ins instanceof PhiInstr phi) {
                    if (newPhis.containsKey(phi)) {
                        AllocaInstr allocaInstr = newPhis.get(phi);
                        Value value = incomingValMap.get(allocaInstr);
                        if (value == null) {
                            // fixme: 补丁行为，如果一个值在流入一个基本块的时候是不确定的，不确定的方向赋缺省值 0
                            //  其实这个基本不会发生，因为在正确的情况下，如果流入的时候不确定，这个值就一定要先定义再使用了
                            //  我觉得这种 phi 后续都会删掉
                            if (allocaInstr.getReferencedType() instanceof ArithmeticType arithmeticType) {
                                value = arithmeticType.getDefaultValue();
                            } else if (allocaInstr.getReferencedType() instanceof TPointer pointerType) {
                                value = pointerType.getDefaultValue();
                            } else {
                                throw new RuntimeException("这里只能处理算术类型和指针类型的情况");
                            }
                            
                        }
                        phi.addOperand(curBasicBlock, value);
                    }
                    ins = (Instruction) ins.getNext();
                }
            }
        }
    }
    
    private static void placePhi(Function function) {
        newPhis.clear();
        corrAllocaSet.clear();
        Instruction ins = function.getFirstBlk().getFirstInstr();
        Set<BasicBlock> visited = new HashSet<>();
        Queue<BasicBlock> defBlkQueue = new LinkedList<>();
        
        while (ins instanceof AllocaInstr alloca) {
            if (alloca.getReferencedType() instanceof TArray) {
                ins = (Instruction) ins.getNext();
                continue;
            }
            corrAllocaSet.add(alloca);
            
            visited.clear();
            defBlkQueue.clear();
            for (Value user : ins.getUserList()) {
                if (user instanceof StoreInstr def) {
                    defBlkQueue.add(def.getParentBB());
                }
            }
            
            while (!defBlkQueue.isEmpty()) {
                BasicBlock defBlock = defBlkQueue.poll();
                for (BasicBlock df : defBlock.getDominanceFrontier()) {
                    if (visited.contains(df)) {
                        continue;
                    }
                    visited.add(df);
                    PhiInstr phi = new PhiInstr(alloca.getReferencedType(), function.getAndAddRegIdx(), df);
                    df.insertInsToHead(phi);
                    newPhis.put(phi, alloca);
                    defBlkQueue.add(df);
                }
            }
            
            ins = (Instruction) ins.getNext();
        }
    }
}
