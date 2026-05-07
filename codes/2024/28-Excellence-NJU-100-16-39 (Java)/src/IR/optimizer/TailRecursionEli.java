package IR.optimizer;

import IR.IRInstruction.*;
import IR.IRType.IRType;
import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRFunctionBlockRef;
import IR.IRValueRef.IRValueRef;
import IR.IRValueRef.IRVirtualRegRef;

import java.util.*;

//https://www.cnblogs.com/JeffreyZhao/archive/2009/04/01/tail-recursion-explanation.html
public class TailRecursionEli implements OptForIR{
    @Override
    public void Optimize(IRFunctionBlockRef irFunctionBlockRef) {
        //判断是否含有尾递归
        if (!TailRecursion(irFunctionBlockRef)) {
            return;
        }
        if (irFunctionBlockRef.getParams().size() > 30){
            return;
        }
        //新建一个phi指令块实现对参数的选择
        IRBaseBlockRef phiBlock = createPhiBlock(irFunctionBlockRef);
        Map<IRValueRef, PhiInstruction> phiInstructions = createPhiInstructions(irFunctionBlockRef, phiBlock);
        for (IRBaseBlockRef block : irFunctionBlockRef.getBaseBlocks()) {
            if (isTailRecursion(irFunctionBlockRef, block)) {
                replaceTailRecursion(irFunctionBlockRef, block, phiBlock, phiInstructions);
            }
        }
    }

    private boolean TailRecursion(IRFunctionBlockRef function) {
        for (IRBaseBlockRef block : function.getBaseBlocks()) {
            if (isTailRecursion(function, block)) {
                return true;
            }
        }
        return false;
    }

    private boolean isTailRecursion(IRFunctionBlockRef function, IRBaseBlockRef block) {
        List<IRInstruction> instructions = block.getInstructionList();
        if (instructions.size() <= 1){
            return false;
        }
        IRInstruction lastInstruction = instructions.get(instructions.size() - 1);
        IRInstruction lastButOneInstruction = instructions.get(instructions.size() - 2);
        if (lastInstruction instanceof ReturnInstruction returnInst
                && lastButOneInstruction instanceof CallInstruction callInst) {
            return callInst.getFunction().getText().equals(function.getText())
                    && returnInst.getOperands().get(0).getText().equals(callInst.getOperands().get(0).getText());
        }
        return false;
    }

    private IRBaseBlockRef createPhiBlock(IRFunctionBlockRef function) {
        IRBaseBlockRef entryBlock = function.getBaseBlocks().get(0);
        IRBaseBlockRef phiBlock = new IRBaseBlockRef("phiBlock");
        function.addBaseBlockRef(phiBlock);

        List<IRInstruction> entryInstructions = entryBlock.getInstructionList();
        List<IRInstruction> phiInstructions = new ArrayList<>(entryInstructions);
        for (IRInstruction instruction : phiInstructions) {
            phiBlock.appendInstr(instruction);
        }
        entryInstructions.clear();
        entryBlock.appendInstr(new BranchInstruction(Collections.singletonList(phiBlock), entryBlock));
        return phiBlock;
    }

    private Map<IRValueRef, PhiInstruction> createPhiInstructions(IRFunctionBlockRef function, IRBaseBlockRef phiBlock) {
        Map<IRValueRef, PhiInstruction> phiInstructions = new HashMap<>();
        List<IRValueRef> params = function.getParams();
        for (IRValueRef param : params) {
            IRType pointerType = param.getType();
            IRValueRef resRegister = new IRVirtualRegRef("phi", pointerType);
            PhiInstruction phiInst = new PhiInstruction(Collections.singletonList(resRegister), phiBlock);
            phiBlock.addInstructionAtStart(phiInst);
            phiInst.setIncomingValue(param, function.getBaseBlocks().get(0));
            phiInstructions.put(param, phiInst);
        }
        return phiInstructions;
    }

    private void replaceTailRecursion(IRFunctionBlockRef function, IRBaseBlockRef block, IRBaseBlockRef phiBlock, Map<IRValueRef, PhiInstruction> phiInstructions) {
        //替换尾递归块的call和ret为br
        List<IRInstruction> instructions = block.getInstructionList();
        CallInstruction callInst = (CallInstruction) instructions.get(instructions.size() - 2);
        ReturnInstruction returnInst = (ReturnInstruction) instructions.get(instructions.size() - 1);
        instructions.remove(callInst);
        instructions.remove(returnInst);
        block.appendInstr(new BranchInstruction(Collections.singletonList(phiBlock), block));
        List<IRValueRef> params = function.getParams();
        for (int i = 0; i < params.size(); i++){
            IRValueRef param = function.getParam(i);
            IRValueRef args = callInst.getParams().get(i);
            phiInstructions.get(param).setIncomingValue(args, block);
            //将形参在函数中的使用点替换为Phi
            for (int j = 0; j < function.getBaseBlocks().size(); j++) {
                IRBaseBlockRef replaceBlock = function.getBaseBlocks().get(j);
                for (int k = 0; k < replaceBlock.getInstructionList().size(); k++) {
                    IRInstruction instruction = replaceBlock.getInstructionList().get(k);
                    List<IRValueRef> operands = instruction.getOperands();
                    IRValueRef phi = phiInstructions.get(param).getOperands().get(0);
                    //call指令传参
                    if (instruction instanceof CallInstruction callInstruction) {
                        for (int m = 0; m < callInstruction.getParams().size(); m++) {
                            if (callInstruction.getParams().get(m).equals(param)) {
                                callInstruction.setParams(m, phi);
                            }
                        }
                    }
                    for (int m = 0; m < operands.size(); m++) {
                        if (operands.get(m) == null) {
                            continue;
                        }
                        if (operands.get(m).getText().equals(param.getText())) {
                            if (instruction instanceof GetElementPointerInstruction) {
                                ((GetElementPointerInstruction) instruction).setIndex(m - 2, phi);
                                instruction.IRSetOperand(m, phi);
                            } else if (instruction instanceof TypeTransferInstruction) {
                                if (m == 1) {
                                    ((TypeTransferInstruction) instruction).setOrigin(phi);
                                } else if (m == 0) {
                                    ((TypeTransferInstruction) instruction).setResReg(phi);
                                }
                                instruction.IRSetOperand(m, phi);
                            } else {
                                instruction.IRSetOperand(m, phi);
                            }
                        }
                    }
                }
            }
        }
    }
}