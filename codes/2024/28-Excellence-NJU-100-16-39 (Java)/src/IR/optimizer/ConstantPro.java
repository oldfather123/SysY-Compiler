package IR.optimizer;

import IR.IRConst;
import IR.IRType.IRInt32Type;
import IR.IRType.IRFloatType;
import IR.IRType.IRType;
import IR.IRValueRef.IRConstIntRef;
import IR.IRValueRef.IRConstFloatRef;
import IR.IRValueRef.IRFunctionBlockRef;
import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;
import IR.IRInstruction.*;

import java.util.Iterator;
import java.util.List;

/**
 * 常量折叠
 * 常量传播: 由mem2Reg完成;
 */
public class ConstantPro implements OptForIR {

    @Override
    public void Optimize(IRFunctionBlockRef irFunctionBlockRef) {
        for (IRBaseBlockRef baseBlock : irFunctionBlockRef.getBaseBlocks()) {
            Iterator<IRInstruction> iterator = baseBlock.getInstructionList().iterator();

            while (iterator.hasNext()) {
                IRInstruction instruction = iterator.next();
                if (instruction instanceof CalculateInstruction calcInst) {
                    List<IRValueRef> operands = calcInst.getOperands();
                    IRValueRef pointer = operands.get(0);
                    IRValueRef lhs = operands.get(1);
                    IRValueRef rhs = operands.get(2);
                    if (isConstantValue(lhs) && isConstantValue(rhs)) {
                        IRValueRef result = foldConstant(calcInst.getType(), lhs, rhs);
                        if (result == null) {
                            continue;
                        }
                        iterator.remove();
                        //替换所有pointer为result;
                        for (int i = 0; i < irFunctionBlockRef.getBaseBlocks().size(); i++) {
                            IRBaseBlockRef replaceBlock = irFunctionBlockRef.getBaseBlocks().get(i);
                            for (int j = 0; j < replaceBlock.getInstructionList().size(); j++) {
                                IRInstruction inst = replaceBlock.getInstructionList().get(j);
                                List<IRValueRef> op = inst.getOperands();
                                //call指令传参
                                if (inst instanceof CallInstruction callInstruction) {
                                    for (int k = 0; k < callInstruction.getParams().size(); k++) {
                                        if (callInstruction.getParams().get(k).equals(pointer)) {
                                            callInstruction.setParams(k, result);
                                        }
                                    }
                                }
                                if (inst instanceof PhiInstruction phiInstruction){
                                    for (IRBaseBlockRef key : phiInstruction.getIncomingValues().keySet()){
                                        if (phiInstruction.getIncomingValues().get(key).equals(pointer)){
                                            phiInstruction.setIncomingValue(result, key);
                                        }
                                    }
                                }
                                for (int k = 0; k < op.size(); k++) {
                                    if (op.get(k) == null) {
                                        continue;
                                    }
                                    if (op.get(k).getText().equals(pointer.getText())) {
                                        if (inst instanceof GetElementPointerInstruction) {
                                            ((GetElementPointerInstruction) inst).setIndex(k - 2, result);
                                            inst.IRSetOperand(k, result);
                                        } else {
                                            inst.IRSetOperand(k, result);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //检查值是否为常量
    private boolean isConstantValue(IRValueRef value) {
        return value.getTypeKind() == IRConst.IRConstantInt32ValueKind ||
                value.getTypeKind() == IRConst.IRConstantFloatValueKind;
    }

    //计算常量表达式的结果
    private IRValueRef foldConstant(String type, IRValueRef lhs, IRValueRef rhs) {
        IRType resultType = lhs.getType();
        if (resultType instanceof IRInt32Type) {
            int lhsValue = Integer.parseInt(lhs.getText());
            int rhsValue = Integer.parseInt(rhs.getText());
            int resultValue = 0;
            switch (type) {
                case "add":
                    resultValue = lhsValue + rhsValue;
                    break;
                case "sub":
                    resultValue = lhsValue - rhsValue;
                    break;
                case "mul":
                    resultValue = lhsValue * rhsValue;
                    break;
                case "sdiv":
                    resultValue = lhsValue / rhsValue;
                    break;
                case "srem":
                    resultValue = lhsValue % rhsValue;
                    break;
                case "xor":
                    resultValue = lhsValue ^ rhsValue;
                    break;
            }
            return new IRConstIntRef(resultValue, IRInt32Type.IRInt32Type());
        } else if (resultType instanceof IRFloatType) {
            float lhsValue = Float.parseFloat(lhs.getText());
            float rhsValue = Float.parseFloat(rhs.getText());
            float resultValue = 0;
            switch (type) {
                case "fadd":
                    resultValue = lhsValue + rhsValue;
                    break;
                case "fsub":
                    resultValue = lhsValue - rhsValue;
                    break;
                case "fmul":
                    resultValue = lhsValue * rhsValue;
                    break;
                case "fdiv":
                    resultValue = lhsValue / rhsValue;
                    break;
            }
            return new IRConstFloatRef(resultValue);
        }
        return null;
    }
}
