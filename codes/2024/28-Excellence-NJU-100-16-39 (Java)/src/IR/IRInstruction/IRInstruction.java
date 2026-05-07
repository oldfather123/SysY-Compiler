package IR.IRInstruction;
import IR.IRType.IRFloatType;
import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.ArrayList;
import java.util.List;


public abstract class IRInstruction {
    /*
     * 该类是IR指令的抽象类，用于表示IR中的指令
     */
    private List<IRValueRef> operands; /*操作数*/
    private IRBaseBlockRef BaseBlock;/*指令所在的基本块*/
    public IRInstruction(List<IRValueRef> operands, IRBaseBlockRef BaseBlock) {
        this.operands = operands;
        this.BaseBlock = BaseBlock;
    }

    public List<IRValueRef> getOperands() {
        return new ArrayList<>(operands);
    }

    public IRBaseBlockRef getBaseBlock() {
        return BaseBlock;
    }

    public void setBaseBlock(IRBaseBlockRef baseBlock) {
        BaseBlock = baseBlock;
    }

    /**
     * 获得操作数数量
     * @param instruction
     * @return
     */
    public static int IRGetOperandsNum(IRInstruction instruction) {
        return instruction.getOperands().size();
    }

    /**
     * 获得操作数
     * @param instruction
     * @param index
     * @return
     */
    public static IRValueRef IRGetOperand(IRInstruction instruction, int index) {
        return instruction.getOperands().get(index);
    }

    public void IRSetOperand(int index, IRValueRef newValue) {
        operands.set(index, newValue);
    }

    /**
     * 获得IR指令类型
     * @param irInstruction
     * @return
     */
    public static String IRGetInstructionOpcode(IRInstruction irInstruction) {
        if (irInstruction instanceof AllocateInstruction) {
            return "IRAllocate";
        }
        if (irInstruction instanceof CalculateInstruction) {
            switch (((CalculateInstruction) irInstruction).getType()) {
                case "add" :
                    return "IRAdd";
                case "fadd" :
                    return "IRFAdd";
                case "sub" :
                    return "IRSub";
                case "fsub":
                    return "IRFSub";
                case "mul" :
                    return "IRMul";
                case "fmul" :
                    return "IRFMul";
                case "sdiv" :
                    return "IRSDiv";
                case "fdiv" :
                    return "IRFDiv";
                case "srem" :
                    return "IRSRem";
                case "xor" :
                    return "IRXor";
                default:
                    return null;
            }
        }
        if (irInstruction instanceof CompareInstruction) {
            boolean floatCompare = false;
            for (IRValueRef operand : irInstruction.getOperands()) {
                if (operand.getType() instanceof IRFloatType) {
                    floatCompare = true;
                    break;
                }
            }
            if (floatCompare) {
                switch (((CompareInstruction) irInstruction).getCompareType()) {
                    case 0:
                        return "IRUeq";
                    case 1:
                        return "IRUne";
                    case 2:
                        return "IRUgt";
                    case 3:
                        return "IRUge";
                    case 4:
                        return "IRUlt";
                    case 5:
                        return "IRUle";
                    default:
                        return null;
                }
            }
            else {
                switch (((CompareInstruction) irInstruction).getCompareType()) {
                    case 0:
                        return "IREq";
                    case 1:
                        return "IRNe";
                    case 2:
                        return "IRSgt";
                    case 3:
                        return "IRSge";
                    case 4:
                        return "IRSlt";
                    case 5:
                        return "IRSle";
                    default:
                        return null;
                }
            }
        }
        if (irInstruction instanceof LoadInstruction) {
            return "IRLoad";
        }
        if (irInstruction instanceof ReturnInstruction) {
            return "IRRet";
        }
        if (irInstruction instanceof StoreInstruction) {
            return "IRStore";
        }
        if (irInstruction instanceof TypeTransferInstruction) {
            return "IRTypeTransfer";
        }
        if (irInstruction instanceof ZextInstruction) {
            return "IRZExt";
        }
        if (irInstruction instanceof CallInstruction) {
            return "IRCall";
        }
        if (irInstruction instanceof BranchInstruction) {
            return "IRBr";
        }
        if (irInstruction instanceof GetElementPointerInstruction) {
            return "IRGetElementPointer";
        }
        if (irInstruction instanceof PhiInstruction) {
            return "IRPhi";
        }
        return null;
    }

    /**
     * 用于mem2Reg
     */
    public boolean usesValue(IRValueRef value) {
        return operands.contains(value);
    }
}
