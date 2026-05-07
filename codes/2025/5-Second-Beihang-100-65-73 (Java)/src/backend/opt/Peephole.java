package backend.opt;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.insfact.InstrFactory;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.rv.branch.RVBranch;
import backend.asm.instr.rv.jump.RVJ;
import backend.asm.instr.rv.memory.RVLoad;
import backend.asm.instr.rv.others.RVLa;
import backend.asm.instr.tags.*;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyIReg;
import backend.asm.register.phy.PhyReg;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirIReg;
import backend.asm.register.vir.VirReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMModel;
import util.CustomList;

import java.util.*;

/**
 * 窥孔优化，针对局部特征进行优化，会删掉/重排一些指令/基本块，可能破坏数据流/控制流分析、def-use 记录等，因此应该放在最后
 */
public class Peephole {
    public static void execute4phy(ASMModel asmModel, RegStore regStore, InstrFactory instrFactory, boolean runRV) {
        List<ASMFunction> functions = asmModel.getFunctionList();
        PhyReg retAddrReg = regStore.getRetAddr();

        for (ASMFunction function : functions) {
            removeUselessMove(function);                    // 删除相同物理寄存器之间的 move，没有维护 def-use
            removeUselessLoad(function, instrFactory);      // 删除紧跟在同寄存器、同地址 store 后面的 load（同地址不同寄存器换成 move）
            removeUselessSaveOfRa(function, retAddrReg);    // 删除不必要的 RA 保存（对于没有调用别人的函数，RA 不会变）
            removeUselessJump(function, runRV);             // 删除没用的（直接跳到下个块的）跳转，包含重排和空块删除，做完之后跳转关系不太好分析
        }
    }

    public static void execute4vir(ASMModel asmModel, RegStore regStore, InstrFactory instrFactory) {
        List<ASMFunction> functions = asmModel.getFunctionList();
        PhyReg zeroReg = regStore.getZero();

        for (ASMFunction function : functions) {
            mergeLoadZero(function, zeroReg, instrFactory); // 删除对立即数 0 的加载，换成零寄存器
            deleteUselessMove(function, regStore);          // 删除没用的 move，主要是方便后续合并加立即数
            mergeAddi(function);                            // 合并连续的 addi 为一个，也是方便合并 addi 和 load/store
            mergeMemInsWithAddi(function);                  // 如果内存操作的基指针来自 addi，则进行合并，方便后续针对直接存取栈上固定位置操作的优化
            deleteUnusedInstr(function);
        }
    }

    public static void execute4virAfterInsSel(ASMModel asmModel, RegStore regStore, InstrFactory instrFactory) {
        List<ASMFunction> functions = asmModel.getFunctionList();

        for (ASMFunction function : functions) {
            for (ASMLoop loop : function.getAncientLoopInfo()) {
                loopInvariantsExtraction(loop, instrFactory);
            }
            localValueNumbering(function);
        }
    }

    private static void loopInvariantsExtraction(ASMLoop loop, InstrFactory instrFactory) {
        for (ASMLoop childLoop : loop.getChildLoopSet()) {
            loopInvariantsExtraction(childLoop, instrFactory);
        }

        ASMBasicBlock preHeader = loop.getPreHeader();
        ASMInstruction lastIns = (ASMInstruction) preHeader.getInstructionList().getTail();
        if (lastIns.getPrev() instanceof BranchIns) {
            lastIns = (ASMInstruction) lastIns.getPrev();
        }

        for (ASMBasicBlock basicBlock : loop.getBodyBlocks()) {
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            ASMInstruction nxt;
            RVLa lastLa = null;
            while (ins != null) {
                nxt = (ASMInstruction) ins.getNext();

                if (ins.getRegister() instanceof VirReg virReg
                        && virReg.getDefInsSet().size() == 1) {
                    switch (ins) {
                        case LiIns liIns -> {
                            ASMInstruction newLi = instrFactory.createLi(liIns.getImmediate(), null, virReg);
                            newLi.insertBefore(lastIns);
                            ins.removeFromList();
                            lastLa = null;
                        }
                        case RVLa oldRa -> {
                            RVLa newLa = new RVLa(oldRa.getGlobalObject(), null, virReg);
                            newLa.insertBefore(lastIns);
                            ins.removeFromList();
                            lastLa = newLa;
                        }
                        case RVLoad load -> {
                            if (load.isFltLi() && load.getOffset().getImm() == 0 && lastLa != null
                                    && load.getBase() == lastLa.getRegister()) {  // 其实是浮点数 li
                                RVLoad newLoad = new RVLoad(lastLa.getRegister(), null, load.getRegister(), false, true);
                                newLoad.insertBefore(lastIns);
                                ins.removeFromList();
                            }
                            lastLa = null;
                        }
                        default -> lastLa = null;
                    }
                } else {
                    lastLa = null;
                }

                ins = nxt;
            }
        }
    }

