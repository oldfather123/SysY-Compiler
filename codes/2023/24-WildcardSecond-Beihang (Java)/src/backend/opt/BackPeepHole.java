package backend.opt;

import backend.component.RiscvBlock;
import backend.component.RiscvFunction;
import backend.component.RiscvInstr;
import backend.component.RiscvModule;
import backend.instruction.RiscvBinary;
import backend.instruction.RiscvBranch;
import backend.instruction.RiscvCall;
import backend.instruction.RiscvFld;
import backend.instruction.RiscvFlw;
import backend.instruction.RiscvFmv;
import backend.instruction.RiscvFsd;
import backend.instruction.RiscvFsw;
import backend.instruction.RiscvJ;
import backend.instruction.RiscvJr;
import backend.instruction.RiscvLd;
import backend.instruction.RiscvLi;
import backend.instruction.RiscvLw;
import backend.instruction.RiscvMv;
import backend.instruction.RiscvSd;
import backend.instruction.RiscvSext;
import backend.instruction.RiscvSw;
import backend.operand.RiscvImm;
import backend.operand.RiscvOperand;
import backend.operand.RiscvReg;
import backend.operand.*;
import ir.value.ConstNumber;
import tools.DoubleNode;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

public class BackPeepHole {
    private final RiscvModule riscvModule;
    private final LinkedHashSet<RiscvReg> influencedReg = new LinkedHashSet<>();
    private LinkedHashMap<RiscvBlock, LiveInfo> infoLinkedHashMap;
    private RiscvFunction function;

    public BackPeepHole(RiscvModule riscvModule) {
        this.riscvModule = riscvModule;
    }

    public void process() {
        boolean finished = false;
        while (!finished) {
            finished = peephole();
        }
        finalPeepHole(); // 这个窥孔必须是在最后进行
    }

    public void DataFlowProcess() { // 伪
        loopPeephole();
        deleteUselessDef();
    }

