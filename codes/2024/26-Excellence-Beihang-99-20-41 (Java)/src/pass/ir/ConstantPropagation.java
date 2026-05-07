package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.value.*;
import pass.Pass;
import utils.CalculatorInstr;

import java.util.ArrayList;
import java.util.HashMap;

import static ir.type.Ty.*;

public class ConstantPropagation implements Pass.IrPass{
    private HashMap<String, Literal> valueToConstant = new HashMap<>();

    @Override
    public void run(IrModule module) {
        for (Constant constant : module.globalConstants) {
            Literal literal = convertConstantToLiteral(constant);
            if (literal != null) {
                valueToConstant.put(constant.getName(), literal);
            }
        }
        passStepOne(module);
    }

    private void passStepOne(IrModule module) {
        for (Function function : module.functions) {
            for (BasicBlock basicBlock : function.getBasicBlocks()) {
                ArrayList<IrInstr> instrsToMaintain = new ArrayList<>();
                //valueToConstant.clear();
                for (IrInstr instr : basicBlock.getInstrs()) {
                    if (instr.toString().contains("float")) {
                        instrsToMaintain.add(instr);
                        continue;
                    }
                    int flag = 0;
                    ArrayList<Literal> operands = new ArrayList<>();
//                    Value value : instr.getOperands()，但是跳过第一个
                    ArrayList<Value> values = instr.getDependentValues();
                    if (instr.getDependentValues() == null) {
                        continue;
                    }
                    for (int i = 0; i < instr.getDependentValues().size(); i++) {
                        Value value = values.get(i);
                        if (value == null) {
                            continue;
                        }
                        if (!(value instanceof Literal || valueToConstant.containsKey(value.getName()) || value instanceof Constant)) {
                            flag = 1;
                        }
                        else {
                            if (valueToConstant.containsKey(value.getName())) {
                                operands.add(valueToConstant.get(value.getName()));
                                instr.replaceOperand(value, valueToConstant.get(value.getName()));
                            } else if (value instanceof Literal) {
                                operands.add((Literal) value);
                            } else if (value instanceof Constant) {
                                Literal literal = convertConstantToLiteral((Constant) value);
                                if (literal != null) {
                                    operands.add(literal);
                                } else {
                                    flag = 1;
                                }
                            }
                        }
                    }
                    if (flag == 0 && !operands.isEmpty() && instr.getInitialValue() != null) {
                        Literal result;
                        if (operands.size() > 1) {
                            result = CalculatorInstr.compute(instr.getOpCode(), operands,instr,1);
                        }
                        else {
                            result = CalculatorInstr.compute(instr.getOpCode(), operands, instr);
                        }
                        valueToConstant.put(instr.getInitialValue().getName(), result);
                    }
                    else {
                        instrsToMaintain.add(instr);
                    }
                }
            }
        }
    }

    private Literal convertConstantToLiteral(Constant constant) {
        if (constant instanceof Variable) {
            return null;
        }
        if (!valueToConstant.containsKey(constant)) {
            if (constant.getType().equals(I1)) {
                return new Literal(((Boolean) constant.getValue()) ? 1 : 0, I1);
            } else if (constant.getType().equals(I32)) {
                return new Literal((Integer) constant.getValue());
            } else if (constant.getType().equals(F32)) {
                return new Literal((Float) constant.getValue());
            }
            else {
                return null;
            }
        }
        return valueToConstant.get(constant.getName());
    }

    @Override
    public String getName() {
        return "const-propagation";
    }
}