    private static void localValueNumbering(ASMFunction function) {
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;

            Map<String, VirReg> valueMap = new LinkedHashMap<>();

            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            ASMInstruction nxt;
            while (ins != null) {
                nxt = (ASMInstruction) ins.getNext();

                if (ins.getRegister() instanceof VirReg virReg
                        && virReg.getDefInsSet().size() == 1
                        && ins instanceof PureIns) {
                    String hash = ins.hash();
                    if (!valueMap.containsKey(hash)) {
                        valueMap.put(hash, virReg);
                    } else {
                        virReg.replaceWith(valueMap.get(hash));
                        ins.removeFromList();
                    }
                }

                ins = nxt;
            }
        }
    }

    private static void mergeMemInsWithAddi(ASMFunction function) {
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            ASMInstruction nxt;
            while (ins != null) {
                nxt = (ASMInstruction) ins.getNext();

                if (ins instanceof LoadIns loadIns) {
                    if (loadIns.getBase() instanceof VirReg base
                            && base.getDefInsSet().size() == 1
                            && base.getDefInsSet().getFirst() instanceof AddIns addIns
                            && addIns.getOperand2() instanceof ASMImmediate addImm) {
                        ASMImmediate newOffset = new ASMImmediate(loadIns.getOffset().getImm() + addImm.getImm());
                        ins.resetOperands(Arrays.asList(addIns.getOperand1(), newOffset));

                        Set<ASMInstruction> userSet = ((ASMInstruction) addIns).getRegister().getUserSet();
                        if (userSet.isEmpty()) {
                            ((ASMInstruction) addIns).removeFromList();
                        }
                    }
                } else if (ins instanceof StoreIns storeIns) {
                    if (storeIns.getBase() instanceof VirReg base
                            && base.getDefInsSet().size() == 1
                            && base.getDefInsSet().getFirst() instanceof AddIns addIns
                            && addIns.getOperand2() instanceof ASMImmediate addImm) {
                        ASMImmediate newOffset = new ASMImmediate(storeIns.getOffset().getImm() + addImm.getImm());
                        ins.resetOperands(Arrays.asList(addIns.getOperand1(), storeIns.getValue(), newOffset));

                        Set<ASMInstruction> userSet = ((ASMInstruction) addIns).getRegister().getUserSet();
                        if (userSet.isEmpty()) {
                            ((ASMInstruction) addIns).removeFromList();
                        }
                    }
                }

                ins = nxt;
            }
        }
    }

    private static void mergeAddi(ASMFunction function) {
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            ASMInstruction nxt;
            while (ins != null) {
                nxt = (ASMInstruction) ins.getNext();

                if (ins instanceof AddIns addIns && addIns.getOperand2() instanceof ASMImmediate imm
                        && ins.getRegister() instanceof VirReg virReg && virReg.getDefInsSet().size() == 1
                        && virReg.getUserSet().size() == 1
                        && virReg.getUserSet().iterator().next() instanceof AddIns userAdd
                        && userAdd.getOperand2() instanceof ASMImmediate userImm) {
                    List<ASMValue> newOpList = Arrays.asList(
                            addIns.getOperand1(), new ASMImmediate(imm.getImm() + userImm.getImm())
                    );
                    ((ASMInstruction) userAdd).resetOperands(newOpList);
                    ins.removeFromList();
                }

                ins = nxt;
            }
        }
    }

    private static void deleteUselessMove(ASMFunction function, RegStore regStore) {
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            ASMInstruction nxt;
            while (ins != null) {
                nxt = (ASMInstruction) ins.getNext();

                // fixme 这里对来源为 SP 的 move 进行了删除，可能会在 ARM 架构下引起一些问题（因为部分 ARM 指令不能直接用 SP 寄存器）
                //  依赖 PostProcess 中的 fixUsageOfSP 进行修复，但是可能没修全，出问题再修
                if (ins instanceof MoveIns moveIns) {
                    if (ins.getRegister() instanceof VirReg virReg && virReg.getDefInsSet().size() == 1) {
                        if ((moveIns.getSrc() instanceof VirReg srcVirReg && (srcVirReg.getDefInsSet().size() == 1
                                || srcVirReg.getPhyReg() == regStore.getStackPtr() || srcVirReg.getPhyReg() == regStore.getZero())
                                || moveIns.getSrc() instanceof PhyIReg srcPhyIReg
                                && (srcPhyIReg == regStore.getStackPtr() || srcPhyIReg == regStore.getZero()))) {
                            Reg tar = moveIns.getSrc();
                            if (moveIns.getSrc() instanceof PhyIReg srcPhyIReg && srcPhyIReg == regStore.getStackPtr()) {
                                VirIReg virIReg = new VirIReg();
                                virIReg.setDouble(true);
                                virIReg.setPhyReg(regStore.getStackPtr());
                                tar = virIReg;
                            }
                            virReg.replaceWith(tar);
                            ins.removeFromList();
                        }
                    } else if (ins.getRegister() instanceof PhyReg phyReg
                            && moveIns.getSrc() instanceof VirReg srcVirReg
                            && srcVirReg.getDefInsSet().size() == 1
                            && srcVirReg.getUserSet().size() == 1
                            && srcVirReg.getDefInsSet().getFirst() == ins.getPrev()) {
                        // todo 可能能把最后一条换成更泛化的 check，即 def-use 之间没有对该物理寄存器的再次定义
                        srcVirReg.getDefInsSet().getFirst().changeRegTo(phyReg);
                        srcVirReg.replaceWith(phyReg);
                        ins.removeFromList();
                    }

                }

                ins = nxt;
            }
        }
    }

    private static void mergeLoadZero(ASMFunction function, PhyReg zeroReg, InstrFactory instrFactory) {
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            ASMInstruction nxt;
            while (ins != null) {
                nxt = (ASMInstruction) ins.getNext();

                if (ins instanceof LiIns li && li.getImmediate().getImm() == 0) {
                    if (ins.getRegister() instanceof VirReg virReg && virReg.getDefInsSet().size() == 1) {
                        virReg.replaceWith(zeroReg);
                    } else {
                        ASMInstruction move = instrFactory.createMove(zeroReg, null, ins.getRegister());
                        move.insertAfter(ins);
                    }
                    ins.removeFromList();
                }

                ins = nxt;
            }
        }
    }

    private static void removeUselessJump(ASMFunction function, boolean runRV) {
        basicBlockRearrange(function);

        if (runRV) {
            for (CustomList.Node node : function.getBasicBlockList()) {
                ASMBasicBlock basicBlock = (ASMBasicBlock) node;
                ASMInstruction lastIns = (ASMInstruction) basicBlock.getInstructionList().getTail();
                if (lastIns instanceof RVJ j && lastIns.getPrev() instanceof RVBranch branch) {
                    if (branch.getTarget() == basicBlock.getNext()) {
                        RVBranch invertedBr = branch.createInvertedBranch(j.getTarget(), null);
                        RVJ newJ = new RVJ(branch.getTarget(), null);
                        invertedBr.insertBefore(branch);
                        newJ.insertBefore(branch);
                        branch.removeFromList();
                        j.removeFromList();
                    }
                }
            }
        }

        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            if (basicBlock.getInstructionList().getTail() instanceof JumpIns asmJ) {
                if (asmJ.getTarget() == basicBlock.getNext()) {
                    ((ASMInstruction) asmJ).removeFromList();
                }
            }
        }

        deleteEmptyBlock(function);
    }

    /**
     * 删除空白的块，但是因为每个块至少有一个跳转指令，需要放在删完没用的跳转之后
     * 删掉空白块之前要确保没有哪个块要跳转到这里（因为这个优化没什么运行上的实质作用，做的保守一些）
     */
    private static void deleteEmptyBlock(ASMFunction function) {
        Set<ASMBasicBlock> tarSet = new LinkedHashSet<>();

        for (CustomList.Node blockNode : function.getBasicBlockList()) {
            ASMBasicBlock block = (ASMBasicBlock) blockNode;
            ASMInstruction ins = (ASMInstruction) block.getInstructionList().getTail();
            while (ins instanceof JumpIns || ins instanceof BranchIns) {
                if (ins instanceof JumpIns jumpIns) {
                    tarSet.add(jumpIns.getTarget());
                } else {
                    BranchIns branchIns = (BranchIns) ins;
                    tarSet.add(branchIns.getTarget());
                }
                ins = (ASMInstruction) ins.getPrev();
            }
        }

        for (CustomList.Node blockNode : function.getBasicBlockList()) {
            ASMBasicBlock block = (ASMBasicBlock) blockNode;
            if (block.getInstructionList().isEmpty() && !tarSet.contains(block)) {
                block.removeFromList();
            }
        }
    }

    private static void basicBlockRearrange(ASMFunction function) {
        Set<ASMBasicBlock> visited = new LinkedHashSet<>();
        ASMBasicBlock basicBlock = (ASMBasicBlock) function.getBasicBlockList().getHead();
        while (basicBlock != null) {
            if (visited.contains(basicBlock)) {
                basicBlock = (ASMBasicBlock) basicBlock.getNext();
                continue;
            }
            visited.add(basicBlock);

            if (basicBlock.getInstructionList().getTail() instanceof JumpIns asmJ) {
                ASMBasicBlock tar = asmJ.getTarget();
                if (tar == basicBlock || tar == basicBlock.getNext()) {
                    basicBlock = (ASMBasicBlock) basicBlock.getNext();
                    continue;
                }
                ASMBasicBlock tarPrev = ((ASMBasicBlock) tar.getPrev());
                if (tarPrev.getInstructionList().getTail() instanceof JumpIns j) {
                    if (j.getTarget() != tar || tarPrev.getLoopDepth() < basicBlock.getLoopDepth()) {
                        tar.removeFromList();
                        tar.insertAfter(basicBlock);
                    }
                }
            }
            basicBlock = (ASMBasicBlock) basicBlock.getNext();
        }
    }

    private static void removeUselessSaveOfRa(ASMFunction function, PhyReg retAddrReg) {
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            while (ins != null) {
                // 有函数调用指令说明保存返回地址是有必要的
                if (ins instanceof CallIns) {
                    return;
                }
                ins = (ASMInstruction) ins.getNext();
            }
        }
        // 走到这里说明这个函数没调用别人
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            while (ins != null) {
                if (ins instanceof StoreIns && ins.getOperands().contains(retAddrReg) ||
                        ins instanceof LoadIns && ins.getRegister() == retAddrReg) {
                    ins.removeFromList();
                }
                ins = (ASMInstruction) ins.getNext();
            }
        }
    }

    private static void removeUselessLoad(ASMFunction function, InstrFactory instrFactory) {
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            while (ins != null) {
                if (ins instanceof LoadIns ldIns && getPreStoreInsInBlk(ins, ldIns.getBase()) instanceof StoreIns stIns) {
                    String regOfLd = ins.getRegister().toString();
                    String regOfSt = stIns.getValue().toString();
                    String posOfLd = ldIns.getBase().toString() + "_" + ldIns.getOffset().toString();
                    String posOfSt = stIns.getBase().toString() + "_" + stIns.getOffset().toString();
                    if (posOfSt.equals(posOfLd)) {
                        // 寄存器不同，换成 move
                        if (!regOfSt.equals(regOfLd)) {
                            ASMInstruction moveIns = instrFactory.createMove(
                                    (Reg) stIns.getValue(), null, ins.getRegister()
                            );
                            moveIns.insertAfter(ins);
                        }
                        // 寄存器相同直接删掉
                        ins.removeFromList();
                    }
                }
                ins = (ASMInstruction) ins.getNext();
            }
        }
    }

    private static ASMInstruction getPreStoreInsInBlk(ASMInstruction ins, ASMValue base) {
        ASMInstruction pre = (ASMInstruction) ins.getPrev();
        while (pre != null && !(pre instanceof CallIns)) {
            if (pre.toString().contains(base.printAsOperand()) || pre.toString().contains(ins.printAsOperand())) {
                if (pre instanceof StoreIns) {
                    return pre;
                } else {
                    return null;
                }
            }
            pre = (ASMInstruction) pre.getPrev();
        }
        return null;
    }

    private static void removeUselessMove(ASMFunction function) {
        for (CustomList.Node node : function.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) node;
            ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
            while (ins != null) {
                if (ins instanceof MoveIns moveIns) {
                    if (ins.printAsOperand().equals(moveIns.getSrc().printAsOperand())) {
                        ins.removeFromList();
                    }
                }
                ins = (ASMInstruction) ins.getNext();
            }
        }
    }

    private static void deleteUnusedInstr(ASMFunction function) {
        for (CustomList.Node blockNode : function.getBasicBlockList()) {
            ASMBasicBlock block = (ASMBasicBlock) blockNode;
            for (CustomList.Node instrNode : block.getInstructionList()) {
                ASMInstruction instr = (ASMInstruction) instrNode;
                if (instr.getRegister() instanceof VirReg virReg && virReg.getUserSet().isEmpty()) {
                    instr.removeFromList();
                }
            }
        }
    }

}
