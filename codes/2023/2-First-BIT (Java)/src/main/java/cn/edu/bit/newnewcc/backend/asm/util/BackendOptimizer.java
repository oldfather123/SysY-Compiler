package cn.edu.bit.newnewcc.backend.asm.util;

import cn.edu.bit.newnewcc.backend.asm.controller.LifeTimeController;
import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.*;

import java.util.*;
import java.util.function.Consumer;

public class BackendOptimizer {
    public static List<AsmInstruction> beforeAllocateScanForward(List<AsmInstruction> instructionList, LifeTimeController lifeTimeController) {
        CopyCoalescer copyCoalescer = new CopyCoalescer(new ArrayList<>(instructionList), lifeTimeController);
        return copyCoalescer.work();
    }

    public static LinkedList<AsmInstruction> afterAllocateScanForward(LinkedList<AsmInstruction> oldInstructionList) {
        LinkedList<AsmInstruction> newInstructionList = new LinkedList<>();
        while (!oldInstructionList.isEmpty()) {
            Consumer<Integer> popx = (Integer x) -> {
                for (int j = 0; j < x; j++) {
                    oldInstructionList.removeFirst();
                }
            };
            Consumer<Integer> backward = (Integer x) -> {
                for (int j = 0; j < x && j < newInstructionList.size(); j++) {
                    oldInstructionList.addFirst(newInstructionList.removeLast());
                }
            };
            //这个部分是将额外生成的栈空间地址重新转换为普通寻址的过程，必须首先进行该优化
            if (oldInstructionList.size() > 1) {
                {
                    var iSv = oldInstructionList.get(0);
                    var iLd = oldInstructionList.get(1);
                    if (iSv instanceof AsmStore && iLd instanceof AsmLoad) {
                        var iSvOp2 = iSv.getOperand(2);
                        var iLdOp2 = iLd.getOperand(2);
                        if (!(iSvOp2 instanceof ExStackVarContent) && iSvOp2.equals(iLdOp2)) {
                            newInstructionList.addLast(oldInstructionList.removeFirst());
                            popx.accept(1);
                            Register source = (Register) iSv.getOperand(1);
                            Register destination = (Register) iLd.getOperand(1);
                            if (!source.equals(destination)) {
                                oldInstructionList.addFirst(new AsmMove(destination, source));
                            }
                            backward.accept(1);
                            continue;
                        }
                    } else if (iSv instanceof AsmJump jump) {
                        if (iLd instanceof AsmLabel label) {
                            if (jump.getCondition() == AsmJump.Condition.UNCONDITIONAL && ((Label)jump.getOperand(1)).getLabelName().equals(label.getLabel().getLabelName())) {
                                popx.accept(1);
                            }
                        }
                    }
                }
                if (oldInstructionList.size() > 2) {
                    var iLi = oldInstructionList.get(0);
                    var iAdd = oldInstructionList.get(1);
                    var iMov = oldInstructionList.get(2);
                    if (iLi instanceof AsmLoad && iLi.getOperand(2) instanceof Immediate offset) {
                        int offsetVal = offset.getValue();
                        if (!ImmediateValues.bitLengthNotInLimit(offsetVal)) {
                            if (iAdd instanceof AsmAdd && iAdd.getOperand(3) instanceof IntRegister baseRegister && baseRegister.equals(IntRegister.S0)) {
                                if (iMov instanceof AsmLoad iLoad && iLoad.getOperand(2) instanceof StackVar stackVar) {
                                    if (stackVar.getRegister() == iLi.getOperand(1) && stackVar.getAddress().getOffset() == 0) {
                                        ExStackVarContent now = ExStackVarContent.transform(new StackVar(offsetVal, stackVar.getSize(), true));
                                        popx.accept(3);
                                        oldInstructionList.addFirst(new AsmLoad((Register) iLoad.getOperand(1), now));
                                        backward.accept(1);
                                        continue;
                                    }
                                } else if (iMov instanceof AsmStore iStore && iStore.getOperand(2) instanceof StackVar stackVar) {
                                    if (stackVar.getRegister() == iLi.getOperand(1) && stackVar.getAddress().getOffset() == 0) {
                                        ExStackVarContent now = ExStackVarContent.transform(new StackVar(offsetVal, stackVar.getSize(), true));
                                        popx.accept(3);
                                        oldInstructionList.addFirst(new AsmStore((Register) iStore.getOperand(1), now));
                                        backward.accept(1);
                                        continue;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            newInstructionList.addLast(oldInstructionList.removeFirst());
        }
        return newInstructionList;
    }

    public static LinkedList<AsmInstruction> afterAllocateScanBackward(LinkedList<AsmInstruction> oldInstructionList) {
        LinkedList<AsmInstruction> newInstructionList = new LinkedList<>();
        Set<String> lastWrite = new HashSet<>();
        Set<String> lastWriteReg = new HashSet<>();
        while (!oldInstructionList.isEmpty()) {
            var inst = oldInstructionList.removeLast();

            //此部分为重复写入内存空间的优化
            {
                var address = inst.getOperand(2);
                if (inst instanceof AsmLabel) {
                    lastWrite.clear();
                }
                if (address instanceof StackVar && !(address instanceof ExStackVarContent)) {
                    String addressStr = address.toString();
                    //仅有s0为基址寄存器的栈变量需要进行该变换
                    if (addressStr.contains("s0")) {
                        if (inst instanceof AsmLoad) {
                            lastWrite.remove(addressStr);
                        } else if (inst instanceof AsmStore) {
                            if (lastWrite.contains(addressStr)) {
                                continue;
                            }
                            lastWrite.add(address.toString());
                        }
                    }
                }
            }

            //此部分为重复写入寄存器的优化
            {
                if (inst instanceof AsmLabel) {
                    lastWriteReg.clear();
                }
                for (var reg : AsmInstructions.getWriteRegSet(inst)) {
                    if (lastWriteReg.contains(reg.toString())) {
                        continue;
                    }
                    lastWriteReg.add(reg.toString());
                }
                for (var reg : AsmInstructions.getReadRegSet(inst)) {
                    lastWriteReg.remove(reg.toString());
                }
            }

            newInstructionList.addFirst(inst);
        }
        return newInstructionList;
    }
}
