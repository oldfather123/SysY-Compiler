package IR.optimizer;

import IR.IRInstruction.AllocateInstruction;
import IR.IRInstruction.CallInstruction;
import IR.IRInstruction.GetElementPointerInstruction;
import IR.IRInstruction.IRInstruction;
import IR.IRType.*;
import IR.IRValueRef.*;
import IR.IRModule;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashMap;

public class GlobalArrToLocal {
    //全局数组局部化（仅针对main函数）
    private final HashSet<IRGlobalRegRef> varsToRemove = new HashSet<>();

    public void Optimize(IRModule irModule) {
        int length = 0;
        ArrayList<IRFunctionBlockRef> irFunctionBlocks = irModule.getFunctionBlocks();
        LinkedHashMap<IRGlobalRegRef, IRValueRef> globalVars = irModule.getGlobalVariables();
        // 找到main函数
        IRFunctionBlockRef mainFunctionBlock = null;
        for (IRFunctionBlockRef functionBlock : irFunctionBlocks) {
            if (functionBlock.getFunctionName().equals("main")) {
                mainFunctionBlock = functionBlock;
                break;
            }
        }
        if (mainFunctionBlock != null) {
            for (IRGlobalRegRef globalVar : globalVars.keySet()) {
                if (isArrayType(globalVar)) {
                    if (globalVars.get(globalVar) instanceof IRArrayRef irArrayRef) {
                        if (irArrayRef.isAllZero() && isOnlyUsedInMain(globalVar, irFunctionBlocks)) {
                            IRPointerType pointerType = (IRPointerType) globalVar.getType();
                            IRArrayType irArrayType = (IRArrayType) pointerType.getBaseType();
                            length += Math.pow(irArrayType.getLength(), irArrayType.getLengthList().size());
                            transformGlobalToLocal(globalVar, mainFunctionBlock, irFunctionBlocks);
                        }
                    }
                }
                if (length > 500000) {
                    break;
                }
            }
            // 删除原本的全局变量
            for (IRGlobalRegRef globalVar : varsToRemove) {
                globalVars.remove(globalVar);
            }
        }
    }

    private boolean isArrayType(IRGlobalRegRef globalVar) {
        IRPointerType pointerType = (IRPointerType) globalVar.getType();
        return pointerType.getBaseType() instanceof IRArrayType;
    }

    private boolean isOnlyUsedInMain(IRGlobalRegRef globalVar, ArrayList<IRFunctionBlockRef> irFunctionBlockRefs) {
        for (IRFunctionBlockRef irFunctionBlockRef : irFunctionBlockRefs) {
            if (irFunctionBlockRef.getFunctionName().equals("main")) {
                for (IRBaseBlockRef irBaseBlockRef : irFunctionBlockRef.getBaseBlocks()) {
                    for (IRInstruction irInstruction : irBaseBlockRef.getInstructionList()) {
                        //main函数中仅看call指令
                        if (irInstruction instanceof CallInstruction callInstruction) {
                            for (int i = 0; i < callInstruction.getParams().size(); i++) {
                                if (callInstruction.getParams().get(i) instanceof IRVirtualRegRef param) {
                                    if (param.getIdentity().equals(globalVar.getIdentity())){
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                for (IRBaseBlockRef irBaseBlockRef : irFunctionBlockRef.getBaseBlocks()) {
                    for (IRInstruction irInstruction : irBaseBlockRef.getInstructionList()) {
                        for (int i = 0; i < irInstruction.getOperands().size(); i++) {
                            if (irInstruction.getOperands().get(i) == null) {
                                continue;
                            }
                            if (irInstruction.getOperands().get(i).equals(globalVar)) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
        return true;
    }

    private void transformGlobalToLocal(IRGlobalRegRef globalVar, IRFunctionBlockRef
            mainFunctionBlock, ArrayList<IRFunctionBlockRef> irFunctionBlockRefs) {
        IRPointerType pointerType = (IRPointerType) globalVar.getType();
        // 在main函数开始位置声明数组
        IRValueRef resRegister;
        resRegister = new IRVirtualRegRef("arr", pointerType);
        IRInstruction allocateInst = new AllocateInstruction(Collections.singletonList(resRegister), mainFunctionBlock.getBaseBlocks().get(0));
        mainFunctionBlock.getBaseBlocks().get(0).addInstructionAtStart(allocateInst);
        //替换所有对globalVar的使用
        for (IRFunctionBlockRef irFunctionBlockRef : irFunctionBlockRefs) {
            for (IRBaseBlockRef irBaseBlockRef : irFunctionBlockRef.getBaseBlocks()) {
                for (IRInstruction irInstruction : irBaseBlockRef.getInstructionList()) {
                    for (int i = 0; i < irInstruction.getOperands().size(); i++) {
                        if (irInstruction.getOperands().get(i) == null) {
                            continue;
                        }
                        if (irInstruction.getOperands().get(i).equals(globalVar)) {
                            irInstruction.IRSetOperand(i, resRegister);
                            if (irInstruction instanceof GetElementPointerInstruction) {
                                ((GetElementPointerInstruction) irInstruction).setBase(resRegister);
                            }
                        }
                    }
                }
            }
        }
        // 标记需要移除的全局变量
        varsToRemove.add(globalVar);
    }
}