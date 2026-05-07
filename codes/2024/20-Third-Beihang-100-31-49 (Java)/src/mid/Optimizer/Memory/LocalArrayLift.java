package mid.Optimizer.Memory;

import mid.IntermediatePresentation.*;
import mid.IntermediatePresentation.Array.ArrayInitializer;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.Instruction.*;
import mid.IntermediatePresentation.Module;
import mid.SymbolTable.SymbolTableManager;


import java.util.ArrayList;
import java.util.HashMap;

public class LocalArrayLift {

    private final Module module;
    private final SymbolTableManager symbolTableManager;

    public LocalArrayLift() {
        module = IRManager.getModule();
        symbolTableManager = SymbolTableManager.getInstance();
    }

    public void optimize() {
        for (Function function : module.getDecledFunctions()) {
            if (function.getName().equals("main"))  //todo:or 调用该函数的函数数量getCaller == 1
            {
                for (BasicBlock bb : function.getBlocks()) {
                    if (bb.getLoopDepth() != 0) {
                        continue;
                    }
                    ArrayList<Instruction> instList = bb.getInstructionList();
                    /*
                    因为变量声明不是顺序的，LAL时不能依次顺序处理，所以存了很多额外信息:(
                    某个优化会加新的数组，需要注意
                     */
                    HashMap<String, Integer> aGEPCnt = new HashMap<>();     // 待处理的GEP次数
                    HashMap<String, ArrayInitializer> aInitializerMap = new HashMap<>();    // 初始化数组
                    HashMap<String, Alloca> aAllocMap = new HashMap<>();    // 待替换的alloca
                    ArrayList<GetElementPtr> gepToDel = new ArrayList<>();
                    ArrayList<Store> storeToDel = new ArrayList<>();
                    ArrayList<Call> callToDel = new ArrayList<>();
                    for (int i = 0; i < instList.size(); i++) {
                        Instruction inst = instList.get(i);
                        if (inst instanceof Alloca alloca) {
                            if (alloca.getType().equals(ValueType.ARRAY)) {
                                if (!alloca.getRefType().equals(ValueType.I32)) {
                                    continue;
                                }
                                aGEPCnt.put(alloca.getName(), alloca.getLen());
                                aInitializerMap.put(alloca.getName(), new ArrayInitializer(alloca.getLen()));
                                aAllocMap.put(alloca.getName(), alloca);
                            }
                        }
                        // 处理GEP与STORE/MEMSET
                        else if (inst instanceof GetElementPtr gep) {
                            String ptr = gep.getPtr().getName();
                            if (!aGEPCnt.containsKey(ptr) || aGEPCnt.get(ptr) == 0) {
                                continue;
                            }
                            aGEPCnt.compute(ptr, (key, cnt) -> (cnt == null) ? 0 : cnt - 1);
                            Instruction nextInst = instList.get(i + 1);
                            // nextInst也可能为load，不过不需要处理
                            if (nextInst instanceof Store store) {
                                if (!(store.getSrc() instanceof ConstNumber)) {
                                    aGEPCnt.remove(ptr);
                                    gepToDel.remove(gep);
                                    continue;
                                }
                                ArrayInitializer aInitializer = aInitializerMap.get(ptr);
                                int index = Integer.parseInt(gep.getElemIndex().getReg());
                                aInitializer.setVal(index, store.getSrc());
                                gepToDel.add(gep);
                                storeToDel.add(store);
                            } else if (nextInst instanceof Call call && call.getCallingFunction() instanceof LibFunction.Memset) {
                                aGEPCnt.put(ptr, 0);
                                gepToDel.add(gep);
                                callToDel.add(call);
                            }
                        }
                    }
                    //call和store要先于gep删除，如果先删gep，删store时会找不到store的addr，use-def出错
                    for (Call call : callToDel) {
                        bb.removeInstruction(call);
                        call.destroy();
                    }
                    for (Store store : storeToDel) {
                        bb.removeInstruction(store);
                        store.destroy();
                    }
                    for (GetElementPtr gep : gepToDel) {
                        if (gep.getUserList().size() == 0) {
                            bb.removeInstruction(gep);
                            gep.destroy();
                        }
                    }
                    for (String key : aGEPCnt.keySet()) {
                        String name = "@lift_" + key.replace("%", "");
                        GlobalDecl globalDecl = new GlobalDecl(aInitializerMap.get(key).withType(true), true);
                        globalDecl.rename(name);
                        symbolTableManager.setIRValue(name, globalDecl);
                        Alloca alloca = aAllocMap.get(key);
                        alloca.beReplacedBy(globalDecl);
                        alloca.destroy();
                    }
                }
            }
        }

        liftForStack();
    }

    public void liftForStack() {
        // 不带初始化的提升，就不用担心多次调用不一致的问题了，而且除非主动传参，否则这个地址也只能自己访问
        // 唯一的问题是递归的时候，回到原函数时会出问题，不能是递归就ok了
        for (Function f : IRManager.getModule().getDecledFunctions()) {
            if (f.getBlocks().stream().anyMatch(b ->
                    b.getInstructionList().stream().anyMatch(
                            i -> i instanceof Call call && call.getCallingFunction().equals(f)
                    )
            )) {
                continue;
            }
            BasicBlock entry = f.getEntranceBlock();
            for (Instruction i : new ArrayList<>(entry.getInstructionList())) {
                if (!(i instanceof Alloca alloca)) {
                    break;
                }
                if (alloca.getType().equals(ValueType.ARRAY)) {
                    int len = alloca.getLen();
                    boolean isInt = !alloca.isFloat();
                    GlobalDecl globalDecl = new GlobalDecl(new ArrayInitializer(len), isInt, false);
                    alloca.beReplacedBy(globalDecl);
                    alloca.destroy();
                    entry.removeInstruction(alloca);
                }
            }
        }
    }
}
