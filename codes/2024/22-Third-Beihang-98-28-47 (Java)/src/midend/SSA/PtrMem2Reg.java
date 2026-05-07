package midend.SSA;

import frontend.ir.FuncDef;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.GlobalObject;
import frontend.ir.structure.Procedure;

import java.util.*;

/**
 * 针对指针的 mem2reg，可以认为是在做别名分析
 * 现在的想法是按照控制树 dfs，保存对于一个地址的赋值记录，如果 dom 分叉（直接控制块不止一个）则清空当前列表，以避免多重定义导致冲突。
 * 对于 call 需要特殊处理，函数中可能出现修改指针指向内存内容的操作。
 * 还有一个前置任务是要合并 GEP，避免相同指针的表现形式不同导致错误删除和不当替换
 * 对于只定义过一次的值可以直接替换
 * 增加一些简单的针对全局非数组变量的分析和替换，暂时只针对仅赋值一次的情况
 */
public class PtrMem2Reg {
    private static HashSet<String> gepHashUsed;
    private static HashSet<Value> rootsIndefinitelyUsed; // 这些基址对应的任何一个指针都有可能被使用
    private static HashSet<Value> rootsDefinitelyUsed;   // 这些基址对应的某些特定指针会被使用
    private static HashMap<String, StoreInstr> singleDefVal;
    private static HashMap<GlobalObject, Value> singleDefGlbObj;
    
    public static void execute(ArrayList<Function> functions) {
        if (functions == null) {
            throw new NullPointerException();
        }
        
        gepHashUsed = new HashSet<>();
        rootsIndefinitelyUsed = new HashSet<>();
        rootsDefinitelyUsed = new HashSet<>();
        singleDefVal = new HashMap<>();
        singleDefGlbObj = new HashMap<>();
        
        searchOnlyDef(functions);
        
        for (Function function : functions) {
            BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
            dfsIdoms(basicBlock, new HashMap<>(), new HashMap<>(), new HashSet<>(), new HashMap<>());
        }
        
        removeUselessStore(functions);
    }
    
    private static void searchOnlyDef(List<Function> functions) {
        HashMap<String, GEPInstr> keyMap = new HashMap<>();
        singleDefVal.clear();
        singleDefGlbObj.clear();
        for (Function function : functions) {
            BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
            while (basicBlock != null) {
                Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
                while (instruction != null) {
                    if (instruction instanceof CallInstr) {
                        List<Value> rParams = ((CallInstr) instruction).getRParams();
                        for (Value rp : rParams) {
                            if (rp instanceof GEPInstr) {
                                Value root = ((GEPInstr) rp).getRoot();
                                Iterator<String> it = keyMap.keySet().iterator();
                                while (it.hasNext()) {
                                    String key = it.next();
                                    GEPInstr gepKey = keyMap.get(key);
                                    if (gepKey != null && gepKey.getRoot().equals(root)) {
                                        singleDefVal.put(key, null);
                                        it.remove();
                                    }
                                }
                            }
                        }
                    } else if (instruction instanceof StoreInstr) {
                        Value ptr = ((StoreInstr) instruction).getPtr();
                        if (ptr instanceof GEPInstr) {
                            // root 只能是 AllocaInstr，GlobalObject，FParam 三种
                            if (((GEPInstr) ptr).hasNonConstIndex()) {
                                Value root = ((GEPInstr) ptr).getRoot();
                                Iterator<String> it = keyMap.keySet().iterator();
                                while (it.hasNext()) {
                                    String key = it.next();
                                    GEPInstr gepKey = keyMap.get(key);
                                    if (gepKey != null && gepKey.getRoot().equals(root)) {
                                        singleDefVal.put(key, null);
                                        it.remove();
                                    }
                                }
                            } else {
                                String key = ((GEPInstr) ptr).myHash();
                                if (singleDefVal.containsKey(key)) {
                                    singleDefVal.put(key, null);
                                    keyMap.remove(key);
                                } else {
                                    singleDefVal.put(key, (StoreInstr) instruction);
                                    keyMap.put(key, (GEPInstr) ptr);
                                }
                            }
                        } else if (ptr instanceof GlobalObject) {
                            if (singleDefGlbObj.containsKey(ptr)) {
                                singleDefGlbObj.put((GlobalObject) ptr, null);
                            } else {
                                singleDefGlbObj.put((GlobalObject) ptr, ((StoreInstr) instruction).getValue());
                            }
                        } else {
                            throw new RuntimeException("这里 load 和 store 的指针应该只能是 gep 或者全局对象");
                        }
                    }
                    instruction = (Instruction) instruction.getNext();
                }
                basicBlock = (BasicBlock) basicBlock.getNext();
            }
        }
    }
    
