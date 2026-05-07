package midend.range;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;

/**
 * 值域分析运行版
 * 前置pass：SCEV、CFG
 */
public class RangeAnalysisV3 {
    private static final Map<Function, RangeAnalysisWorklistV3> func2RangeAnalysisWorklist = new HashMap<>();

    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            executeOnFunc(function);
        }
//        dumpAllRanges();
    }

    private static void executeOnFunc(Function function) {
        RangeAnalysisWorklistV3 ra = RangeAnalysisWorklistV3.runOn(function);
        func2RangeAnalysisWorklist.put(function, ra);
    }

    /**
     * 对外接口
     */
    public static Range getRange(Value value, BasicBlock curBlk) {
        if (value instanceof IntConst vic) return Range.constRange(vic.getConstInt().intValue());
        if (value instanceof Function.FParam) return Range.top();
        if (!(value instanceof Instruction instr)) return Range.top();

        RangeAnalysisWorklistV3 ra = func2RangeAnalysisWorklist.getOrDefault(curBlk.getParentFunc(), null);
        if (ra == null) return Range.top();
        Context c = ra.ctxAfter(instr, curBlk);
        if (c == null) {
            // 保险处理
            return Range.top();
        }
        Range rx = c.getOrDefaultTop(value);
        return rx;
    }

    /**
     * 对外接口
     */
    public static RangeAnalysisWorklistV3 getWorklist(Function function) {
        return func2RangeAnalysisWorklist.get(function);
    }

    static Value reg1;
    static Value reg14;
    static BasicBlock bb3;
    static BasicBlock bb13;

    public static void dumpFunctionRanges(Function function) {
        RangeAnalysisWorklistV3 ra = func2RangeAnalysisWorklist.get(function);
        if (ra == null) {
            System.out.println("[RangeAnalysis] Function not analyzed: " + function.getName());
            return;
        }

        System.out.println("=== Ranges for Function: " + function.getName() + " ===");

        // 1) 打印参数
        for (Value arg : function.getFParams()) {
            if (arg.getType() instanceof frontend.ir.objecttype.arithmetic.TInt) {
                Range r = getRange(arg, function.getFirstBlk()); // 参数目前为 TOP
                System.out.println("  [Arg] " + arg.value2string() + " -> " + r);
            }
        }

        // 2) 遍历基本块与指令；采集所有出现的整型 Value（指令结果 + 出现的整型常量）
        for (BasicBlock bb : function.getBasicBlockList()) {

            /*if (bb.value2string().equals("main_blk_13")) {
                bb13 = bb;
            }
            if (bb.value2string().equals("main_blk_3")) {
                bb3 = bb;
            }*/

            System.out.println("  [Block] " + bb.value2string());
            // （可选）把块头的 PHI 结果也打出来
            for (Instruction inst : bb.getInstrList()) {

                if (inst.value2string().equals("%reg_1")) {
                    reg1 = inst;
                }
                if (inst.value2string().equals("%reg_14")) {
                    reg14 = inst;
                }

                if (!(inst.getType() instanceof frontend.ir.objecttype.arithmetic.TInt)) continue;
                Range r = getRange(inst, bb);
                System.out.println("    " + inst.value2string() + " -> " + r);
            }

            // 3) 也把指令操作数里出现过的整型常量/值打印（避免重复用一个 set）
            LinkedHashSet<Value> seen = new LinkedHashSet<>();
            for (Instruction inst : bb.getInstrList()) {
                for (Value op : inst.getOperands()) {
                    if (op == null) continue;
                    if (!(op.getType() instanceof frontend.ir.objecttype.arithmetic.TInt)) continue;
                    if (op instanceof Function.FParam) continue; // 参数已在上面打印
                    if (seen.add(op)) {
                        Range r = getRange(op, bb);
                        System.out.println("    [use] " + op.value2string() + " -> " + r);
                    }
                }
            }
        }

        /*System.out.println("==Special Search==");
        for (BasicBlock bb : function.getBasicBlockList()) {
            System.out.println("  [Block] " + bb.value2string());
            System.out.println("     " + reg14.value2string() + " -> " + getRange(reg14, bb));
        }*/

        System.out.println("=== End Function: " + function.getName() + " ===\n");
    }

    /**
     * 打印所有已分析函数的范围
     */
    public static void dumpAllRanges() {
        if (func2RangeAnalysisWorklist.isEmpty()) {
            System.out.println("[RangeAnalysis] No functions analyzed. Call execute(...) first.");
            return;
        }
        for (Map.Entry<Function, RangeAnalysisWorklistV3> e : func2RangeAnalysisWorklist.entrySet()) {
            dumpFunctionRanges(e.getKey());
        }
    }

}
