package midend.DCE;

import backend.asm.instr.tags.CallIns;
import frontend.ir.Value;
import frontend.ir.Visitor;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.BinaryOperation;
import frontend.ir.instr.convop.Bitcast;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.GlbObjPtr;
import frontend.syntax.ast.nodes.expression.Call;

import java.util.*;

/**
 * 删除冗余的Array
 * 冗余的Array有以下两类：
 * 1. 从未被使用
 * 2. 只被存入内容，但从未被读取
 */
public class RemoveDeadArray {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            if (function instanceof Function.LibFunc) continue;
            executeOnFunc(function);
        }
        executeOnGlobals();
    }

    private static void executeOnFunc(Function function) {
        Set<AllocaInstr> deadAllocs = new HashSet<>();

        for (BasicBlock block : function.getBasicBlockList()) {
            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof AllocaInstr alloca) {
                    if (isDeadArray(alloca)) {
                        deadAllocs.add(alloca);
                    }
                }
            }
        }

        for (AllocaInstr alloc : deadAllocs) {
            removeDeadValueWithUsersRecursively(alloc);
        }
    }

    private static void executeOnGlobals() {
        Set<GlbObjPtr> deadGlobals = new HashSet<>();
        for (GlbObjPtr g : Visitor.getGlobalVariables()) {
            if (g.getType() instanceof TPointer && isDeadArray(g)) {
                deadGlobals.add(g);
            }
        }

        for (GlbObjPtr g : deadGlobals) {
            for (Value user : new ArrayList<>(g.getUserList())) {
                removeDeadValueWithUsersRecursively(user);
            }
        }
    }

    private static boolean isDeadArray(Value array) {
        Set<Value> worklist = new HashSet<>();
        Queue<Value> queue = new LinkedList<>();
        worklist.add(array);
        queue.add(array);

        while (!queue.isEmpty()) {
            Value val = queue.poll();

            for (Value user : val.getUserList()) {
                // Case 1: 是 Load，那数组被读，不能删
                if (user instanceof LoadInstr) {
                    return false;
                }

                // Case 2: 是传参、函数调用、return 等复杂 escape
                if (!isSimpleEscape(user)) {
                    return false;
                }

                // Case 3: 是 GEP，加入继续追踪
                if (user instanceof GEPInstr gep && !worklist.contains(gep)) {
                    worklist.add(gep);
                    queue.add(gep);
                }

                // Case 4: 是 BitCast，加入继续追踪
                if (user instanceof Bitcast bitcast && !worklist.contains(bitcast)) {
                    worklist.add(bitcast);
                    queue.add(bitcast);
                }
            }
        }

        // 所有路径都没看到读/escape，只是 Store，可删
        return true;
    }

    private static boolean isSimpleEscape(Value val) {
        if (val instanceof StoreInstr) return true;
        if (val instanceof GEPInstr) return true;
        if (val instanceof Bitcast) return true;
        if (val instanceof CallInstr call &&
                call.getCallee() instanceof Function.LibFunc libFunc &&
                libFunc.getName().equals("memset"))
            return true;
        return false;
    }


    /**
     * 递归删除 dead value 的所有 user
     * dead value 的 user 也必然是 dead 的
     */
    private static void removeDeadValueWithUsersRecursively(Value root) {
        Set<Value> visited = new HashSet<>();
        Deque<Value> stack = new ArrayDeque<>();
        stack.push(root);
        visited.add(root);

        while (!stack.isEmpty()) {
            Value val = stack.pop();

            for (Value user : new ArrayList<>(val.getUserList())) {
                if (user instanceof Instruction userInstr && visited.add(userInstr)) {
                    stack.push(userInstr);
                }
            }

            if (val instanceof Instruction instr) {
                instr.forceRemoveFromList();
            } else {
                throw new RuntimeException("val " + val.value2string() + " is not instr");
            }
        }
    }


}
