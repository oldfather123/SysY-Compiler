package backend.opt.riscv;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.rv.arithmetic.*;
import backend.asm.instr.rv.memory.RVLoad;
import backend.asm.instr.rv.memory.RVStore;
import backend.asm.register.Reg;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirIReg;
import backend.asm.register.vir.VirReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMModel;
import util.CustomList;

import java.util.List;

public class InsSelect4RV {
    public static void execute4phy(ASMModel asmModel, RegStore regStore) {
        List<ASMFunction> functions = asmModel.getFunctionList();
        mergeLoadStore(functions, regStore);  // 将两个相邻的 32 位 store/load 拼成 64 位一起存取
    }

    /**
     * todo: 这个未必是正收益，可以考虑只保留合并两个 0
     *  然后注意现在还没写两条访存指令不直接相邻的情况！
     */
    private static void mergeLoadStore(List<ASMFunction> functionList, RegStore regStore) {
        for (ASMFunction function : functionList) {
            for (CustomList.Node node : function.getBasicBlockList()) {
                ASMBasicBlock  blk = (ASMBasicBlock) node;
                ASMInstruction ins = (ASMInstruction) blk.getInstructionList().getHead();
                ASMInstruction nxt;
                while (ins != null) {
                    nxt = (ASMInstruction) ins.getNext();
                    
                    if (ins instanceof RVStore store1 && !store1.isDouble()) {
                        ASMValue base1 = store1.getBase();
                        int offset1 = store1.getOffset().getImm();
                        if ((offset1 & 7) != 0) {
                            ins = nxt;
                            continue;
                        }
                        if (!(nxt instanceof RVStore store2 && !store2.isDouble())) {
                            ins = nxt;
                            continue;
                        }
                        ASMValue base2 = store2.getBase();
                        int offset2 = store2.getOffset().getImm();
                        if (base1 != base2 || store1.isFloat() || store2.isFloat()) { // todo 都是 float 行不行
                            ins = nxt;
                            continue;
                        }
                        if (offset2 - offset1 == 4 && offset1 <= 2040 && offset1 >= -2048) {
                            Reg value1ToStore = (Reg) store1.getValue();
                            Reg value2ToStore = (Reg) store2.getValue();
                            if (value1ToStore == value2ToStore && value1ToStore == regStore.getZero()) {
                                // 都是 zero 就不用拼了，直接存zero
                                RVStore storeZero = new RVStore(store1.getOffset(), base1,
                                        value1ToStore, null, true, false);
                                storeZero.insertBefore(store1);
                            } else {
                                if (value2ToStore instanceof VirIReg virIReg) {
                                    virIReg.setDouble(true);
                                }
                                RVSll sllHigh   = new RVSll(value2ToStore, new ASMImmediate(32), null, value2ToStore);
                                VirIReg tmp = new VirIReg();
                                tmp.setDouble(true);
                                tmp.setPhyReg(regStore.getReservedReg());
                                RVSll sllClear;
                                RVSrl srlLow;
                                if (value1ToStore.printAsOperand().equals(value2ToStore.printAsOperand())) {
                                    sllClear = null;
                                    srlLow = new RVSrl(value1ToStore, new ASMImmediate(32), null, tmp);
                                } else {
                                    sllClear = new RVSll(value1ToStore, new ASMImmediate(32), null, tmp);
                                    srlLow = new RVSrl(tmp, new ASMImmediate(32), null, tmp);
                                }
                                RVOr  orMerge   = new RVOr(value2ToStore, tmp, null, tmp);
                                RVStore storePair = new RVStore(store1.getOffset(), base1, tmp, null,
                                        true, false);
                                RVSra sraRestore  = new RVSra(value2ToStore, new ASMImmediate(32), null, value2ToStore);

                                sllHigh.insertBefore(store1);
                                if (sllClear != null) {
                                    sllClear.insertBefore(store1);
                                }
                                srlLow.insertBefore(store1);
                                orMerge.insertBefore(store1);
                                storePair.insertBefore(store1);
                                sraRestore.insertBefore(store1);
                            }

                            store1.removeFromList();
                            store2.removeFromList();
                            nxt = (ASMInstruction) nxt.getNext();
                        }
                    } else if (ins instanceof RVLoad load1 && !load1.isDouble()) {
                        ASMValue base1 = load1.getBase();
                        int offset1 = load1.getOffset().getImm();
                        if ((offset1 & 7) != 0) {
                            ins = nxt;
                            continue;
                        }
                        if (!(nxt instanceof RVLoad load2 && !load2.isDouble())) {
                            ins = nxt;
                            continue;
                        }
                        ASMValue base2 = load2.getBase();
                        int offset2 = load2.getOffset().getImm();
                        if (base1 != base2 || load1.isFloat() || load2.isFloat()) { // todo 都是 float 行不行
                            ins = nxt;
                            continue;
                        }
                        if (offset2 - offset1 == 4 && offset1 <= 2040 && offset1 >= -2048) {
                            VirIReg tmp = new VirIReg();
                            tmp.setDouble(true);
                            tmp.setPhyReg(regStore.getReservedReg());
                            RVLoad loadPair = new RVLoad(load1.getOffset(), base1,
                                    null, tmp, true, false);
                            RVAdd  getLo = new RVAdd(tmp, regStore.getZero(), null, load1.getRegister());
                            if (load2.getRegister() instanceof VirIReg virIReg) {
                                virIReg.setDouble(true);
                            }
                            RVSra  getHi = new RVSra(tmp, new ASMImmediate(32), null, load2.getRegister());

                            loadPair.insertBefore(load1);
                            getLo.insertBefore(load1);
                            getHi.insertBefore(load1);

                            load1.removeFromList();
                            load2.removeFromList();
                            nxt = (ASMInstruction) nxt.getNext();
                        }
                    }
                    
                    ins = nxt;
                }
            }
        }
    }

    /**
     * 这个方法由于比赛的评测设备不支持 sh2add 指令而不能使用
     * 即使使用也应该修改一下放到 execute4vir 里面
     */
    public static void createSh2Add(ASMFunction function) {
        for (CustomList.Node blockNode : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) blockNode;
            ASMInstruction instr = (ASMInstruction) basicBlock.getInstructionList().getHead();
            ASMInstruction nextInstr;
            while (instr != null) {
                nextInstr = (ASMInstruction) instr.getNext();
                if (instr instanceof RVSll sll
                        && sll.getOperand2() instanceof ASMImmediate
                        && sll.getShiftImm().getImm() == 2
                        && nextInstr instanceof RVAdd add
                        && add.getOperand2() == sll.getRegister()
                        && sll.getRegister() instanceof VirReg virReg
                        && virReg.getUserSet().size() == 1) {
                    RVSh2Add sh2Add = new RVSh2Add((Reg) add.getOperand1(), sll.getOperand1(), null, add.getRegister());
                    sh2Add.insertBefore(sll);
                    sll.removeFromList();
                    add.removeFromList();
                }
                instr = nextInstr;
            }
        }
    }
}