    private static void dfsIdoms(BasicBlock basicBlock, HashMap<String, Value> defMap,
                                 HashMap<String, GEPInstr> keyMap, HashSet<Instruction> done,
                                 HashMap<GlobalObject, Value> glbObjMap) {
        if (basicBlock == null) {
            throw new NullPointerException();
        }
        Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
        while (instruction != null) {
            
            OIS.simpleOIS4ins(instruction);
            
            if (instruction instanceof CallInstr) {
                List<Value> rParams = ((CallInstr) instruction).getRParams();
                for (Value rp : rParams) {  // todo: 全局数组似乎没考虑
                    if (rp instanceof GEPInstr) {
                        Value root = ((GEPInstr) rp).getRoot();
                        rootsIndefinitelyUsed.add(root);
                        clearSameRoot(root, defMap, keyMap);
                    }
                }
                Iterator<GlobalObject> it = glbObjMap.keySet().iterator();
                while (it.hasNext()) {
                    GlobalObject globalObject = it.next();
                    if (checkFuncStoreGlbObj(((CallInstr) instruction).getFuncDef(), globalObject)) {
                        it.remove();
                    }
                }
            } else if (instruction instanceof StoreInstr) {
                Value ptr = ((StoreInstr) instruction).getPtr();
                if (ptr instanceof GEPInstr) {
                    // root 只能是 AllocaInstr，GlobalObject，FParam 三种
                    Value root = ((GEPInstr) ptr).getRoot();
                    if (((GEPInstr) ptr).hasNonConstIndex()) {
                        clearSameRoot(root, defMap, keyMap);
                    }
                    String key = ((GEPInstr) ptr).myHash();
                    defMap.put(key, ((StoreInstr) instruction).getValue());
                    keyMap.put(key, (GEPInstr) ptr);
                } else if (ptr instanceof GlobalObject) {
                    glbObjMap.put((GlobalObject) ptr, ((StoreInstr) instruction).getValue());
                } else {
                    throw new RuntimeException("这里 load 和 store 的指针应该只能是 gep 或者全局对象");
                }
            } else if (instruction instanceof LoadInstr) {
                Value ptr = ((LoadInstr) instruction).getPtr();
                if (ptr instanceof GEPInstr) {
                    Value defVal;
                    StoreInstr onlyDef;
                    String key = ((GEPInstr) ptr).myHash();
                    onlyDef = singleDefVal.get(key);
                    if (onlyDef == null) {
                        defVal = defMap.get(key);
                    } else {
                        // 这里是为了避免 load 出现在第一次显示 store 之前（这种情况下应该用 0）
                        if (done.contains(onlyDef)) {
                            defVal = onlyDef.getValue();
                        } else {
                            defVal = ConstInt.Zero;
                        }
                    }
                    if (defVal != null) {
                        instruction.replaceUseTo(defVal);
                        instruction.removeFromList();
                    } else {
                        defMap.put(key, instruction);
                        keyMap.put(key, (GEPInstr) ptr);
                        if (((GEPInstr) ptr).hasNonConstIndex()) {
                            rootsIndefinitelyUsed.add(((GEPInstr) ptr).getRoot());
                        } else {
                            gepHashUsed.add(((GEPInstr) ptr).myHash());
                            rootsDefinitelyUsed.add(((GEPInstr) ptr).getRoot());
                        }
                    }
                } else if (ptr instanceof GlobalObject) {
                    if (glbObjMap.containsKey(ptr) && singleDefGlbObj.get(ptr) != null) {
                        instruction.replaceUseTo(glbObjMap.get(ptr));
                        instruction.removeFromList();
                    }
                } else {
                    throw new RuntimeException("这里 load 和 store 的指针应该只能是 gep 或者全局对象");
                }
            }
            
            done.add(instruction);
            instruction = (Instruction) instruction.getNext();
        }
        
        HashSet<BasicBlock> idoms = basicBlock.getIDoms();
        HashMap<String, Value> nextDefMap = null;
        HashMap<String, GEPInstr> nextKeyMap = null;
        if (idoms.isEmpty()) {
            return;
        } else if (idoms.size() == 1) {
            boolean onlyYou = true;
            for (BasicBlock idom : idoms) {
                HashSet<BasicBlock> pres = idom.getPres();
                for (BasicBlock pre : pres) {
                    if (!pre.equals(basicBlock)) {
                        onlyYou = false;
                        break;
                    }
                }
            }
            if (onlyYou) {
                nextDefMap = defMap;
                nextKeyMap = keyMap;
            }
        }
        
        HashMap<GlobalObject, Value> nextGlbObjMap = new HashMap<>();
        for (GlobalObject glbObj : glbObjMap.keySet()) {
            boolean objStored = false;
            for (BasicBlock idom : idoms) {
                if (checkBlkStoreGlbObj(idom, glbObj)) {
                    objStored = true;
                    break;
                }
            }
            if (!objStored) {
                nextGlbObjMap.put(glbObj, glbObjMap.get(glbObj));
            }
        }
        
        
        for (BasicBlock next : idoms) {
            if (nextDefMap == null) {
                dfsIdoms(next, new HashMap<>(), new HashMap<>(), new HashSet<>(done), new HashMap<>(nextGlbObjMap));
            } else {
                dfsIdoms(next, nextDefMap, nextKeyMap, new HashSet<>(done), glbObjMap); // 如果只有唯一后继用自己的容器就行
            }
        }
    }
    
