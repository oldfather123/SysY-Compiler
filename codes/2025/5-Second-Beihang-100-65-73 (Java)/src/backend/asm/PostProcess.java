package backend.asm;

import backend.asm.immediate.ASMImmediate;
import backend.asm.insfact.InstrFactory;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.arm.memory.ARMStore;
import backend.asm.instr.arm.others.ARMMove;
import backend.asm.instr.arm.ternary.ARMMadd;
import backend.asm.instr.rv.jump.RVJ;
import backend.asm.instr.tags.*;
import backend.asm.register.Reg;
import backend.asm.register.store.ArmRegStore;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMModel;
import util.CustomList;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class PostProcess {
    public static PostProcess POST_PROCESS = null;

    private RegStore regStore;
    private InstrFactory instrFactory;

    private PostProcess() {
    }

    public static PostProcess getInstance() {
        if (POST_PROCESS == null) {
            POST_PROCESS = new PostProcess();
        }
        return POST_PROCESS;
    }

    public void execute(ASMModel asmModel, RegStore regStore, InstrFactory instrFactory) {
        this.regStore = regStore;
        this.instrFactory = instrFactory;

        fixImm(asmModel);
        if (regStore instanceof ArmRegStore) {
            fixUsageOfSP(asmModel);
        }
    }

    private void fixImm(ASMModel asmModel) {    // 修正立即数的使用
        for (ASMFunction asmFunction : asmModel.getFunctionList()) {
            for (CustomList.Node blockNode : asmFunction.getBasicBlockList()) {
                ASMBasicBlock block = (ASMBasicBlock) blockNode;
                HashSet<ASMInstruction> instructionsToRemove = new HashSet<>();
                boolean removeFlag = false;
                for (CustomList.Node instrNode : block.getInstructionList()) {
                    ASMInstruction instr = (ASMInstruction) instrNode;
                    if (removeFlag) {
                        instructionsToRemove.add(instr);
                        continue;
                    }
                    if (instr instanceof JumpIns) {
                        removeFlag = true;
                        continue;
                    }
                    if (instr instanceof LiIns || instr instanceof MoveIns) {
                        continue;
                    }
                    if (instr instanceof LoadIns load) {
                        int offsetValue = load.getOffset().getImm();
                        if (offsetValue > 2047 || offsetValue < -2048) {
                            VirIReg virIReg1 = new VirIReg();
                            virIReg1.setPhyReg(regStore.getReservedReg());
                            ASMInstruction li = instrFactory.createLi(load.getOffset(), null, virIReg1);
                            li.insertBefore(instr);

                            VirIReg virIReg2 = new VirIReg();
                            virIReg2.setPhyReg(regStore.getReservedReg());
                            virIReg2.setDouble(true);
                            ASMInstruction add = instrFactory.createAdd((Reg) load.getBase(), li.getRegister(),
                                    null, virIReg2);
                            add.insertBefore(instr);
                            ((ASMInstruction) load).resetOperands(
                                    List.of(add.getRegister(), new ASMImmediate(0))
                            );  // 重置偏移量为0
                        }
                        continue;
                    }
                    if (instr instanceof StoreIns store) {
                        int offsetValue = store.getOffset().getImm();
                        if (offsetValue > 2047 || offsetValue < -2048) {
                            VirIReg virIReg1 = new VirIReg();
                            virIReg1.setPhyReg(regStore.getReservedReg());
                            ASMInstruction li = instrFactory.createLi(store.getOffset(), null, virIReg1);
                            li.insertBefore(instr);

                            VirIReg virIReg2 = new VirIReg();
                            virIReg2.setPhyReg(regStore.getReservedReg());
                            virIReg2.setDouble(true);
                            ASMInstruction add = instrFactory.createAdd((Reg) store.getBase(), li.getRegister(),
                                    null, virIReg2);
                            add.insertBefore(instr);
                            ((ASMInstruction) store).resetOperands(
                                    List.of(add.getRegister(), store.getValue(), new ASMImmediate(0))
                            );  // 重置偏移量为 0
                        }
                        continue;
                    }
                    List<ASMValue> operands = instr.getOperands();
                    if (operands == null || operands.isEmpty()) {
                        continue;
                    }
                    List<ASMValue> newOperands = new ArrayList<>();
                    for (ASMValue operand : operands) {
                        if (operand instanceof ASMImmediate imm) {
                            int immValue = imm.getImm();
                            if (immValue > 2047 || immValue < -2048) {
                                VirIReg virIReg = new VirIReg();
                                virIReg.setPhyReg(regStore.getReservedReg());
                                ASMInstruction li = instrFactory.createLi(imm, null, virIReg);
                                li.insertBefore(instr);
                                newOperands.add(li.getRegister());
                            } else {
                                newOperands.add(imm);
                            }
                        } else {
                            newOperands.add(operand);
                        }
                    }
                    instr.resetOperands(newOperands);
                }
                for (ASMInstruction instr : instructionsToRemove) {
                    instr.removeFromList();
                }
            }
        }
    }

    private void fixUsageOfSP(ASMModel asmModel) {    // 针对 ARM，有些指令不能直接使用 SP，需要先 move 到通用寄存器
        for (ASMFunction asmFunction : asmModel.getFunctionList()) {
            for (CustomList.Node blockNode : asmFunction.getBasicBlockList()) {
                ASMBasicBlock block = (ASMBasicBlock) blockNode;
                for (CustomList.Node instrNode : block.getInstructionList()) {
                    ASMInstruction instr = (ASMInstruction) instrNode;
                    if (instr instanceof ARMStore armStore && armStore.getValue().printAsOperand().equals("SP")) {
                        VirIReg virIReg = new VirIReg();
                        virIReg.setDouble(true);
                        virIReg.setPhyReg(regStore.getReservedReg());

                        ARMMove move = new ARMMove(regStore.getStackPtr(), null, virIReg);
                        move.insertBefore(armStore);

                        armStore.replaceUsedVal(armStore.getValue(), virIReg);
                    } else if (instr instanceof ARMMadd madd && madd.getOperands().get(2).printAsOperand().equals("SP")) {
                        VirIReg virIReg = new VirIReg();
                        virIReg.setDouble(true);
                        virIReg.setPhyReg(regStore.getReservedReg());

                        ARMMove move = new ARMMove(regStore.getStackPtr(), null, virIReg);
                        move.insertBefore(madd);

                        madd.replaceUsedVal(madd.getOperands().get(2), virIReg);
                    }
                }
            }
        }
    }
}
