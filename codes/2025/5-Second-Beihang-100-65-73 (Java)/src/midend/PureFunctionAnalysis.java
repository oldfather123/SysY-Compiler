package midend;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.convop.Bitcast;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.AtomaticAddInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.GlbObjPtr;

import java.util.*;

import static midend.FunctionInfoCollector.getFuncInfo;

/**
 * 前置pass：DeadArgumentElimination
 */

public class PureFunctionAnalysis {
    private static Map<Function, Set<Function>> callGraph = new HashMap<>();
    private static final HashSet<Function> visitedFunctionSet = new HashSet<>();
    private static final HashSet<Function> parallelFunctionPtrSet = new HashSet<>();

    public static void execute(List<Function> functions) {
        visitedFunctionSet.clear();

        callGraph = buildCallGraph(functions);

        ArrayList<Function> funcTopoSort = topoSortFunctions(functions.getLast());
        deadFunctionElimination(functions);

        analyzeFunctions(funcTopoSort);
    }


    private static Map<Function, Set<Function>> buildCallGraph(List<Function> functions) {
        Map<Function, Set<Function>> callGraph = new HashMap<>();

        for (Function caller : functions) {
            for (BasicBlock block : caller.getBasicBlockList()) {
                for (Instruction instruction : block.getInstrList()) {
                    if (instruction instanceof CallInstr callInstr) {
                        Function callee = callInstr.getCallee();
                        if (callee != null) {
                            callGraph.computeIfAbsent(caller, _ -> new HashSet<>()).add(callee);
                        }
                        if (callee.getName().equals("OHMParallelFor")) {
                            for (Value rParam : callInstr.getRParams()) {
                                if (rParam instanceof Function funcParam) {
                                    callGraph.computeIfAbsent(callee, _ -> new HashSet<>()).add(funcParam);
                                    parallelFunctionPtrSet.add(funcParam);
                                    getFuncInfo(callee).setParallelFunctionPtrSet(parallelFunctionPtrSet);
                                }
                            }
                        }
                    }
                }
            }
        }

        return callGraph;
    }

    private static ArrayList<Function> topoSortFunctions(Function mainFunction) {
        visitedFunctionSet.clear();
        ArrayList<Function> sorted = new ArrayList<>();
        HashSet<Function> onStack = new HashSet<>();

        if (!dfs(mainFunction, onStack, sorted)) {
            throw new RuntimeException("Call graph contains mutual recursion");
        }

        return sorted;
    }

    private static boolean dfs(Function function, Set<Function> onStack, List<Function> result) {
        visitedFunctionSet.add(function);
        onStack.add(function);

        for (Function callee : callGraph.getOrDefault(function, Collections.emptySet())) {
            if (callee.equals(function)) continue; // 允许自递归

            if (!visitedFunctionSet.contains(callee)) {
                if (!dfs(callee, onStack, result)) {
                    return false;
                }
            } else if (onStack.contains(callee)) {
                return false;
            }
        }

        onStack.remove(function);
        result.add(function);
        return true;
    }

    private static void deadFunctionElimination(List<Function> functions) {
        Iterator<Function> it = functions.iterator();
        while (it.hasNext()) {
            Function function = it.next();
            if (!visitedFunctionSet.contains(function)) {
                function.removeFromList();
                it.remove();
            }
        }
    }

    /**
     * 当前函数的状态
     */
    private static boolean isExternRead = false;
    private static boolean isExternWrite = false;
    private static boolean isGlobalWrite = false;
    private static boolean isParamWrite = false;
    private static boolean isGlobalScalarRead = false;
    private static boolean isGlobalScalarWrite = false;
    private static boolean isAlloc = false;
    private static boolean isIO = false;
    private static boolean isRecur = false;
    private static Set<Value> mayWrite = new HashSet<>();
    private static Set<Value> mayRead = new HashSet<>();

