package backend.opt.arm;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.arm.ARMCond;
import backend.asm.instr.arm.binary.ARMAdd;
import backend.asm.instr.arm.binary.ARMMul;
import backend.asm.instr.arm.binary.ARMSub;
import backend.asm.instr.arm.cmp.ARMCmp;
import backend.asm.instr.arm.cmp.ARMCset;
import backend.asm.instr.arm.jump.ARMBl;
import backend.asm.instr.arm.jump.ARMBranch;
import backend.asm.instr.arm.jump.ARMCbnz;
import backend.asm.instr.arm.jump.ARMCbz;
import backend.asm.instr.arm.memory.ARMLoad;
import backend.asm.instr.arm.memory.ARMLoadPair;
import backend.asm.instr.arm.memory.ARMStore;
import backend.asm.instr.arm.memory.ARMStorePair;
import backend.asm.instr.arm.neon.ARMVecMoveFromSca;
import backend.asm.instr.arm.neon.ARMVecMoveImm;
import backend.asm.instr.arm.neon.binary.ARMVecEor;
import backend.asm.instr.arm.others.ARMLi;
import backend.asm.instr.arm.others.ARMMove;
import backend.asm.instr.arm.ternary.ARMMadd;
import backend.asm.instr.arm.ternary.ARMMsub;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyReg;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirFReg;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMModel;
import util.CustomList;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class InsSelect4ARM {
    /**
     * 针对虚拟寄存器进行的指令选择优化
     */
    public static void execute4vir(ASMModel asmModel, RegStore regStore, boolean opt) {
        List<ASMFunction> functionList = asmModel.getFunctionList();
        memsetVectorization(functionList);                          // 可确定长度的 memset 展开成向量化存储 0
        vecMoveOpt(functionList, regStore.getZero());               // 给向量寄存器加载相同立即数可以变成广播指令
        mergeMAS(functionList); // todo: 位移-加减 也能合并，比如 `ADD Rd, Rn, Rm, LSL #n` 乘法优化上线之后尝试做一下
        if (opt) {
            simplifyBranchIns(functionList, regStore);  // 这个要求中端把能简化的分支全部简化，但是 longline 可能实在简化不掉了
        }
    }

    /**
     * 针对物理寄存器进行指令选择的优化
     */
    public static void execute4phy(ASMModel asmModel) {
        List<ASMFunction> functionList = asmModel.getFunctionList();
        mergeLoadStore(functionList);   // todo 受立即数位数限制，很多没能合并
    }

    private static void mergeLoadStore(List<ASMFunction> functionList) {
        for (ASMFunction function : functionList) {
            for (CustomList.Node node : function.getBasicBlockList()) {
                ASMBasicBlock  blk = (ASMBasicBlock) node;
                ASMInstruction ins = (ASMInstruction) blk.getInstructionList().getHead();
                ASMInstruction nxt;
                while (ins != null) {
                    nxt = (ASMInstruction) ins.getNext();

                    if (ins instanceof ARMStore store1) {
                        ASMValue base1 = store1.getBase();
                        String valOp1 = store1.getValue().printAsOperand();
                        int offset1 = store1.getOffset().getImm();
                        if (!(nxt instanceof ARMStore store2)) {
                            ins = nxt;
                            continue;
                        }
                        ASMValue base2 = store2.getBase();
                        String valOp2 = store2.getValue().printAsOperand();
                        int offset2 = store2.getOffset().getImm();
                        if (base1 != base2) {
                            ins = nxt;
                            continue;
                        }
                        if (canMerge(valOp1, valOp2, offset1, offset2)) {
                            ARMStorePair storePair = new ARMStorePair(store1.getOffset(), base1,
                                    store1.getValue(), store2.getValue(), null, false);
                            storePair.insertBefore(store1);
                            store1.removeFromList();
                            store2.removeFromList();
                        }
                    } else if (ins instanceof ARMLoad load1) {
                        ASMValue base1 = load1.getBase();
                        String valOp1 = load1.printAsOperand();
                        int offset1 = load1.getOffset().getImm();
                        if (!(nxt instanceof ARMLoad load2)) {
                            ins = nxt;
                            continue;
                        }
                        ASMValue base2 = load2.getBase();
                        String valOp2 = load2.printAsOperand();
                        int offset2 = load2.getOffset().getImm();
                        if (base1 != base2) {
                            ins = nxt;
                            continue;
                        }
                        if (canMerge(valOp1, valOp2, offset1, offset2)) {
                            ARMLoadPair loadPair = new ARMLoadPair(load1.getOffset(), base1,
                                    null, load1.getRegister(), load2.getRegister());
                            loadPair.insertBefore(load1);
                            load1.removeFromList();
                            load2.removeFromList();
                        }
                    }

                    ins = nxt;
                }
            }
        }
    }

    private static boolean canMerge(String valOp1, String valOp2, int offset1, int offset2) {
        boolean bothX = valOp1.contains("X") && (offset1 & 15) == 0 &&
                valOp2.contains("X") && offset2 - offset1 == 8 && offset1 <= 504 && offset1 >= -512;
        boolean bothW = valOp1.contains("W") && (offset1 & 7) == 0 &&
                valOp2.contains("W") && offset2 - offset1 == 4 && offset1 <= 252 && offset1 >= -256;
        boolean bothS = valOp1.contains("S") && (offset1 & 7) == 0 &&
                valOp2.contains("S") && offset2 - offset1 == 4 && offset1 <= 252 && offset1 >= -256;
        return  bothX || bothW || bothS;
    }

    /**
     * 简化有条件跳转指令，指将一系列比较指令简化成简单的几条
     */
    private static void simplifyBranchIns(List<ASMFunction> functionList, RegStore regStore) {
        for (ASMFunction function : functionList) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) function.getBasicBlockList().getTail();
            while (basicBlock != null) {
                ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getTail();

                while (!(ins instanceof ARMBranch) && ins != null) {
                    ins = (ASMInstruction) ins.getPrev();
                }
                ARMBranch branch = (ARMBranch) ins;
                if (branch == null || branch.getCond() != ARMCond.EQ) {
                    basicBlock = (ASMBasicBlock) basicBlock.getPrev();
                    continue;   // 这个块没有可处理的条件跳转
                }
                if (!(branch.getPrev() instanceof ARMCmp armCmp1 && armCmp1.getOperands().getFirst() == regStore.getZero())) {
                    throw new RuntimeException("exc1");
                }

                if (!(armCmp1.getPrev() instanceof ARMCset armCset1)) {
                    // 这个块的跳转条件在别的块里算完了，或者本身很简单
                    if (!(armCmp1.getOperands().get(1) instanceof VirIReg op
                            && op.getDefInsSet().size() == 1
                            && op.getDefInsSet().getFirst() instanceof ARMCset armCset)) {
                        basicBlock = (ASMBasicBlock) basicBlock.getPrev();
                        continue;   // 简单判断，不用简化了
                    }
                    if (!(armCset.getPrev() instanceof ARMCmp armCmp)) {
                        throw new RuntimeException("exc1.2");
                    }

                    List<ASMValue> cmpOpList = armCmp.getOperands();
                    ARMCmp clonedCmp = new ARMCmp((Reg) cmpOpList.get(0), cmpOpList.get(1), null);
                    clonedCmp.insertBefore(armCmp1);

                    ARMCset clonedCset = new ARMCset(armCset.getCond(), null, armCset.getRegister());
                    clonedCset.insertBefore(armCmp1);

                    // 返回这个块再算一遍，因此不向前迭代
                    continue;
                }
                if (!(armCset1.getPrev() instanceof ARMCmp armCmp2)) {
                    throw new RuntimeException("exc2");
                }

                if (armCset1.getCond() == ARMCond.NE) {
                    Reg op1 = (Reg) armCmp2.getOperands().getFirst();
                    ASMValue op2 = armCmp2.getOperands().get(1);
                    if (op1 == regStore.getZero() && op2 instanceof Reg) {
                        ARMCbz cbz = new ARMCbz(branch.getTarget(), (Reg) op2, null);
                        cbz.insertBefore(branch);
                        branch.removeFromList();
                        if (armCset1.getRegister().getUserSet().isEmpty()) {
                            armCmp2.removeFromList();
                        }
                    } else if (op2 == regStore.getZero() || op2 instanceof ASMImmediate i && i.getImm() == 0) {
                        ARMCbz cbz = new ARMCbz(branch.getTarget(), op1, null);
                        cbz.insertBefore(branch);
                        branch.removeFromList();
                        if (armCset1.getRegister().getUserSet().isEmpty()) {
                            armCmp2.removeFromList();
                        }
                    } else {
                        branch.setCond(armCset1.getCond().getOpposite());
                    }
                } else if (armCset1.getCond() == ARMCond.EQ) {
                    Reg op1 = (Reg) armCmp2.getOperands().getFirst();
                    ASMValue op2 = armCmp2.getOperands().get(1);
                    if (op1 == regStore.getZero() && op2 instanceof Reg) {
                        ARMCbnz cbnz = new ARMCbnz(branch.getTarget(), (Reg) op2, null);
                        cbnz.insertBefore(branch);
                        branch.removeFromList();
                        if (armCset1.getRegister().getUserSet().isEmpty()) {
                            armCmp2.removeFromList();
                        }
                    } else if (op2 == regStore.getZero() || op2 instanceof ASMImmediate i && i.getImm() == 0) {
                        ARMCbnz cbnz = new ARMCbnz(branch.getTarget(), op1, null);
                        cbnz.insertBefore(branch);
                        branch.removeFromList();
                        if (armCset1.getRegister().getUserSet().isEmpty()) {
                            armCmp2.removeFromList();
                        }
                    } else {
                        branch.setCond(armCset1.getCond().getOpposite());
                    }
                } else {
                    branch.setCond(armCset1.getCond().getOpposite());
                }

                armCmp1.removeFromList();
                if (armCset1.getRegister().getUserSet().isEmpty()) {
                    armCset1.removeFromList();
                }

                basicBlock = (ASMBasicBlock) basicBlock.getPrev();
            }
        }
    }

    private static void memsetVectorization(List<ASMFunction> functionList) {
        for (ASMFunction function : functionList) {
            for (CustomList.Node node : function.getBasicBlockList()) {
                ASMBasicBlock basicBlock = (ASMBasicBlock) node;
                ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
                while (ins != null) {
                    if (ins instanceof ARMBl bl && bl.getTargetFunc().toString().equals("memset")) {
                        if (bl.getPrev() instanceof ARMLi loadLenConst) {
                            if (!(loadLenConst.getPrev() instanceof ARMMove moveZero
                                    && moveZero.getPrev() instanceof ARMMove movePtr)) {
                                throw new RuntimeException("memset 相关格式出现了意料之外的情况");
                            }

                            VirFReg virFReg1 = new VirFReg(true);
                            ARMVecEor vecEor1 = new ARMVecEor(virFReg1, virFReg1, null, virFReg1);
                            vecEor1.insertBefore(moveZero);

                            int len = loadLenConst.getImmediate().getImm();
                            int curPos = 0;
                            ASMValue base = movePtr.getRegister();
                            if (base instanceof PhyReg phyReg) {
                                base = new VirIReg();
                                ((VirIReg) base).setDouble(true);
                                ((VirIReg) base).setPhyReg(phyReg);
                            }
                            while (len >= 32) {
                                ARMStorePair storePair = new ARMStorePair(new ASMImmediate(32), base, virFReg1, virFReg1, null, true);
                                storePair.insertBefore(moveZero);
                                // 使用了每次迭代指针的 stp，这里不更新相对位置
                                len -= 32;
                            }
                            while (len > 0) {  // 规定数组对象空间后 16 字节对齐后可以放心多清空一点
                                ARMStore store = new ARMStore(new ASMImmediate(curPos), base, virFReg1, null);
                                store.insertBefore(moveZero);
                                curPos += 16;
                                len -= 16;
                            }

                            moveZero.removeFromList();
                            loadLenConst.removeFromList();
                            ins.removeFromList();
                        } else if (bl.getPrev() instanceof ARMMove loadLen) {
                            if (!(loadLen.getPrev() instanceof ARMMove moveZero
                                    && moveZero.getPrev() instanceof ARMMove movePtr)) {
                                throw new RuntimeException("memset 相关格式出现了意料之外的情况");
                            }
                            // todo 感觉对于长度不能在编译时确定的 memset 转成向量指令性价比太低了，先不做了
                        } else {
                            throw new RuntimeException("memset <UNK>");
                        }
                    }
                    ins = (ASMInstruction) ins.getNext();
                }
            }
        }
    }

    private static void vecMoveOpt(List<ASMFunction> functionList, PhyReg zero) {
        for (ASMFunction function : functionList) {
            for (CustomList.Node node : function.getBasicBlockList()) {
                ASMBasicBlock basicBlock = (ASMBasicBlock) node;
                ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
                while (ins != null) {
                    if (ins instanceof ARMVecMoveFromSca vecMoveFromSca
                            && vecMoveFromSca.getRegister() instanceof VirFReg virFReg
                            && virFReg.getDefInsSet().size() == 1) {
                        List<ASMValue> srcValList = vecMoveFromSca.getOperands();
                        ASMValue src0 = srcValList.get(0);
                        ASMValue src1 = srcValList.get(1);
                        ASMValue src2 = srcValList.get(2);
                        ASMValue src3 = srcValList.get(3);

                        if (src0 == zero && src1 == zero && src2 == zero && src3 == zero) {
                            VirFReg virFReg1 = new VirFReg(true);
                            ARMVecEor vecEor1 = new ARMVecEor(virFReg1, virFReg1, null, virFReg1);
                            vecEor1.insertBefore(ins);
                            virFReg.replaceWith(virFReg1);
                            ins.removeFromList();
                        } else if (src0 instanceof VirIReg virIReg0 && virIReg0.getDefInsSet().size() == 1
                                && virIReg0.getDefInsSet().getFirst() instanceof ARMLi li0
                                && src1 instanceof VirIReg virIReg1 && virIReg1.getDefInsSet().size() == 1
                                && virIReg1.getDefInsSet().getFirst() instanceof ARMLi li1
                                && src2 instanceof VirIReg virIReg2 && virIReg2.getDefInsSet().size() == 1
                                && virIReg2.getDefInsSet().getFirst() instanceof ARMLi li2
                                && src3 instanceof VirIReg virIReg3 && virIReg3.getDefInsSet().size() == 1
                                && virIReg3.getDefInsSet().getFirst() instanceof ARMLi li3
                                && li0.getImmediate().getImm() == li1.getImmediate().getImm()
                                && li1.getImmediate().getImm() == li2.getImmediate().getImm()
                                && li2.getImmediate().getImm() == li3.getImmediate().getImm()) {
                            VirFReg virFReg1 = new VirFReg(true);
                            ARMVecMoveImm moveImm = new ARMVecMoveImm(li0.getImmediate(), null, virFReg1);
                            moveImm.insertBefore(ins);
                            virFReg.replaceWith(virFReg1);
                            ins.removeFromList();
                            li0.removeFromList();
                            li1.removeFromList();
                            li2.removeFromList();
                            li3.removeFromList();
                        }
                    }
                    ins = (ASMInstruction) ins.getNext();
                }
            }
        }
    }

    /**
     * 将可以合并的乘法+加法、乘法+减法组合合并成单一指令
     */
    private static void mergeMAS(List<ASMFunction> functionList) {
        for (ASMFunction function : functionList) {
            for (CustomList.Node node : function.getBasicBlockList()) {
                ASMBasicBlock basicBlock = (ASMBasicBlock) node;

                Map<Reg, ARMMul> mulMap = new LinkedHashMap<>();
                List<ARMAdd> addList = new ArrayList<>();
                List<ARMSub> subList = new ArrayList<>();
                findMAS(basicBlock, mulMap, addList, subList);

                for (ARMAdd add : addList) {
                    try2genMADD(add, mulMap);
                }
                for (ARMSub sub : subList) {
                    try2genMSUB(sub, mulMap);
                }
            }
        }
    }

    private static void try2genMADD(ARMAdd add, Map<Reg, ARMMul> mulMap) {
        List<ASMValue> addOperands = add.getOperands();
        ARMMul mul;
        ARMMadd madd;
        if (addOperands.getFirst() instanceof Reg reg && mulMap.containsKey(reg)) {
            mul = mulMap.get(reg);
            List<ASMValue> mulOperands = mul.getOperands();
            Reg addOp2;
            if (addOperands.get(1) instanceof ASMImmediate immediate) {
                addOp2 = new VirIReg();
                ARMLi li = new ARMLi(immediate, null, addOp2);
                li.insertBefore(add);
            } else {
                addOp2 = (Reg) addOperands.get(1);
            }
            VirIReg virIReg = new VirIReg();
            virIReg.setDouble(addOp2 instanceof VirIReg op && op.isDouble());
            add.getRegister().replaceWith(virIReg);
            madd = new ARMMadd((Reg) mulOperands.getFirst(), (Reg) mulOperands.get(1), addOp2, null, virIReg);
        } else if (addOperands.get(1) instanceof Reg reg && mulMap.containsKey(reg)) {
            mul = mulMap.get(reg);
            List<ASMValue> mulOperands = mul.getOperands();
            Reg addOp1 = (Reg) addOperands.getFirst();
            VirIReg virIReg = new VirIReg();
            virIReg.setDouble(addOp1 instanceof VirIReg op && op.isDouble());
            add.getRegister().replaceWith(virIReg);
            madd = new ARMMadd((Reg) mulOperands.getFirst(), (Reg) mulOperands.get(1), addOp1, null, virIReg);
        } else {
            return;
        }

        madd.insertBefore(add);
        add.removeFromList();
        mul.removeFromList();
    }

    private static void try2genMSUB(ARMSub sub, Map<Reg, ARMMul> mulMap) {
        List<ASMValue> subOperands = sub.getOperands();
        if (subOperands.get(1) instanceof Reg reg && mulMap.containsKey(reg)) {
            ARMMul mul = mulMap.get(reg);
            List<ASMValue> mulOperands = mul.getOperands();
            Reg subOp1 = (Reg) subOperands.getFirst();
            VirIReg virIReg = new VirIReg();
            sub.getRegister().replaceWith(virIReg);
            ARMMsub msub = new ARMMsub((Reg) mulOperands.getFirst(), (Reg) mulOperands.get(1), subOp1, null, virIReg);

            msub.insertBefore(sub);
            sub.removeFromList();
            mul.removeFromList();
        }
    }

    /**
     * 寻找基本块内的整数乘法、加法、减法指令，便于后续尝试合并
     * 其中要求乘法的结果有且只有一个使用者（这样才可以合并）
     * 加减法所定义的寄存器只能被定义过一次，否则替换可能出问题
     */
    private static void findMAS(ASMBasicBlock basicBlock, Map<Reg, ARMMul> mulMap,
                                List<ARMAdd> addList, List<ARMSub> subList) {
        for (CustomList.Node node : basicBlock.getInstructionList()) {
            ASMInstruction ins = (ASMInstruction) node;
            if (ins instanceof ARMMul && ins.getRegister().getUserSet().size() == 1) {
                mulMap.put(ins.getRegister(), (ARMMul) ins);
            } else if (ins instanceof ARMAdd && ins.getRegister() instanceof VirIReg virIReg
                    && virIReg.getDefInsSet().size() == 1) {
                addList.add((ARMAdd) ins);
            } else if (ins instanceof ARMSub && ins.getRegister() instanceof VirIReg virIReg
                    && virIReg.getDefInsSet().size() == 1) {
                subList.add((ARMSub) ins);
            }
        }
    }
}
