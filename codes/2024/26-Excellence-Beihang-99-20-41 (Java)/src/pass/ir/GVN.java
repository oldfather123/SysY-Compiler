package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.instr.BinaInstr;
import ir.value.Value;
import pass.Pass;

import java.util.ArrayList;
import java.util.HashMap;

// 本优化基于二元运算指令的参与value均为中间量的假设
public class GVN implements Pass.IrPass {
    private final HashMap<Integer, Value> GVNMapForward = new HashMap<>();
    private final HashMap<Value, Integer> GVNMapBackward = new HashMap<>();
    private final HashMap<Value, Value> GVNIntermediateMap = new HashMap<>();

    @Override
    public String getName() {
        return "GVN";
    }

    @Override
    public void run(IrModule module) {
        for (var func : module.getFunctions()) {
            GVNIntermediateMap.clear();
            GVNMapForward.clear();
            GVNMapBackward.clear();
            for (var bb : func.getBasicBlocks()) {
                ArrayList<IrInstr> result = new ArrayList<>();
                for (var instr : bb.getInstrs()) {
                    IrInstr newInstr = tackle(instr);
                    if (newInstr != null) {
                        result.add(newInstr);
                    }
                }
                bb.resetInstrs(result);
            }
        }
    }

    private IrInstr tackle(IrInstr instr) {
        for (var entry : GVNIntermediateMap.entrySet()) {
            instr.resetIntermediate(entry.getKey(), entry.getValue());
        }
        if (!(instr instanceof BinaInstr binaInstr)) {
            return instr;
        }
        Value left = getIntermediate(binaInstr.val1);
        Value right = getIntermediate(binaInstr.val2);
        int newHash = calculate(trackHash(left), trackHash(right), binaInstr.op);
        if (GVNMapForward.containsKey(newHash) && newHash != -1) {
            trackHash(left);
            GVNIntermediateMap.put(binaInstr.result, GVNMapForward.get(newHash));
            return null;
        } else {
            GVNMapForward.put(newHash, binaInstr.result);
            GVNMapBackward.put(binaInstr.result, newHash);
            return binaInstr;
        }
    }

    private Value getIntermediate(Value value) {
        return GVNIntermediateMap.getOrDefault(value, value);
    }

    private int trackHash(Value value) {
        if (GVNMapBackward.containsKey(value)) {
            return GVNMapBackward.get(value);
        }
        return -1;
    }

    private int calculate(int left, int right, BinaInstr.OpCode op) {
        if (left == -1 || right == -1) {
            return -1;
        }
        return switch (op) {
            case FADD, ADD -> left + right;
            case FSUB, SUB -> left - right;
            case FMUL, MUL -> left * right;
            case FDIV, SDIV -> left / right;
            case FREM, SREM -> left % right;
            case AND -> left & right;
            case OR -> left | right;
            case SHL -> left << right;
            case LSHR -> left >>> right;
            case ASHR -> left >> right;
            default -> -1;
        };
    }
}