    private static boolean isMemoryLibFunc(Function.LibFunc libFunc) {
        if (libFunc == Function.LibFunc.MEMSET || libFunc.getName().startsWith("get")) return true;
        if (libFunc.getName().startsWith("put") || libFunc.getName().startsWith("start")) return true;
        return false;
    }

    private static void analyzeFunctions(List<Function> sorted) {
        for (Function func : sorted) {
            if (func instanceof Function.LibFunc libFunc) {
                FunctionInfo funcInfo = getFuncInfo(libFunc);
                if (libFunc == Function.LibFunc.MEMSET || libFunc == Function.LibFunc.GETARRAY || libFunc == Function.LibFunc.GETFARRAY) {
                    funcInfo.setExternWrite(true);
                    funcInfo.setParamWrite(true);
                    mayWrite = new HashSet<>();
                    funcInfo.setMayWrite(mayWrite);
                } else {
                    funcInfo.setIO(true);
                    if (libFunc.getName().startsWith("put") || libFunc.getName().startsWith("start")) {
                        funcInfo.setExternRead(true);
                        mayRead = new HashSet<>();
                        for (Function.FParam fParam : libFunc.getFParams()) {
                            mayRead.add(fParam);
                        }
                        funcInfo.setMayRead(mayRead);
                    }
                }
                continue;
            }

            isExternRead = false;
            isExternWrite = false;
            isGlobalWrite = false;
            isParamWrite = false;
            isGlobalScalarRead = false;
            isGlobalScalarWrite = false;
            isAlloc = false;
            isIO = false;
            isRecur = false;
            mayWrite = new HashSet<>();
            mayRead = new HashSet<>();

            for (BasicBlock block : func.getBasicBlockList()) {
                for (Instruction instr : block.getInstrList()) {
                    if (instr instanceof CallInstr callInstr) {
                        if (callInstr.getCallee() == func) {
                            isRecur = true;
                        } else {
                            // OHMParallelFor在此处随便其传播，仅在FunctionInfo判断的时候特殊处理
                            // 传播
                            Function callee = callInstr.getCallee();
                            FunctionInfo calleeInfo = getFuncInfo(callee);

                            isAlloc |= calleeInfo.isAlloc(callInstr);
                            isIO |= calleeInfo.isIO(callInstr);

                            isGlobalScalarRead |= calleeInfo.isGlobalScalarRead;
                            isGlobalScalarWrite |= calleeInfo.isGlobalScalarWrite;


                            // 内存相关参数：若callee是库函数，先检查实参，再决定是否传播
                            if (callee instanceof Function.LibFunc libFunc && isMemoryLibFunc(libFunc)) {
                                for (Value rParam : callInstr.getRParams()) {
                                    Value baseAddr = getBaseAddr(rParam);
                                    if (baseAddr instanceof GlbObjPtr && calleeInfo.isExternWrite(callInstr)) {
                                        isExternWrite = true;
                                        isGlobalWrite = true;
                                        mayWrite.add(baseAddr); // todo ? 和store存储的方式有点不同
                                    } else if (baseAddr instanceof Function.FParam && calleeInfo.isExternWrite(callInstr)) {
                                        isExternWrite = true;
                                        isParamWrite = true;
                                        mayWrite.add(baseAddr); // 同上
                                    } else if ((baseAddr instanceof GlbObjPtr || baseAddr instanceof Function.FParam) && calleeInfo.isExternRead(callInstr)) {
                                        isExternRead = true;
                                        mayRead.add(baseAddr); // 同上
                                    }
                                }
                            }
                            // 非内存库函数和外部函数：照常传播
                            else {
                                mayWrite.addAll(calleeInfo.getMayWrite(callInstr));
                                mayRead.addAll(calleeInfo.getMayRead(callInstr));
                                isExternRead |= calleeInfo.isExternRead(callInstr);
                                isExternWrite |= calleeInfo.isExternWrite(callInstr);
                                isGlobalWrite |= calleeInfo.isGlobalWrite(callInstr);
                                isParamWrite |= calleeInfo.isParamWrite(callInstr);

                                isGlobalScalarRead |= calleeInfo.isGlobalScalarRead;
                                isGlobalScalarWrite |= calleeInfo.isGlobalScalarWrite;

                            }


                        }
                    } else if (instr instanceof AllocaInstr) {
                        isAlloc = true;
                    } else if (instr instanceof AtomaticAddInstr atomicAdd) {
                        Value atomicAddr = atomicAdd.getPtr();
                        // 既load又store
                        isExternRead |= findReadExternStatus(atomicAddr);
                        if (isExternRead) mayRead.add(atomicAddr);
                        isExternWrite |= findWriteExternStatus(atomicAddr);
                        if (isExternWrite) mayWrite.add(atomicAddr);
                    } else if (instr instanceof StoreInstr storeInstr) {
                        Value storeAddr = storeInstr.getPointer();
                        isExternWrite |= findWriteExternStatus(storeAddr);
                        if (isExternWrite) mayWrite.add(storeAddr);
                    } else if (instr instanceof LoadInstr loadInstr) {
                        Value loadAddr = loadInstr.getPointer();
                        isExternRead |= findReadExternStatus(loadAddr);
                        if (isExternRead) mayRead.add(loadAddr);
                    }

                    if (isExternWrite && isExternRead && isAlloc && isIO && isRecur) {
                        break;
                    }
                }
                if (isExternWrite && isExternRead && isAlloc && isIO && isRecur) {
                    break;
                }
            }

            FunctionInfo funcInfo = getFuncInfo(func);

            funcInfo.setExternRead(isExternRead);
            funcInfo.setExternWrite(isExternWrite);
            funcInfo.setGlobalWrite(isGlobalWrite);
            funcInfo.setMayWrite(mayWrite);
            funcInfo.setMayRead(mayRead);
            funcInfo.setParamWrite(isParamWrite);
            funcInfo.setAlloc(isAlloc);
            funcInfo.setIO(isIO);
            funcInfo.setRecur(isRecur);

            funcInfo.isGlobalScalarRead = isGlobalScalarRead;
            funcInfo.isGlobalScalarWrite = isGlobalScalarWrite;

        }
    }

