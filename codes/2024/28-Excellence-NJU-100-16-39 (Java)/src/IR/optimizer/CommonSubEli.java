package IR.optimizer;

import IR.IRInstruction.*;
import IR.IRType.IRFloatType;
import IR.IRType.IRInt32Type;
import IR.IRType.IRPointerType;
import IR.IRValueRef.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * 公共子表达式消除
 * int a = b + c;
 * int d = b + c + 5;
 * // 优化后
 * int temp = b + c;
 * int a = temp;
 * int d = temp + 5;
 */

public class CommonSubEli implements OptForIR {
    @Override
    public void Optimize(IRFunctionBlockRef irFunctionBlockRef) {
        // 遍历函数块中的每个基本块
        for (IRBaseBlockRef basicBlock : irFunctionBlockRef.getBaseBlocks()) {
            // 记录每个基本块中的指令
            List<IRInstruction> instructions = basicBlock.getInstructionList();
            // map对应关系：公共子表达式的结果 -> 公共子表达式
            Map<IRValueRef,IRValueRef> commonSubExp = new HashMap<>();
            // 用于记录每个calculate指令的操作数，以及对应的指令
            Map<List<IRValueRef>,CalculateInstruction> commonSubCalc = new HashMap<>();
            // 用于记录每个getElementPointer指令的操作数，以及对应的指令
            Map<IRValueRef,GetElementPointerInstruction> commonSubGetElementPointer = new HashMap<>();
            // 用于记录每个Compare指令的操作数，以及对应的指令
            Map<List<IRValueRef>,CompareInstruction> commonSubCompare = new HashMap<>();
            // 用于记录每个Phi指令的操作数，以及对应的指令
            Map<Map<IRBaseBlockRef,IRValueRef>,PhiInstruction> commonSubPhi = new HashMap<>();
            // 用于记录每个TypeTransfer指令的操作数，以及对应的指令
            Map<List<IRValueRef>,TypeTransferInstruction> commonSubTypeTransfer = new HashMap<>();
            // 用于记录每个Zext指令的操作数，以及对应的指令
            Map<List<IRValueRef>,ZextInstruction> commonSubZext = new HashMap<>();
            // 遍历每个指令
            for (int i = 0;i < instructions.size();i++) {
                IRInstruction instruction = basicBlock.getInstructionList().get(i);

                //首先查看该指令的所有操作数是否在commonSubExp中，如果在，则将其替换为commonSubExp中的值
                for (int j = 0; j < instruction.getOperands().size(); j++) {
                    if(commonSubExp.containsKey(instruction.getOperands().get(j))){
                        instruction.IRSetOperand(j,commonSubExp.get(instruction.getOperands().get(j)));
                    }
                }
                //如果是getElementPointer指令，那么还需要查看其base是否在commonSubExp中，如果在，则将其替换为commonSubExp中的值
                if(instruction instanceof GetElementPointerInstruction){
                    if(commonSubExp.containsKey(((GetElementPointerInstruction) instruction).getBase())){
                        ((GetElementPointerInstruction)instruction).setBase(commonSubExp.get(((GetElementPointerInstruction) instruction).getBase()));
                    }
                    for(int l = 0;l < ((GetElementPointerInstruction) instruction).getIndex().size();l++){
                        if(commonSubExp.containsKey(((GetElementPointerInstruction) instruction).getIndex().get(l))){
                            ((GetElementPointerInstruction)instruction).setIndex(l,commonSubExp.get(((GetElementPointerInstruction) instruction).getIndex().get(l)));
                        }
                    }
                }
                //如果是call指令，那么还需要查看其参数是否在commonSubExp中，如果在，则将其替换为commonSubExp中的值
                if(instruction instanceof CallInstruction){
                    for(int k = 0;k < ((CallInstruction)instruction).getParams().size();k++){
                        if(commonSubExp.containsKey(((CallInstruction)instruction).getParams().get(k))){
                            ((CallInstruction)instruction).setParams(k,commonSubExp.get(((CallInstruction)instruction).getParams().get(k)));
                        }
                    }
                }

                // 如果是计算指令、获取元素指针指令、比较指令、phi指令、类型转换指令、零扩展指令
                if (instruction instanceof CalculateInstruction) {
                    // 如果再次遇到相同的除了第一个operands之外，其他全部相同的指令
                    // 则删去这一条指令，并将这一条指令的等式左值和其对应的值记录在commonSubExp中
                    // 之后再遇到相同的指令，就将其左值替换为commonSubExp中的值
                    List<IRValueRef> operands = instruction.getOperands();
                    IRValueRef resRegister = operands.get(0);
                    operands = operands.subList(1,operands.size());
                    if(commonSubCalc.containsKey(operands) && commonSubCalc.get(operands).getType().equals(((CalculateInstruction) instruction).getType())){
                        //如果计算的类型以及操作数相同
                        IRValueRef temp = commonSubCalc.get(operands).getOperands().get(0);
                        //将这一条指令删除
                        instruction.getBaseBlock().getInstructionList().remove(instruction);
                        i --;
                        //将这一条指令的左值记录在commonSubExp中
                        commonSubExp.put(resRegister,temp);
                    }else{
                        commonSubCalc.put(operands,(CalculateInstruction) instruction);
                    }
                }
                else if (instruction instanceof GetElementPointerInstruction) {
                    List<IRValueRef> operands = instruction.getOperands();
                    IRValueRef resRegister = operands.get(0);
                    //获取GetElementPointer指令的操作数
                    //如果两个操作数都是int类型的常数
                    if(operands.size() == 4) {
                        if (!(operands.get(2) instanceof IRConstIntRef) || !(operands.get(3) instanceof IRConstIntRef)) {
                            //如果不是int类型的常数，那么直接比较即可
                            if (commonSubGetElementPointer.containsKey(operands.get(1))
                                    && commonSubGetElementPointer.get(operands.get(1)).getOperands().get(2).equals(operands.get(2))
                                    && commonSubGetElementPointer.get(operands.get(1)).getOperands().get(3).equals(operands.get(3))) {
                                //如果计算的类型以及操作数相同
                                IRValueRef temp = commonSubGetElementPointer.get(operands.get(1)).getOperands().get(0);
                                //将这一条指令删除
                                instruction.getBaseBlock().getInstructionList().remove(instruction);
                                i--;
                                //将这一条指令的左值记录在commonSubExp中
                                commonSubExp.put(resRegister, temp);
                            } else {
                                commonSubGetElementPointer.put(operands.get(1), (GetElementPointerInstruction) instruction);
                            }
                        } else {
                            //如果两个操作数都是int类型的常数
                            int temp1 = ((IRConstIntRef) (operands.get(2))).getValue();
                            int temp2 = ((IRConstIntRef) (operands.get(3))).getValue();
                            if (commonSubGetElementPointer.containsKey(operands.get(1))
                                    && (commonSubGetElementPointer.get(operands.get(1)).getOperands().get(2)) instanceof IRConstIntRef
                                    && (commonSubGetElementPointer.get(operands.get(1)).getOperands().get(3)) instanceof IRConstIntRef
                                    && temp1 == ((IRConstIntRef) (commonSubGetElementPointer.get(operands.get(1)).getOperands().get(2))).getValue()
                                    && temp2 == ((IRConstIntRef) (commonSubGetElementPointer.get(operands.get(1)).getOperands().get(3))).getValue()
                                    ) {
//                                if(((GetElementPointerInstruction) instruction).getBase().getType() instanceof IRPointerType
//                                        && ((IRPointerType)((GetElementPointerInstruction) instruction).getBase().getType()).getBaseType() instanceof IRArrayType
//                                        &&((IRArrayType)((IRPointerType)((GetElementPointerInstruction) instruction).getBase().getType()).getBaseType()).getBaseType() instanceof IRFloatType) {
//                                    //如果是float类型的数组，那么不能继续优化
//                                    continue;
//                                }
                                //如果计算的类型以及操作数相同
                                IRValueRef temp = commonSubGetElementPointer.get(operands.get(1)).getOperands().get(0);
                                //将这一条指令删除
                                instruction.getBaseBlock().getInstructionList().remove(instruction);
                                i--;
                                //将这一条指令的左值记录在commonSubExp中
                                commonSubExp.put(resRegister, temp);
                            } else {
                                commonSubGetElementPointer.put(operands.get(1), (GetElementPointerInstruction) instruction);
                            }
                        }
                    } else{
                        if (!(operands.get(2) instanceof IRConstIntRef)){
                            if(commonSubGetElementPointer.containsKey(operands.get(1))
                                    && commonSubGetElementPointer.get(operands.get(1)).getOperands().get(2).equals(operands.get(2))) {
                                //如果计算的类型以及操作数相同
                                IRValueRef temp = commonSubGetElementPointer.get(operands.get(1)).getOperands().get(0);
                                //将这一条指令删除
                                instruction.getBaseBlock().getInstructionList().remove(instruction);
                                i--;
                                //将这一条指令的左值记录在commonSubExp中
                                commonSubExp.put(resRegister, temp);
                            }else{
                                commonSubGetElementPointer.put(operands.get(1), (GetElementPointerInstruction) instruction);
                            }
                        }else{
                            int temp = ((IRConstIntRef) (operands.get(2))).getValue();
                            if(commonSubGetElementPointer.containsKey(operands.get(1))
                                    && temp == ((IRConstIntRef) (commonSubGetElementPointer.get(operands.get(1)).getOperands().get(2))).getValue()) {
                                //如果计算的类型以及操作数相同
                                IRValueRef temp1 = commonSubGetElementPointer.get(operands.get(1)).getOperands().get(0);
                                //将这一条指令删除
                                instruction.getBaseBlock().getInstructionList().remove(instruction);
                                i--;
                                //将这一条指令的左值记录在commonSubExp中
                                commonSubExp.put(resRegister, temp1);
                            }else{
                                commonSubGetElementPointer.put(operands.get(1), (GetElementPointerInstruction) instruction);
                            }
                        }
                    }
                }
                else if (instruction instanceof CompareInstruction) {
                    List<IRValueRef> operands = instruction.getOperands();
                    IRValueRef resRegister = operands.get(0);
                    operands = operands.subList(1,operands.size());
                    if(commonSubCompare.containsKey(operands) && commonSubCompare.get(operands).getCompareType() == ((CompareInstruction) instruction).getCompareType()){
                        //如果计算的类型以及操作数相同
                        IRValueRef temp = commonSubCompare.get(operands).getOperands().get(0);
                        //将这一条指令删除
                        instruction.getBaseBlock().getInstructionList().remove(instruction);
                        i --;
                        //将这一条指令的左值记录在commonSubExp中
                        commonSubExp.put(resRegister,temp);
                    }else{
                        commonSubCompare.put(operands,(CompareInstruction) instruction);
                    }
                }
                else if (instruction instanceof PhiInstruction) {
                    Map<IRBaseBlockRef,IRValueRef> operands = ((PhiInstruction) instruction).getIncomingValues();
                    IRValueRef resRegister = instruction.getOperands().get(0);
                    if(commonSubPhi.containsKey(operands)){
                        //如果计算的类型以及操作数相同
                        IRValueRef temp = commonSubPhi.get(operands).getOperands().get(0);
                        //将这一条指令删除
                        instruction.getBaseBlock().getInstructionList().remove(instruction);
                        i --;
                        //将这一条指令的左值记录在commonSubExp中
                        commonSubExp.put(resRegister,temp);
                    }else{
                        commonSubPhi.put(operands,(PhiInstruction) instruction);
                    }
                }
                else if (instruction instanceof ZextInstruction) {
                    List<IRValueRef> operands = instruction.getOperands();
                    IRValueRef resRegister = operands.get(0);
                    operands = operands.subList(1, operands.size());
                    if (commonSubZext.containsKey(operands)) {
                        //如果计算的类型以及操作数相同
                        IRValueRef temp = commonSubZext.get(operands).getOperands().get(0);
                        //将这一条指令删除
                        instruction.getBaseBlock().getInstructionList().remove(instruction);
                        i--;
                        //将这一条指令的左值记录在commonSubExp中
                        commonSubExp.put(resRegister, temp);
                    } else {
                        commonSubZext.put(operands, (ZextInstruction) instruction);
                    }
                }
                else if (instruction instanceof TypeTransferInstruction) {
                    List<IRValueRef> operands = instruction.getOperands();
                    IRValueRef resRegister = operands.get(0);
                    operands = operands.subList(1, operands.size());
                    if (commonSubTypeTransfer.containsKey(operands) && commonSubTypeTransfer.get(operands).getTransferType() == ((TypeTransferInstruction) instruction).getTransferType()){
                        //如果计算的类型以及操作数相同
                        IRValueRef temp = commonSubTypeTransfer.get(operands).getOperands().get(0);
                        //将这一条指令删除
                        instruction.getBaseBlock().getInstructionList().remove(instruction);
                        i--;
                        //将这一条指令的左值记录在commonSubExp中
                        commonSubExp.put(resRegister, temp);
                    } else {
                        commonSubTypeTransfer.put(operands, (TypeTransferInstruction) instruction);
                    }
                }
            }
        }
    }
}
