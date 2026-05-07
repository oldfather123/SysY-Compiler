package midend;

import frontend.ir.Value;
import frontend.ir.instr.convop.Bitcast;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.GlbObjPtr;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class FunctionInfo {
    private final Function function;

    public FunctionInfo(Function function) {
        this.function = function;
    }

    /* 并行化 */
    private Set<Function> parallelFunctionPtrSet = new HashSet<>();

    /* 内存相关 */
    private boolean isExternRead = false;
    private Set<Value> mayRead = new HashSet<>();

    private boolean isExternWrite = false;
    private boolean isGlobalWrite = false;
    private boolean isParamWrite = false;
    private Set<Value> mayWrite = new HashSet<>();

    private boolean isAlloc = false;
    private boolean isIO = false;
    private boolean isRecur = false;
    public boolean isGlobalScalarRead = false;
    public boolean isGlobalScalarWrite = false;

    public boolean canGCM() {
        // GCM（全局代码移动）要求没有副作用，且不产生控制流依赖
        if (function.getName().equals("OHMParallelFor")) return false;
        return !isIO && !isExternRead && !isExternWrite && !isAlloc && !isRecur && !function.getName().equals("OHMParallelFor");
    }

    public boolean canGVN() {
        // GVN（全局值编号）要求：没有副作用，且相同输入保证相同输出
        if (function.getName().equals("OHMParallelFor")) return false;
        return !isIO && !isExternWrite && !isAlloc && !isExternRead && !isRecur && !function.getName().equals("OHMParallelFor");
    }

    public boolean canLiftInLICM() {
        if (function.getName().equals("OHMParallelFor")) return false;
        return !isExternRead && !isExternWrite;
    }

    public boolean isPureInRemoveDeadLoop(CallInstr call) {
        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).isPureInRemoveDeadLoop(null);
                    }
                }
            } else {
                return false; // call为null，认为其一定impure，保守处理
            }
        }
        return !isIO && !isExternWrite && !isExternRead;
    }

    public boolean canBeMemorizedBase() {
        return !isIO && !isExternWrite;
    }

    public boolean canGlobal2ParamTopLevel() {
        return !isGlobalScalarWrite && isGlobalScalarRead && isRecur;
    }

    public boolean canGlobal2Param() {
        return !isGlobalScalarWrite && isGlobalScalarRead;
    }

    /**
     * 仅供RemoveDeadFunction使用
     *
     * @return
     */
    public boolean isDead() {
        if (function.getName().equals("OHMParallelFor") || parallelFunctionPtrSet.contains(function)) {
            return false;
        }
        return !isIO && !isExternWrite;
    }

    public boolean isExternWrite(CallInstr call) {
        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).isExternWrite(null);
                    }
                }
            } else {
                return true; // call为null，认为其一定impure，保守处理
            }
        }
        return isExternWrite;
    }

    public boolean isGlobalWrite(CallInstr call) {
        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).isGlobalWrite(null);
                    }
                }
            } else {
                return true; // call为null，认为其一定impure，保守处理
            }
        }
        if (isGlobalWrite) return true;
        if (isParamWrite) {
            for (Value rParam : call.getRParams()) {
                if (getBase(rParam) instanceof GlbObjPtr) return true;
            }
        }
        return false;
    }

    private static Value getBase(Value val) {
        Value res = val;
        while (res instanceof GEPInstr || res instanceof Bitcast) {
            if (res instanceof GEPInstr gep) {
                res = gep.getPtrVal();
            }
            if (res instanceof Bitcast bitcast) {
                res = bitcast.getValue();
            }
        }
        return res;
    }

    public boolean isParamWrite(CallInstr call) {
        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).isParamWrite(null);
                    }
                }
            } else {
                return true; // call为null，认为其一定impure，保守处理
            }
        }
        return isParamWrite;
    }

    public boolean isExternRead(CallInstr call) {
        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).isExternRead(null);
                    }
                }
            } else {
                return true; // call为null，认为其一定impure，保守处理
            }
        }
        return isExternRead;
    }

    public boolean isExternRead() {
        return isExternRead;
    }

    public boolean isAlloc(CallInstr call) {
        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).isAlloc(null);
                    }
                }
            } else {
                return true; // call为null，认为其一定impure，保守处理
            }
        }
        return isAlloc;
    }

    public boolean isRecur() {
        return isRecur;
    }

    public boolean isIO(CallInstr call) {
        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).isIO(null);
                    }
                }
            } else {
                return true; // call为null，认为其一定impure，保守处理
            }
        }
        return isIO;
    }

    public Set<Value> getMayWrite(CallInstr call) {
        /*if (function instanceof Function.LibFunc libFunc && libFunc.getName().equals("memset")) {
            mayWrite.clear();
            Set<Value> mayWriteIncludeRealParam = new HashSet<>(mayWrite);
            for (Value rParam : call.getRParams()) {
                if (rParam instanceof GlbObjPtr || rParam.getType() instanceof TPointer) {
                    mayWriteIncludeRealParam.add(rParam);
                }
            }
            return mayWriteIncludeRealParam;
        }*/

        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).getMayWrite(null);
                    }
                }
            } else {
                return mayWrite; // call为null，认为其一定impure，保守处理
            }
        }
        if (isParamWrite) {
            Set<Value> mayWriteIncludeRealParam = new HashSet<>(mayWrite);
            for (Value rParam : call.getRParams()) {
                if (rParam instanceof GlbObjPtr || rParam.getType() instanceof TPointer) {
                    mayWriteIncludeRealParam.add(rParam);
                }
            }
            return mayWriteIncludeRealParam;
        } else
            return mayWrite;
    }

    public Set<Value> getMayRead(CallInstr call) {
        if (function.getName().equals("OHMParallelFor")) {
            if (call != null) {
                for (Value rParam : call.getRParams()) {
                    if (rParam instanceof Function funcParam) {
                        return FunctionInfoCollector.getFuncInfo(funcParam).getMayRead(null);
                    }
                }
            } else {
                return mayRead; // call为null，认为其一定impure，保守处理
            }
        }
        return mayRead;
    }

    /* 流图相关 */
    // todo check if parallel matters
    private List<BasicBlock> domBfsOrder = null;
    private ReachabilityCache reachabilityCache = null;

    public List<BasicBlock> getDomBfsOrder() {
        if (domBfsOrder == null) CFG.executeOnFunc(function);
        return domBfsOrder;
    }

    public void setDomBfsOrder(List<BasicBlock> domBfsOrder) {
        this.domBfsOrder = domBfsOrder;
    }

    /**
     * （懒初始化）获得模块第一次或refresh后执行LoadCSE状态下的函数可达图；
     * 如果函数的pre和suc发生变化，注意必须先执行refreshReachabilityCache，重构可达图
     */
    public ReachabilityCache getReachabilityCache() {
        if (reachabilityCache == null) reachabilityCache = new ReachabilityCache(function);
        return reachabilityCache;
    }

    public void refreshReachabilityCache() {
        this.reachabilityCache = new ReachabilityCache(function);
    }

    /* setter */

    public void setAlloc(boolean alloc) {
        isAlloc = alloc;
    }

    public void setExternRead(boolean externRead) {
        isExternRead = externRead;
    }

    public void setExternWrite(boolean externWrite) {
        isExternWrite = externWrite;
    }

    public void setGlobalWrite(boolean globalWrite) {
        isGlobalWrite = globalWrite;
    }

    public void setMayRead(Set<Value> mayRead) {
        this.mayRead = mayRead;
    }

    public void setMayWrite(Set<Value> mayWrite) {
        this.mayWrite = mayWrite;
    }

    public void setParamWrite(boolean paramWrite) {
        isParamWrite = paramWrite;
    }

    public void setReachabilityCache(ReachabilityCache reachabilityCache) {
        this.reachabilityCache = reachabilityCache;
    }

    public void setRecur(boolean recur) {
        isRecur = recur;
    }

    public void setIO(boolean IO) {
        isIO = IO;
    }

    public void setParallelFunctionPtrSet(Set<Function> parallelFunctionPtrSet) {
        this.parallelFunctionPtrSet = parallelFunctionPtrSet;
    }

    @Override
    public String toString() {

        return "FunctionInfo " + function.getName() + " {\n" +
                "  isExternRead: " + isExternRead + "\n" +
                "  mayRead: " + formatSet(mayRead) + "\n" +
                "  isExternWrite: " + isExternWrite + "\n" +
                "  isGlobalWrite: " + isGlobalWrite + "\n" +
                "  isParamWrite: " + isParamWrite + "\n" +
                "  mayWrite: " + formatSet(mayWrite) + "\n" +
                "  isAlloc: " + isAlloc + "\n" +
                "  isIO: " + isIO + "\n" +
                "  isRecur: " + isRecur + "\n" +
                "}";
    }

    private String formatSet(Set<Value> set) {
        if (set.isEmpty()) return "[]";
        StringBuilder sb = new StringBuilder("[");
        for (Value v : set) {
            sb.append(v.value2string()).append(", ");
        }
        sb.setLength(sb.length() - 2); // remove trailing ", "
        sb.append("]");
        return sb.toString();
    }

}