    /**
     * 针对非数组全局对象，如果在对应函数中会被定义，则不能做过于激进的操作
     */
    private static boolean checkFuncStoreGlbObj(FuncDef funcDef, GlobalObject globalObject) {
        if (funcDef instanceof Function) {
            BasicBlock blk = (BasicBlock) ((Function) funcDef).getBasicBlocks().getHead();
            while (blk != null) {
                if (checkBlkStoreGlbObj(blk, globalObject)) {
                    return true;
                }
                blk = (BasicBlock) blk.getNext();
            }
        }
        return false;
    }
    
    private static boolean checkBlkStoreGlbObj(BasicBlock blk, GlobalObject globalObject) {
        Instruction ins = (Instruction) blk.getInstructions().getHead();
        while (ins != null) {
            if (ins instanceof CallInstr) {
                Function parentFunc = ((Procedure) blk.getParent().getOwner()).getParentFunc();
                if (!((CallInstr) ins).getFuncDef().equals(parentFunc)) {
                    // 对于递归的情况，不用继续往下检查，当前层没有下面也没有，因此只需要检查非递归情形
                    if (checkFuncStoreGlbObj(((CallInstr) ins).getFuncDef(), globalObject)) {
                        return true;
                    }
                }
            } else if (ins instanceof StoreInstr) {
                if (((StoreInstr) ins).getPtr().equals(globalObject)) {
                    return true;
                }
            }
            ins = (Instruction) ins.getNext();
        }
        return false;
    }
    
    private static void clearSameRoot(Value root, HashMap<String, Value> defMap,
                                      HashMap<String, GEPInstr> keyMap) {
        Iterator<String> it = defMap.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            if (keyMap.get(key).getRoot().equals(root)) {
                it.remove();
                keyMap.remove(key);
            }
        }
    }
    
    private static void removeUselessStore(List<Function> functions) {
        for (Function function : functions) {
            BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
            while (basicBlock != null) {
                Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
                while (instruction != null) {
                    if (instruction instanceof StoreInstr) {
                        Value ptr = ((StoreInstr) instruction).getPtr();
                        if (ptr instanceof GEPInstr) {
                            boolean mightBeUsed = (rootsDefinitelyUsed.contains(((GEPInstr) ptr).getRoot())
                                    && ((GEPInstr) ptr).hasNonConstIndex());
                            if (!gepHashUsed.contains(((GEPInstr) ptr).myHash()) &&
                                    !rootsIndefinitelyUsed.contains(((GEPInstr) ptr).getRoot()) &&
                                    !mightBeUsed) {
                                instruction.removeFromList();
                            }
                        } else if (!(ptr instanceof GlobalObject)) {
                            throw new RuntimeException("这里 load 和 store 的指针应该只能是 gep 或者全局对象");
                        }
                    }
                    instruction = (Instruction) instruction.getNext();
                }
                basicBlock = (BasicBlock) basicBlock.getNext();
            }
        }
    }
}
