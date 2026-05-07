package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.value.BasicBlock;
import ir.value.Function;
import ir.value.Intermediate;
import ir.value.Value;
import pass.Pass;

import java.util.HashMap;

import static utils.Panic.panic;

/**
 * 不是优化 Pass，用来给操作数重命名，仅供 IR 生成时使用
 * MAKE THE DAMN LLVM WORK
 */
public class OperandsRename implements Pass.IrPass {
    private static int counter = 0;
    private final HashMap<String, String> mapper = new HashMap<>();

    @Override
    public void run(IrModule module) {
        for (Function func : module.getFunctions()) {
            evaluateAndReplace(func);
        }
    }

    private void evaluateAndReplace(Function func) {
        for (BasicBlock block : func.getBasicBlocks()) {
            evaluate(block);
        }
        for (BasicBlock block : func.getBasicBlocks()) {
            for (IrInstr instr : block.getInstrs()) {
                for (Value operand : instr.getOperands()) {
                    if (operand == null || !isIntermediate(operand)) {
                        continue;
                    }
                    if (mapper.containsKey(operand.getName())) {
                        instr.replaceOperand(operand, new Intermediate("$" + mapper.get(operand.getName()), operand.getType()));
                    } else {
                        panic("Operand undefined: " + operand.getName());
                    }
                }
                for (Value operand : instr.getOperands()) {
                    if (operand == null || !operand.getName().startsWith("$")) {
                        continue;
                    }
                    instr.replaceOperand(operand, new Intermediate(operand.getName().substring(1), operand.getType()));
                }
            }
        }
    }

    private void evaluate(BasicBlock block) {
        for (IrInstr instr : block.getInstrs()) {
            Value[] operands = instr.getOperands();
            if (operands.length > 0 && operands[0] != null && isIntermediate(operands[0]) && !mapper.containsKey(operands[0].getName())) {
                String newName = "%" + counter++;
                mapper.put(operands[0].getName(), newName);
            }
        }
    }

    private boolean isIntermediate(Value value) {
        return value.getName().matches("^%\\d+$");
    }

    @Override
    public String getName() {
        return "operands-rename";
    }
}
