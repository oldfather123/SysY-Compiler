package IR.optimizer;

import IR.DomAnalysis;
import IR.IRInstruction.*;

import IR.IRType.IRFloatType;
import IR.IRType.IRInt32Type;
import IR.IRType.IRType;
import IR.IRValueRef.*;
import backend.MemoryRegisterAlloc;

import java.util.*;

//实现思路
//https://buaa-se-compiling.github.io/miniSysY-tutorial/challenge/mem2reg/help.html
//https://blog.csdn.net/qq_48201696/article/details/129979831
public class Mem2Reg implements OptForIR {
    //需要进行Mem2Reg优化的所有分配指令。
    Map<String, IRValueRef> allocaVars = new HashMap<>();
    //分配指令在哪些基本块中被定义
    Map<IRValueRef, Set<IRBaseBlockRef>> varDefs = new HashMap<>();
    //前驱
    Map<IRBaseBlockRef, List<IRBaseBlockRef>> predecessors;
    MemoryRegisterAlloc memoryRegisterAlloc = new MemoryRegisterAlloc();

    @Override
    public void Optimize(IRFunctionBlockRef irFunctionBlockRef) {
        //对于基本块过大的函数不做优化
        if (irFunctionBlockRef.getBaseBlocks().size() > 5000){
            return;
        }
        //支配边界
        DomAnalysis domAnalysis = new DomAnalysis(irFunctionBlockRef);
        Map<IRBaseBlockRef, Set<IRBaseBlockRef>> df = domAnalysis.getDominanceFrontiers();
        predecessors = domAnalysis.computePredecessors(irFunctionBlockRef);
        //收集所有需要优化的分配指令
        for (IRBaseBlockRef block : irFunctionBlockRef.getBaseBlocks()) {
            List<IRInstruction> instructions = block.getInstructionList();
            Iterator<IRInstruction> iterator = instructions.iterator();
            while (iterator.hasNext()) {
                IRInstruction inst = iterator.next();
                if (inst instanceof AllocateInstruction) {
                    if ((((AllocateInstruction) inst).getType().getText().equals("i32*"))
                            || (((AllocateInstruction) inst).getType().getText().equals("float*"))) {
                        IRValueRef operand = inst.getOperands().get(0);
                        varDefs.put(operand, new HashSet<>());
                        allocaVars.put(((AllocateInstruction) inst).getName(), operand);
                    }
                } else if (inst instanceof BranchInstruction ||
                        inst instanceof ReturnInstruction) {
                    while (iterator.hasNext()) {
                        iterator.next();
                        iterator.remove();
                    }
                    break;
                }
            }
        }
        // 记录每个变量的定义点
        for (IRBaseBlockRef block : irFunctionBlockRef.getBaseBlocks()) {
            for (IRInstruction inst : block.getInstructionList()) {
                if (inst instanceof StoreInstruction) {
                    IRValueRef pointer = inst.getOperands().get(2);
                    if (allocaVars.containsKey(pointer.getText())) {
                        varDefs.get(allocaVars.get(pointer.getText())).add(block);
                    }
                }
            }
        }
        //插入phi节点
        Map<PhiInstruction, IRValueRef> newPhis = insertPhiNodes(df);
        //更新所有使用和定义
        reNameUses(irFunctionBlockRef, newPhis);
        //移除无用的phi
        removeUnlessPhi(irFunctionBlockRef);
    }