    private static Value getBaseAddr(Value pointer) {
        Value addr = pointer;
        while (addr instanceof GEPInstr || addr instanceof Bitcast) {
            if (addr instanceof GEPInstr gep) {
                addr = gep.getPtrVal();
            }
            if (addr instanceof Bitcast bitcast) {
                addr = bitcast.getValue();
            }
        }
        return addr;
    }

    /**
     * 指令是否写外部状态（全局/指针参数）
     *
     * @param pointer：指令读写的位置
     * @return
     */
    private static boolean findWriteExternStatus(Value pointer) {
        Value addr = pointer;
        if (addr instanceof GlbObjPtr) {
            isGlobalWrite = true;

            isGlobalScalarWrite = true;

            return true;
        } else if (addr instanceof GEPInstr) {
            while (addr instanceof GEPInstr) {
                addr = ((GEPInstr) addr).getPtrVal();
            }
            if (addr instanceof GlbObjPtr) {
                isGlobalWrite = true;
                return true;
            } else if (addr instanceof Function.FParam) {
                isParamWrite = true;
                return true;
            }
        }
        return false;
    }

    /**
     * 指令是否写外部状态（全局/指针参数）
     *
     * @param pointer：指令读写的位置
     * @return
     */
    private static boolean findReadExternStatus(Value pointer) {
        Value addr = pointer;
        if (addr instanceof GlbObjPtr) {

            isGlobalScalarRead = true;

            return true;
        } else if (addr instanceof GEPInstr) {
            while (addr instanceof GEPInstr) {
                addr = ((GEPInstr) addr).getPtrVal();
            }
            if (addr instanceof GlbObjPtr) {
                return true;
            } else if (addr instanceof Function.FParam) {
                return true;
            }
        }
        return false;
    }

}