    private boolean peephole() {
        // 任何优化都不会发生，那么才叫做 finished
        boolean finished = true;
        for (RiscvFunction riscvFunction : riscvModule.getFunctions().values()) {
            // 正序遍历blocks
            function = riscvFunction;
            infoLinkedHashMap = LiveInfo.liveInfoAnalysis(function);
            influencedReg.clear();
            for (RiscvBlock block : riscvFunction.getBlocks()) {
                for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                     insNode != null; insNode = insNode.getSucc()) {
                    finished &= moveSameRegOptimize(insNode);
                    finished &= addiOptimize(insNode);
                    finished &= storeLoadOptimize(insNode);
                    finished &= removeUseLessStackFixer(insNode);
                    finished &= mergeAddiSP(insNode, block);
                }
            }

            // 倒序遍历blocks
            for (int i = riscvFunction.getBlocks().size() - 1; i >= 0; i--) {
                RiscvBlock block = riscvFunction.getBlocks().get(i);
                for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                     insNode != null; insNode = insNode.getSucc()) {
                    finished &= moveUselessOptimize(insNode); // 必须和 moveSameRegOptimize 分开
                }
                finished &= jumpOptimize(riscvFunction, block);
            }
        }
        return finished;
    }

    private boolean removeUseLessStackFixer(DoubleNode<RiscvInstr> insNode) {
        boolean finished = true;
        if (insNode.getContent() instanceof RiscvLi && insNode.getSucc() != null
                && insNode.getContent().getOperands().get(0) instanceof StackFixer) {
            DoubleNode<RiscvInstr> afterNode = insNode.getSucc();
            int load = Integer.parseInt(insNode.getContent().getOperands().get(0).toString());
            if (afterNode.getContent() instanceof RiscvBinary) {
                if (afterNode.getContent().getDefReg() == RiscvCPUReg.getRiscvCPUReg(2)) {
                    if (load >= 2048 || load <= -2048) {
                        return true;
                    }
                    assert ((RiscvBinary) afterNode.getContent()).getInstType() == RiscvBinary.RiscvBinaryType.sub;
                    DoubleNode<RiscvInstr> node = afterNode.getSucc();
                    assert afterNode.getSucc().getContent() instanceof RiscvCall;
                    while (node != null && !(node.getContent() instanceof RiscvBinary
                            && node.getContent().getDefReg() == RiscvCPUReg.getRiscvCPUReg(2))) { //find add
                        node = node.getSucc();
                    }
                    assert node != null && node.getContent() instanceof RiscvBinary
                            && node.getContent().getDefReg() == RiscvCPUReg.getRiscvCPUReg(2)
                            && ((RiscvBinary) node.getContent()).getInstType() == RiscvBinary.RiscvBinaryType.add;
                    DoubleNode<RiscvInstr> newNode1 = new DoubleNode<>(
                            new RiscvBinary(new ArrayList<>(Arrays.asList(
                                    RiscvCPUReg.getRiscvCPUReg(2), new RiscvImm(-1 * load))),
                                    RiscvCPUReg.getRiscvCPUReg(2), RiscvBinary.RiscvBinaryType.addi));
                    DoubleNode<RiscvInstr> newNode2 = new DoubleNode<>(
                            new RiscvBinary(new ArrayList<>(Arrays.asList(
                                    RiscvCPUReg.getRiscvCPUReg(2), new RiscvImm(load))),
                                    RiscvCPUReg.getRiscvCPUReg(2), RiscvBinary.RiscvBinaryType.addi));
                    insNode.getParentList().insertBeforeNode(insNode, newNode1);
                    addInfluencedReg(newNode1.getContent());
                    newNode1.getParentList().removeNode(insNode);
                    addInfluencedReg(insNode.getContent());
                    newNode1.getParentList().removeNode(afterNode);
                    addInfluencedReg(afterNode.getContent());
                    insNode.setSucc(afterNode.getSucc());
                    insNode.getParentList().insertBeforeNode(node, newNode2);
                    addInfluencedReg(newNode2.getContent());
                    newNode1.getParentList().removeNode(node);
                    addInfluencedReg(node.getContent());
                    return false;
                }
                switch (((RiscvBinary) afterNode.getContent()).getInstType()) {
                    case add -> {
                        if (afterNode.getContent().getOperands().get(0) == (insNode.getContent()).getDefReg()
                                && load < 2048 && load >= -2048) {
                            finished = false;
                            DoubleNode<RiscvInstr> newNode = new DoubleNode<>(
                                    new RiscvBinary(new ArrayList<>(Arrays.asList(
                                            afterNode.getContent().getOperands().get(1), new RiscvImm(load))),
                                            (afterNode.getContent()).getDefReg(), RiscvBinary.RiscvBinaryType.addi));
                            insNode.getParentList().insertBeforeNode(insNode, newNode);
                            addInfluencedReg(newNode.getContent());
                            newNode.getParentList().removeNode(insNode);
                            addInfluencedReg(insNode.getContent());
                            newNode.getParentList().removeNode(afterNode);
                            addInfluencedReg(afterNode.getContent());
                            insNode.setSucc(afterNode.getSucc());
                        } else if (afterNode.getContent().getOperands().get(1) == (insNode.getContent()).getDefReg()
                                && load < 2048 && load >= -2048) {
                            finished = false;
                            DoubleNode<RiscvInstr> newNode = new DoubleNode<>(
                                    new RiscvBinary(new ArrayList<>(Arrays.asList(
                                            afterNode.getContent().getOperands().get(0), new RiscvImm(load))),
                                            (afterNode.getContent()).getDefReg(), RiscvBinary.RiscvBinaryType.addi));
                            insNode.getParentList().insertBeforeNode(insNode, newNode);
                            addInfluencedReg(newNode.getContent());
                            newNode.getParentList().removeNode(insNode);
                            addInfluencedReg(insNode.getContent());
                            newNode.getParentList().removeNode(afterNode);
                            addInfluencedReg(afterNode.getContent());
                            insNode.setSucc(afterNode.getSucc());
                        }
                    }
                    case sub -> {
                        if (afterNode.getContent().getOperands().get(1) == (insNode.getContent()).getDefReg()
                                && load <= 2048 && load > -2048) {
                            finished = false;
                            DoubleNode<RiscvInstr> newNode = new DoubleNode<>(
                                    new RiscvBinary(new ArrayList<>(Arrays.asList(
                                            afterNode.getContent().getOperands().get(0), new RiscvImm(-1 * load))),
                                            (afterNode.getContent()).getDefReg(), RiscvBinary.RiscvBinaryType.addi));
                            insNode.getParentList().insertBeforeNode(insNode, newNode);
                            addInfluencedReg(newNode.getContent());
                            newNode.getParentList().removeNode(insNode);
                            addInfluencedReg(insNode.getContent());
                            newNode.getParentList().removeNode(afterNode);
                            addInfluencedReg(afterNode.getContent());
                            insNode.setSucc(afterNode.getSucc());
                        }
                    }
                    case subw -> {
                        if (afterNode.getContent().getOperands().get(1) == (insNode.getContent()).getDefReg()
                                && load <= 2048 && load > -2048) {
                            finished = false;
                            DoubleNode<RiscvInstr> newNode = new DoubleNode<>(
                                    new RiscvBinary(new ArrayList<>(Arrays.asList(
                                            afterNode.getContent().getOperands().get(0), new RiscvImm(-1 * load))),
                                            (afterNode.getContent()).getDefReg(), RiscvBinary.RiscvBinaryType.addiw));
                            insNode.getParentList().insertBeforeNode(insNode, newNode);
                            addInfluencedReg(newNode.getContent());
                            newNode.getParentList().removeNode(insNode);
                            addInfluencedReg(insNode.getContent());
                            newNode.getParentList().removeNode(afterNode);
                            addInfluencedReg(afterNode.getContent());
                            insNode.setSucc(afterNode.getSucc());
                        }
                    }
                    case addw -> {
                        if (afterNode.getContent().getOperands().get(0) == (insNode.getContent()).getDefReg()
                                && load < 2048 && load >= -2048) {
                            finished = false;
                            DoubleNode<RiscvInstr> newNode = new DoubleNode<>(
                                    new RiscvBinary(new ArrayList<>(Arrays.asList(
                                            afterNode.getContent().getOperands().get(1), new RiscvImm(load))),
                                            (afterNode.getContent()).getDefReg(), RiscvBinary.RiscvBinaryType.addiw));
                            insNode.getParentList().insertBeforeNode(insNode, newNode);
                            addInfluencedReg(newNode.getContent());
                            newNode.getParentList().removeNode(insNode);
                            addInfluencedReg(insNode.getContent());
                            newNode.getParentList().removeNode(afterNode);
                            addInfluencedReg(afterNode.getContent());
                            insNode.setSucc(afterNode.getSucc());
                        } else if (afterNode.getContent().getOperands().get(1) == (insNode.getContent()).getDefReg()
                                && load < 2048 && load >= -2048) {
                            finished = false;
                            DoubleNode<RiscvInstr> newNode = new DoubleNode<>(
                                    new RiscvBinary(new ArrayList<>(Arrays.asList(
                                            afterNode.getContent().getOperands().get(0), new RiscvImm(load))),
                                            (afterNode.getContent()).getDefReg(), RiscvBinary.RiscvBinaryType.addiw));
                            insNode.getParentList().insertBeforeNode(insNode, newNode);
                            addInfluencedReg(newNode.getContent());
                            newNode.getParentList().removeNode(insNode);
                            addInfluencedReg(insNode.getContent());
                            newNode.getParentList().removeNode(afterNode);
                            addInfluencedReg(afterNode.getContent());
                            insNode.setSucc(afterNode.getSucc());
                        }
                    }
                }
            }
        }
        return finished;
    }

    private boolean moveSameRegOptimize(DoubleNode<RiscvInstr> insNode) {
        boolean finished = true;
        if (insNode.getContent() instanceof RiscvMv) {
            // 将 mv rs, rs 删掉
            if (insNode.getContent().getDefReg() ==
                    (insNode.getContent().getOperands().get(0))) {
                insNode.getParentList().removeNode(insNode);
                addInfluencedReg(insNode.getContent());
                finished = false;
            }
        }
        return finished;
    }

    private boolean mergeAddiSP(DoubleNode<RiscvInstr> insNode, RiscvBlock block) {
        boolean finished = true;
        DoubleNode<RiscvInstr> afterNode = insNode.getSucc();
        DoubleNode<RiscvInstr> newNode = null;
        if (insNode.getContent() instanceof RiscvBinary &&
                (((RiscvBinary) insNode.getContent()).getInstType() == RiscvBinary.RiscvBinaryType.addi
                        || ((RiscvBinary) insNode.getContent()).getInstType() == RiscvBinary.RiscvBinaryType.addiw)
                && insNode.getContent().getDefReg() != RiscvCPUReg.getRiscvCPUReg(2)) {
            if (afterNode != null && afterNode.getContent() instanceof RiscvSw
                    && insNode.getContent().getDefReg() == afterNode.getContent().getOperands().get(2)
                    && insNode.getContent().getDefReg() != insNode.getContent().getOperands().get(0)
                    && canDeleteAddi(block, afterNode, insNode.getContent().getDefReg())) {
                finished = false;
                newNode = new DoubleNode<>(
                        new RiscvSw((RiscvReg) afterNode.getContent().getOperands().get(0),
                                (RiscvImm) insNode.getContent().getOperands().get(1),
                                (RiscvReg) insNode.getContent().getOperands().get(0)));
            } else if (afterNode != null && afterNode.getContent() instanceof RiscvSd
                    && insNode.getContent().getDefReg() == afterNode.getContent().getOperands().get(2)
                    && insNode.getContent().getDefReg() != insNode.getContent().getOperands().get(0)
                    && canDeleteAddi(block, afterNode, insNode.getContent().getDefReg())) {
                finished = false;
                newNode = new DoubleNode<>(
                        new RiscvSd((RiscvReg) afterNode.getContent().getOperands().get(0),
                                (RiscvImm) insNode.getContent().getOperands().get(1),
                                (RiscvReg) insNode.getContent().getOperands().get(0)));
            } else if (afterNode != null && afterNode.getContent() instanceof RiscvFsw
                    && insNode.getContent().getDefReg() == afterNode.getContent().getOperands().get(2)
                    && insNode.getContent().getDefReg() != insNode.getContent().getOperands().get(0)
                    && canDeleteAddi(block, afterNode, insNode.getContent().getDefReg())) {
                finished = false;
                newNode = new DoubleNode<>(
                        new RiscvFsw((RiscvReg) afterNode.getContent().getOperands().get(0),
                                (RiscvImm) insNode.getContent().getOperands().get(1),
                                (RiscvReg) insNode.getContent().getOperands().get(0)));
            } else if (afterNode != null && afterNode.getContent() instanceof RiscvFsd
                    && insNode.getContent().getDefReg() == afterNode.getContent().getOperands().get(2)
                    && insNode.getContent().getDefReg() != insNode.getContent().getOperands().get(0)
                    && canDeleteAddi(block, afterNode, insNode.getContent().getDefReg())) {
                finished = false;
                newNode = new DoubleNode<>(
                        new RiscvFsd((RiscvReg) afterNode.getContent().getOperands().get(0),
                                (RiscvImm) insNode.getContent().getOperands().get(1),
                                (RiscvReg) insNode.getContent().getOperands().get(0)));
            } else if (afterNode != null && afterNode.getContent() instanceof RiscvLw
                    && insNode.getContent().getDefReg() == afterNode.getContent().getOperands().get(1)
                    && insNode.getContent().getDefReg() != insNode.getContent().getOperands().get(0)
                    && canDeleteAddi(block, afterNode, insNode.getContent().getDefReg())) {
                finished = false;
                newNode = new DoubleNode<>(
                        new RiscvLw((RiscvImm) insNode.getContent().getOperands().get(1),
                                (RiscvReg) insNode.getContent().getOperands().get(0),
                                afterNode.getContent().getDefReg()));
            } else if (afterNode != null && afterNode.getContent() instanceof RiscvLd
                    && insNode.getContent().getDefReg() == afterNode.getContent().getOperands().get(1)
                    && insNode.getContent().getDefReg() != insNode.getContent().getOperands().get(0)
                    && canDeleteAddi(block, afterNode, insNode.getContent().getDefReg())) {
                finished = false;
                newNode = new DoubleNode<>(
                        new RiscvLd((RiscvImm) insNode.getContent().getOperands().get(1),
                                (RiscvReg) insNode.getContent().getOperands().get(0),
                                afterNode.getContent().getDefReg()));
            } else if (afterNode != null && afterNode.getContent() instanceof RiscvFlw
                    && insNode.getContent().getDefReg() == afterNode.getContent().getOperands().get(1)
                    && insNode.getContent().getDefReg() != insNode.getContent().getOperands().get(0)
                    && canDeleteAddi(block, afterNode, insNode.getContent().getDefReg())) {
                finished = false;
                newNode = new DoubleNode<>(
                        new RiscvFlw(afterNode.getContent().getDefReg(),
                                (RiscvImm) insNode.getContent().getOperands().get(1),
                                (RiscvReg) insNode.getContent().getOperands().get(0)));
            } else if (afterNode != null && afterNode.getContent() instanceof RiscvFld
                    && insNode.getContent().getDefReg() == afterNode.getContent().getOperands().get(1)
                    && insNode.getContent().getDefReg() != insNode.getContent().getOperands().get(0)
                    && canDeleteAddi(block, afterNode, insNode.getContent().getDefReg())) {
                finished = false;
                newNode = new DoubleNode<>(
                        new RiscvFld((afterNode.getContent().getDefReg()),
                                (RiscvImm) insNode.getContent().getOperands().get(1),
                                (RiscvReg) insNode.getContent().getOperands().get(0)));
            }
        }
        if(newNode != null) {
            insNode.getParentList().insertBeforeNode(insNode, newNode);
            addInfluencedReg(newNode.getContent());
            insNode.getParentList().removeNode(insNode);
            addInfluencedReg(insNode.getContent());
            insNode.getParentList().removeNode(afterNode);
            addInfluencedReg(afterNode.getContent());
            insNode.setSucc(afterNode.getSucc());
        }
        return finished;
    }

    private boolean moveUselessOptimize(DoubleNode<RiscvInstr> insNode) {
        boolean finished = true;
        if (insNode.getContent() instanceof RiscvMv) {
            // 将 mv rs, rs 删掉
            if (insNode.getContent().getDefReg() ==
                    (insNode.getContent().getOperands().get(0))) {
                insNode.getParentList().removeNode(insNode);
                finished = false;
            }

            // 连续两条指令 mv a1, a2 和 mv a1, a3 则删掉第一条指令
            DoubleNode<RiscvInstr> preNode = insNode.getPred();
            if (preNode != null && preNode.getContent() instanceof RiscvMv
                    && insNode.getContent().getDefReg().equals(preNode.getContent().getDefReg())) {
                insNode.getParentList().removeNode(preNode);
                finished = false;
            }
        }
        return finished;
    }

    private boolean addiOptimize(DoubleNode<RiscvInstr> insNode) {
        boolean finished = true;
        if (insNode.getContent() instanceof RiscvBinary) {
            RiscvBinary ins = (RiscvBinary) insNode.getContent();
            if (ins.getInstType().equals(RiscvBinary.RiscvBinaryType.addi)) {
                if (((RiscvImm) ins.getOperands().get(1)).getValue() == 0) {
                    // 将 addi rs, rs, 0 删掉
                    if (ins.getDefReg().equals(ins.getOperands().get(0))) {
                        insNode.getParentList().removeNode(insNode);
                        addInfluencedReg(insNode.getContent());
                        finished = false;
                    } else { // 将 addi rd, rs, 0 改成 mv rd, rs
                        DoubleNode<RiscvInstr> newNode = new DoubleNode<>
                                (new RiscvMv((RiscvReg) ins.getOperands().get(0),
                                        ins.getDefReg()));
                        insNode.getParentList().insertBeforeNode(insNode, newNode);
                        insNode.getParentList().removeNode(insNode);
                        addInfluencedReg(newNode.getContent());
                        addInfluencedReg(insNode.getContent());
                        finished = false;
                    }
                }
            } else if (ins.getInstType().equals(RiscvBinary.RiscvBinaryType.addiw)) {
                if (((RiscvImm) ins.getOperands().get(1)).getValue() == 0) {
                    // 将 addi rs, rs, 0 删掉
                    if (ins.getDefReg().equals(ins.getOperands().get(0))) {
                        insNode.getParentList().removeNode(insNode);
                        addInfluencedReg(insNode.getContent());
                        finished = false;
                    } else { // 将 addi rd, rs, 0 改成 mv rd, rs
                        DoubleNode<RiscvInstr> newNode = new DoubleNode<>
                                (new RiscvMv((RiscvReg) ins.getOperands().get(0),
                                        ins.getDefReg()));
                        insNode.getParentList().insertBeforeNode(insNode, newNode);
                        addInfluencedReg(newNode.getContent());
                        insNode.getParentList().removeNode(insNode);
                        addInfluencedReg(insNode.getContent());
                        finished = false;
                    }
                }
            }
        }
        return finished;
    }

    private boolean storeLoadOptimize(DoubleNode<RiscvInstr> insNode) {
        boolean finished = true;
        // 将 store a, memory; load b, sameMemory 改成 move b, a
        DoubleNode<RiscvInstr> preNode = insNode.getPred();
        if (preNode != null) {
            if ((insNode.getContent() instanceof RiscvLd
                    && preNode.getContent() instanceof RiscvSd
                    && insNode.getContent().getOperands().get(0)
                    .equals(preNode.getContent().getOperands().get(1))
                    && insNode.getContent().getOperands().get(1)
                    .equals(preNode.getContent().getOperands().get(2)))
                    || (insNode.getContent() instanceof RiscvLw
                    && preNode.getContent() instanceof RiscvSw
                    && insNode.getContent().getOperands().get(0)
                    .equals(preNode.getContent().getOperands().get(1))
                    && insNode.getContent().getOperands().get(1)
                    .equals(preNode.getContent().getOperands().get(2)))) {
                DoubleNode<RiscvInstr> newNode = new DoubleNode<>
                        (new RiscvMv((RiscvReg) preNode.getContent().getOperands().get(0),
                                insNode.getContent().getDefReg()));
                insNode.getParentList().insertBeforeNode(preNode, newNode);
                addInfluencedReg(newNode.getContent());
                insNode.getParentList().removeNode(preNode);
                addInfluencedReg(preNode.getContent());
                insNode.getParentList().removeNode(insNode);
                addInfluencedReg(insNode.getContent());
                finished = false;
            } else if ((insNode.getContent() instanceof RiscvFld
                    && preNode.getContent() instanceof RiscvFsd
                    && insNode.getContent().getOperands().get(0)
                    .equals(preNode.getContent().getOperands().get(1))
                    && insNode.getContent().getOperands().get(1)
                    .equals(preNode.getContent().getOperands().get(2))) ||
                    (insNode.getContent() instanceof RiscvFlw
                            && preNode.getContent() instanceof RiscvFsw
                            && insNode.getContent().getOperands().get(0)
                            .equals(preNode.getContent().getOperands().get(1))
                            && insNode.getContent().getOperands().get(1)
                            .equals(preNode.getContent().getOperands().get(2)))) {
                DoubleNode<RiscvInstr> newNode = new DoubleNode<>
                        (new RiscvFmv((RiscvReg) preNode.getContent().getOperands().get(0),
                                insNode.getContent().getDefReg()));
                insNode.getParentList().insertBeforeNode(preNode, newNode);
                addInfluencedReg(newNode.getContent());
                insNode.getParentList().removeNode(preNode);
                addInfluencedReg(preNode.getContent());
                insNode.getParentList().removeNode(insNode);
                addInfluencedReg(insNode.getContent());
                finished = false;
            }
        }
        return finished;
    }

    private boolean jumpOptimize(RiscvFunction riscvFunction, RiscvBlock block) {
        boolean finished = true;
        // 例如：在 blocklable1 的最后 j blocklable2， 那么合并两个block
        // 【前提：全局没有其他 j blocklable2 和 br blocklable2 了】
        DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getTail();
        if (insNode.getContent() instanceof RiscvJ) {
            RiscvBlock defBlock = (RiscvBlock) insNode.getContent().getOperands().get(0);
            if (riscvFunction.getBlocks().contains(defBlock)
                    && riscvFunction.getBlocks().indexOf(block) + 1
                    == riscvFunction.getBlocks().indexOf(defBlock)) {// defBlock 是紧接着 block 的
                boolean signal = true;
                for (RiscvBlock riscvBlock : riscvFunction.getBlocks()) {
                    if (!riscvBlock.equals(block)) {
                        if (riscvBlock.getRiscvInstrs().getTail()
                                .getContent() instanceof RiscvJ
                                && riscvBlock.getRiscvInstrs().getTail().getContent()
                                .getOperands().get(0).equals(defBlock)) {
                            signal = false;
                            break;
                        }
                        if (riscvBlock.getRiscvInstrs().getTail()
                                .getContent() instanceof RiscvBranch
                                &&
                                riscvBlock.getRiscvInstrs().getTail().getContent().getOperands()
                                        .get(2).equals(defBlock)) {
                            signal = false;
                            break;
                        }
                    }
                }
                if (signal) {
                    var thisInsts = insNode.getParentList();
                    for(RiscvBlock block0: defBlock.getSuccs()) {
                        block.addSuccs(block0);
                    }
                    block.getSuccs().remove(defBlock);
                    insNode.getParentList().removeNode(insNode);
                    thisInsts.addListBefore(defBlock.getRiscvInstrs());
                    riscvFunction.getBlocks().remove(defBlock);
                    finished = false;
                }
            }
        }
        return finished;
    }

    private void finalPeepHole() {
        for (RiscvFunction riscvFunction : riscvModule.getFunctions().values()) {
            ArrayList<RiscvBlock> blocks = riscvFunction.getBlocks();
            for (RiscvBlock block : riscvFunction.getBlocks()) {
                DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getTail();
                RiscvInstr instr = insNode.getContent();
                if (instr instanceof RiscvJ && block.getSuccs() != null
                        && blocks.indexOf(instr.getOperands().get(0)) == blocks.indexOf(block) + 1) {
                    insNode.getParentList().removeNode(insNode);
                }
            }
        }
    }

    private boolean dataFlowPeephole() {
        // 任何优化都不会发生，那么才叫做 finished
        boolean finished = true;
        for (RiscvFunction riscvFunction : riscvModule.getFunctions().values()) {
            LinkedHashMap<RiscvBlock, LiveInfo> liveInfoMap = LiveInfo.liveInfoAnalysis(riscvFunction);
            // lastWriter 可以根据寄存器查找上一个写者（最后一次写这个寄存器的指令）
            LinkedHashMap<RiscvOperand, RiscvInstr> lastWriter = new LinkedHashMap<>();
            // 将当前指令作为写指令，writerToReader 可以根据当前指令查询最后一次的读指令
            LinkedHashMap<RiscvInstr, RiscvInstr> writerToReader = new LinkedHashMap<>();
            for (RiscvBlock block : riscvFunction.getBlocks()) {
                getLiveRangeInBlock(block, lastWriter, writerToReader);
            }
            for (RiscvBlock block : riscvFunction.getBlocks()) {
                LinkedHashSet<RiscvReg> liveOut = liveInfoMap.get(block).getLiveOut();

                for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                     insNode != null; insNode = insNode.getSucc()) {
                    RiscvInstr instr = insNode.getContent();

                    if (instr.getDefReg() == null || instr.getDefReg() instanceof RiscvPhyReg) {
                        continue;
                    }
                    boolean isLastWriter = lastWriter.get(instr.getDefReg()).equals(instr); // 判断当前指令写的寄存器是不是最后一次被写
                    boolean isInLiveOut = liveOut.contains(instr.getDefReg());
                    boolean hasCondition = instr instanceof RiscvBranch; // 当前指令是否有条件
                    if (!isLastWriter || !isInLiveOut || !hasCondition) {
                        // 数据流窥孔的精髓:考虑删除的指令必须被限定在基本块内，然后再被限定在窥孔内
                        RiscvInstr lastReader = writerToReader.get(instr);
                        boolean changeSp = instr.getDefReg().equals(RiscvCPUReg.getRiscvCPUReg(2));
                        finished &= deletUselessLastWriter(insNode, lastReader, changeSp);
                    }
                }
            }
        }
        return finished;
    }

    private void getLiveRangeInBlock(RiscvBlock block,
                                     LinkedHashMap<RiscvOperand, RiscvInstr> lastWriter,
                                     LinkedHashMap<RiscvInstr, RiscvInstr> writerToReader) {
        // lastWriter 记录的是每个操作数与它最后一次被写的指令之间的映射

        // writerToReader 记录的是 key = writer，value = reader 的映射
        // key -> value 记录的是一个写了某个寄存器的指令到其后读了某个寄存器的指令之间的映射
        // 因为 writerToReader 会持续更新，所以本质是记录着写指令到最后一条相关的读指令之间的映射
        for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
             insNode != null; insNode = insNode.getSucc()) {
            RiscvInstr instr = insNode.getContent();
            for (RiscvOperand riscvOperand : instr.getOperands()) {
                if (!(riscvOperand instanceof RiscvImm) && lastWriter.containsKey(riscvOperand)) {
                    writerToReader.put(lastWriter.get(riscvOperand), instr);
                }
            }

            lastWriter.put(instr.getDefReg(), instr);
            if (instr instanceof RiscvBranch || instr instanceof RiscvCall
                    || instr instanceof RiscvSd || instr instanceof RiscvSw
                    || instr instanceof RiscvFsw || instr instanceof RiscvFsd
                    || instr instanceof RiscvJ || instr instanceof RiscvJr
                    || instr instanceof RiscvSext) {
                writerToReader.put(instr, instr);
            } else {
                writerToReader.put(instr, null);
            }
        }
    }

    private boolean deletUselessLastWriter(DoubleNode<RiscvInstr> insNode, RiscvInstr lastReader, boolean changeSp) {
        // 删除没有用的写指令
        // some instruction contains a
        // live out doesn't contain a
        // 当一个指令写的寄存器没有被读，那么就是没用的
        // 除此之外还要看是否会改变 sp 指针，这种深远影响是不行的
        boolean finished = true;
        if (lastReader == null && !changeSp) {
            insNode.getParentList().removeNode(insNode);
            finished = false;
        }
        return finished;
    }

    public void addInfluencedReg(RiscvInstr ins) {
        if(ins.getDefReg() != null) {
            influencedReg.add(ins.getDefReg());
        }
        for(RiscvOperand op:ins.getOperands()) {
            if(op instanceof RiscvPhyReg) {
                influencedReg.add((RiscvReg) op);
            }
        }
    }
    private void analyze() {
        infoLinkedHashMap = LiveInfo.liveInfoAnalysis(function);
        influencedReg.clear();
    }

    //使用数据流分析以判断该addi是否可以删除
    //利用局部性原理剪枝
    private boolean canDeleteAddi(RiscvBlock block, DoubleNode<RiscvInstr> insNode, RiscvReg reg) {
        //优先对当前块进行遍历
        if(insNode.getContent() instanceof RiscvLw || insNode.getContent() instanceof RiscvLd
            || insNode.getContent() instanceof RiscvFlw || insNode.getContent() instanceof RiscvFld) {
            if(insNode.getContent().getDefReg() == reg) {
                return true;
            }
        }
        DoubleNode<RiscvInstr> afterNode = insNode.getSucc();
        while(afterNode != null) {
            for(var op:afterNode.getContent().getOperands()) {
                if(op == reg) {
                    return false;
                }
            }
            if(getDefReg(afterNode.getContent()).contains(reg)) {
                return true;
            }
            afterNode = afterNode.getSucc();
        }
        if(!influencedReg.contains(reg)) {
            return !infoLinkedHashMap.get(block).getLiveOut().contains(reg);
        }
        //对一级子孙进行遍历
        for(RiscvBlock block1:block.getSuccs()) {
            afterNode = block1.getRiscvInstrs().getHead();
            while(afterNode != null) {
                for(var op:afterNode.getContent().getOperands()) {
                    if(op == reg) {
                        return false;
                    }
                }
                if(getDefReg(afterNode.getContent()).contains(reg)) {
                    break;
                }
                afterNode = afterNode.getSucc();
            }
        }
        analyze();
        return !infoLinkedHashMap.get(block).getLiveOut().contains(reg);
        //迫不得已 重新调用活跃性分析
    }

    private LinkedHashSet<RiscvPhyReg> getDefReg(RiscvInstr ins) {
        if(!(ins instanceof RiscvCall)) {
            LinkedHashSet<RiscvPhyReg> ret = new LinkedHashSet<>();
            ret.add((RiscvPhyReg) ins.getDefReg());
            return ret;
        } else {
            LinkedHashSet<RiscvPhyReg> ret = new LinkedHashSet<>();
            ArrayList<Integer> list =
                    new ArrayList<>(
                            Arrays.asList(1, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31));
            list.forEach(i -> ret.add(RiscvCPUReg.getRiscvCPUReg(i)));
            ArrayList<Integer> list1 =
                    new ArrayList<>(
                            Arrays.asList(0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29,
                                    30, 31));
            list1.forEach(i -> ret.add(RiscvFPUReg.getRiscvFPUReg(i)));
            return ret;
        }
    }

    private void loopPeephole() {
        for (RiscvFunction riscvFunction : riscvModule.getFunctions().values()) {
            // 正序遍历blocks
            function = riscvFunction;
            infoLinkedHashMap = LiveInfo.liveInfoAnalysis(function);
            influencedReg.clear();
            for (RiscvBlock block : riscvFunction.getBlocks()) {
                // 消除多余li
                for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                     insNode != null; insNode = insNode.getSucc()) {
                    RiscvInstr instr = insNode.getContent();
                    if (instr instanceof RiscvLi && instr.getDefReg() instanceof RiscvVirReg) {
                        RiscvLi li = (RiscvLi) instr;
                        RiscvImm imm = (RiscvImm) li.getOperands().get(0);
                        if (insNode.getSucc() != null) {
                            for (DoubleNode<RiscvInstr> insNode1 = insNode.getSucc();
                                 insNode1 != null; insNode1 = insNode1.getSucc()) {
                                instr = insNode1.getContent();
                                if (instr instanceof RiscvLi && instr.getDefReg() instanceof RiscvVirReg
                                        && ((RiscvImm) instr.getOperands().get(0)).getValue() == imm.getValue()) {
                                    int sumDef = 0;
                                    for (DoubleNode<RiscvInstr> useNode : instr.getDefReg().getUser()) {
                                        if (instr.getDefReg().equals(useNode.getContent().getDefReg())) {
                                            sumDef++;
                                        }
                                    }
                                    if (sumDef > 1) {
                                        continue;
                                    }
                                    instr.getDefReg().replaceAllUser(insNode1, insNode);
                                    insNode1.getParentList().removeNode(insNode1);
                                }
                            }
                        }
                    }
                }
                // 消除重复的二元运算指令
                for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                     insNode != null; insNode = insNode.getSucc()) {
                    RiscvInstr instr = insNode.getContent();
                    if (instr instanceof RiscvBinary && instr.getDefReg() instanceof RiscvVirReg) {
                        RiscvBinary binary = (RiscvBinary) instr;
                        if (insNode.getSucc() != null) {
                            for (DoubleNode<RiscvInstr> insNode1 = insNode.getSucc();
                                 insNode1 != null; insNode1 = insNode1.getSucc()) {
                                instr = insNode1.getContent();
                                if (instr instanceof RiscvBinary
                                        && instr.getDefReg() instanceof RiscvVirReg
                                        && (((RiscvBinary) instr).getInstType()).equals(binary.getInstType())) {
                                    if ((instr.getOperands().get(0).equals(binary.getOperands().get(0))
                                            && instr.getOperands().get(1).equals(binary.getOperands().get(1)))
                                            || (instr.getOperands().get(0).equals(binary.getOperands().get(1))
                                            && instr.getOperands().get(1).equals(binary.getOperands().get(0)))) {
                                        int sumDef = 0;
                                        for (DoubleNode<RiscvInstr> useNode : instr.getDefReg().getUser()) {
                                            if (instr.getDefReg().equals(useNode.getContent().getDefReg())) {
                                                sumDef++;
                                            }
                                        }
                                        if (sumDef > 1) {
                                            continue;
                                        }
                                        instr.getDefReg().replaceAllUser(insNode1, insNode);
                                        insNode1.getParentList().removeNode(insNode1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return;
    }

    private void deleteUselessDef() {
        for (RiscvFunction riscvFunction : riscvModule.getFunctions().values()) {
            for (RiscvBlock block : riscvFunction.getBlocks()) {
                for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                     insNode != null; insNode = insNode.getSucc()) {
                    RiscvInstr instr = insNode.getContent();
                    if (instr.getDefReg() != null && instr.getDefReg() instanceof RiscvVirReg
                            && instr.getDefReg().getUser().size() == 0) {
                        insNode.getParentList().removeNode(insNode);
                    }
                }
            }
        }
    }
}