    private Map<PhiInstruction, IRValueRef> insertPhiNodes(Map<IRBaseBlockRef, Set<IRBaseBlockRef>> dominanceFrontiers) {
        //存储新插入的 phi 指令及其关联的变量
        Map<PhiInstruction, IRValueRef> newPhis = new HashMap<>();
        //已访问的基本块
        Set<IRBaseBlockRef> visited = new HashSet<>();

        for (IRValueRef var : varDefs.keySet()) {
            // 初始化工作队列，包含变量的定义点基本块
            Queue<IRBaseBlockRef> workList = new LinkedList<>(varDefs.get(var));
            visited.clear();
            while (!workList.isEmpty()) {
                IRBaseBlockRef block = workList.poll();
                if (dominanceFrontiers.get(block) == null) {
                    continue;
                }
                for (IRBaseBlockRef df : dominanceFrontiers.get(block)) {
                    // 如果支配边界的基本块未被处理
                    if (!visited.contains(df)) {
                        visited.add(df);
                        IRType pointerType;
                        if (var.getType().getText().equals("i32*")) {
                            pointerType = new IRInt32Type();
                        } else {
                            pointerType = new IRFloatType();
                        }
                        IRValueRef resRegister = new IRVirtualRegRef("phi", pointerType);
                        PhiInstruction phi = new PhiInstruction(Collections.singletonList(resRegister), df);
                        df.addInstructionAtStart(phi);
                        newPhis.put(phi, var);
                        if (!varDefs.get(var).contains(df)) {
                            workList.add(df);
                        }
                    }
                }
            }
        }
        return newPhis;
    }

    private void reNameUses(IRFunctionBlockRef irFunctionBlockRef, Map<PhiInstruction, IRValueRef> newPhis) {
        //存放已经访问过的基本块
        Set<IRBaseBlockRef> visited = new HashSet<>();
        //存储待处理的基本块及其对应的进入值映射
        Queue<Map.Entry<IRBaseBlockRef, Map<IRValueRef, IRValueRef>>> workList = new LinkedList<>();
        //初始化
        Map<IRValueRef, IRValueRef> initialIncomingVals = new HashMap<>();
        workList.add(new AbstractMap.SimpleEntry<>(irFunctionBlockRef.getBaseBlocks().get(0), initialIncomingVals));

        while (!workList.isEmpty()) {
            Map.Entry<IRBaseBlockRef, Map<IRValueRef, IRValueRef>> entry = workList.poll();
            IRBaseBlockRef block = entry.getKey();
            Map<IRValueRef, IRValueRef> incomingVals = entry.getValue();
            //处理过
            if (visited.contains(block)) continue;
            visited.add(block);

            Iterator<IRInstruction> iterator = block.getInstructionList().iterator();
            while (iterator.hasNext()) {
                IRInstruction inst = iterator.next();

                if (inst instanceof AllocateInstruction) {
                    if (allocaVars.containsKey(((AllocateInstruction) inst).getName())) {
                        iterator.remove();
                    }
                } else if (inst instanceof LoadInstruction) {
                    IRValueRef pointer = inst.getOperands().get(1);
                    if (allocaVars.containsKey(pointer.getText())) {
                        //将用到 load 结果的地方都替换成 IncomingVals[L]
                        IRValueRef res = inst.getOperands().get(0);
                        for (int i = 0; i < irFunctionBlockRef.getBaseBlocks().size(); i++) {
                            IRBaseBlockRef replaceBlock = irFunctionBlockRef.getBaseBlocks().get(i);
                            for (int j = 0; j < replaceBlock.getInstructionList().size(); j++) {
                                IRInstruction instruction = replaceBlock.getInstructionList().get(j);
                                List<IRValueRef> operands = instruction.getOperands();
                                //call指令传参
                                if (instruction instanceof CallInstruction callInstruction) {
                                    for (int k = 0; k < callInstruction.getParams().size(); k++) {
                                        if (callInstruction.getParams().get(k).equals(res)) {
                                            callInstruction.setParams(k, incomingVals.get(pointer));
                                        }
                                    }
                                }
                                for (int k = 0; k < operands.size(); k++) {
                                    if (operands.get(k) == null) {
                                        continue;
                                    }
                                    if (operands.get(k).getText().equals(res.getText())) {
                                        if (instruction instanceof GetElementPointerInstruction) {
                                            ((GetElementPointerInstruction) instruction).setIndex(k - 2, incomingVals.get(pointer));
                                            instruction.IRSetOperand(k, incomingVals.get(pointer));
                                        } else if (instruction instanceof TypeTransferInstruction) {
                                            if (k == 1) {
                                                ((TypeTransferInstruction) instruction).setOrigin(incomingVals.get(pointer));
                                            } else if (k == 0) {
                                                ((TypeTransferInstruction) instruction).setResReg(incomingVals.get(pointer));
                                            }
                                            instruction.IRSetOperand(k, incomingVals.get(pointer));
                                        } else {
                                            instruction.IRSetOperand(k, incomingVals.get(pointer));
                                        }
                                    }
                                }
                            }
                        }
                        iterator.remove();
                    }
                } else if (inst instanceof StoreInstruction) {
                    IRValueRef pointer = inst.getOperands().get(2);
                    if (allocaVars.containsKey(pointer.getText())) {
                        incomingVals.put(pointer, inst.getOperands().get(1));
                        iterator.remove();
                    }
                } else if (inst instanceof PhiInstruction && newPhis.containsKey(inst)) {
                    IRValueRef alloca = newPhis.get(inst);
                    incomingVals.put(alloca, inst.getOperands().get(0));
                }
            }
            //更新工作集维护phi指令
            for (IRBaseBlockRef succ : memoryRegisterAlloc.getImmediateSuccessors(block)) {
                workList.add(new AbstractMap.SimpleEntry<>(succ, new HashMap<>(incomingVals)));
                for (IRInstruction phiInst : succ.getInstructionList()) {
                    if (phiInst instanceof PhiInstruction && newPhis.containsKey(phiInst)) {
                        IRValueRef alloca = newPhis.get(phiInst);
                        IRValueRef irValueRef = incomingVals.get(alloca);
                        //这行伪代码里没有，但不加有空指针
                        if (irValueRef == null) {
                            if (phiInst.getOperands().get(0).getType() instanceof IRInt32Type) {
                                irValueRef = new IRConstIntRef(0, IRInt32Type.IRInt32Type());
                            } else {
                                irValueRef = new IRConstIntRef(0, IRFloatType.IRFloatType());
                            }
                        }
                        ((PhiInstruction) phiInst).setIncomingValue(irValueRef, block);
                    }
                }
            }
        }
    }

    private void removeUnlessPhi(IRFunctionBlockRef irFunctionBlockRef) {
        for (IRBaseBlockRef block : irFunctionBlockRef.getBaseBlocks()) {
            List<IRInstruction> instructions = block.getInstructionList();
            Iterator<IRInstruction> iterator = instructions.iterator();
            // 遍历基本块中的所有指令
            while (iterator.hasNext()) {
                IRInstruction inst = iterator.next();
                if (inst instanceof PhiInstruction && !isUsed(inst, irFunctionBlockRef)) {
                    iterator.remove();
                }
            }
        }
    }

    private boolean isUsed(IRInstruction inst, IRFunctionBlockRef irFunctionBlockRef) {
        for (IRBaseBlockRef block : irFunctionBlockRef.getBaseBlocks()) {
            for (IRInstruction instruction : block.getInstructionList()) {
                IRValueRef phiName = inst.getOperands().get(0);
                if (inst.equals(instruction)) {
                    continue;
                }
                if (instruction instanceof CallInstruction callInstruction) {
                    for (int i = 0; i < callInstruction.getParams().size(); i++) {
                        if (callInstruction.getParams().get(i).equals(phiName)) {
                            return true;
                        }
                    }
                } else if (instruction instanceof GetElementPointerInstruction getElementPointerInstruction) {
                    for (int i = 0; i < getElementPointerInstruction.getIndex().size(); i++) {
                        if (getElementPointerInstruction.getIndex().get(i).equals(phiName)) {
                            return true;
                        }
                    }
                } else if (instruction instanceof PhiInstruction phiInstruction) {
                    for (IRBaseBlockRef key : phiInstruction.getIncomingValues().keySet()) {
                        if (phiInstruction.getIncomingValues().get(key).equals(phiName)) {
                            return true;
                        }
                    }
                }
                for (IRValueRef operand : instruction.getOperands()) {
                    if (phiName.equals(operand)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
}