package backend.opt;

import backend.component.RiscvBlock;
import backend.component.RiscvFunction;
import backend.component.RiscvInstr;
import backend.component.RiscvModule;
import backend.instruction.*;
import backend.operand.RiscvImm;
import backend.operand.RiscvOperand;
import backend.operand.RiscvReg;
import backend.operand.RiscvVirReg;
import ir.pass.analyze.Loop;
import ir.value.BasicBlock;
import ir.value.user.Function;
import ir.value.Module;
import org.antlr.v4.runtime.misc.Pair;
import tools.DoubleNode;

import java.util.*;

public class BackLoopLiftOut {
    private final RiscvModule riscvModule;
    private final Module llvmModule;
    private final LinkedHashSet<RiscvInstr> insts = new LinkedHashSet<>();
    private final ArrayList<Pair<DoubleNode<RiscvInstr>, RiscvBlock>> queue = new ArrayList<>();
    private Loop curLoop;
    private RiscvBlock preHeadblock;
    private RiscvFunction curFunction;
    private LinkedHashMap<Loop, ArrayList<RiscvBlock>> riscvLoop = new LinkedHashMap<>();
    private LinkedHashMap<RiscvReg, ArrayList<DoubleNode<RiscvInstr>>> reg2InstsDef = new LinkedHashMap<>();
    private LinkedHashMap<RiscvReg, ArrayList<RiscvBlock>> reg2BlocksUse = new LinkedHashMap<>();
    private LinkedHashSet<RiscvBlock> outer = new LinkedHashSet<>();
    private LinkedHashMap<Pair<RiscvBlock, RiscvBlock>, Boolean> haveBeenDfs = new LinkedHashMap<>();
    private LinkedHashSet<Loop> visitedLoop = new LinkedHashSet<>();
    private LinkedHashSet<RiscvInstr> beenLiftOnce = new LinkedHashSet<>();
    private int hasLift = 0;
    private ArrayList<DoubleNode<RiscvInstr>> insts2Ins = new ArrayList<>();

    public BackLoopLiftOut(RiscvModule module, Module llvmModule) {
        this.riscvModule = module;
        this.llvmModule = llvmModule;
    }

    public void process() {
        for (Function function : llvmModule.functions.values()) {
            beenLiftOnce.clear();
            curFunction = riscvModule.getFunction(function.getName());
            riscvLoop.clear();
            visitedLoop.clear();
            for (Loop loop : function.loops) {
                buildRiscvLoop(loop);
            }
            for (Loop loop : function.loops) {
                if (visitedLoop.contains(loop)) {
                    continue;
                }
                Loop analyzeLoop = loop;
                if (analyzeLoop.father != null) {
                    analyzeLoop = analyzeLoop.father;
                }
                lift(analyzeLoop);
            }
            rebuildGraph();
        }
    }

    private void buildRiscvLoop(Loop loop) {
        if(riscvLoop.containsKey(loop)) {
            return ;
        }
        ArrayList<RiscvBlock> blocks = new ArrayList<>();
        for (BasicBlock block : loop.blocks) {
            if(block == null) {
                continue;
            }
            blocks.add((RiscvBlock) riscvModule.getValue2Label().get(block));
        }
        riscvLoop.put(loop, blocks);
        for(Loop childloop:loop.children) {
            if(childloop == null) {
                continue;
            }
            if(!riscvLoop.containsKey(childloop)) {
                buildRiscvLoop(childloop);
            }
            blocks.addAll(riscvLoop.get(childloop));
        }
    }

    public void lift(Loop loop) {
        if (visitedLoop.contains(loop)) {
            return;
        }
        for (Loop childLoop : loop.children) {
            lift(childLoop);
        }
        curLoop = loop;
        hasLift = 0;
        preHeadblock = null;
        haveBeenDfs.clear();
        visitedLoop.add(loop);
        queue.clear();
        getOuter();
        analyzeLoop(loop);
        RiscvBlock entrance =
                (RiscvBlock) riscvModule.getValue2Label().get(curLoop.blocks.get(0));
        insts2Ins.clear();
        markInst(entrance);
        moveInvar(entrance);
        finalInsert();
        if (preHeadblock != null) {
            preHeadInsert(entrance);
            brModify(entrance);
            preHeadblock.addInstr(
                    new DoubleNode<>(new RiscvJ(entrance, preHeadblock)), curLoop.level - 1);
        }
    }

    private void insertPreHead(DoubleNode<RiscvInstr> ins) {
        insts2Ins.add(ins);
        hasLift++;
    }

    private void finalInsert() {
        while(insts2Ins.size() != 0) {
            DoubleNode<RiscvInstr> node = insts2Ins.remove(0);
            boolean flag = true;
            if(node.getContent() instanceof RiscvLi) {
                for(DoubleNode<RiscvInstr> ins: node.getContent().getDefReg().getUser()) {
                    if(!ins.equals(node) && !insts2Ins.contains(ins)) {
                        flag = false;
                        break;
                    }
                }
            }
            if(flag) {
                if (preHeadblock == null) {
                    preHeadblock = new RiscvBlock("LoopLift" + curFunction.getIrRegDispatcher().allocId(),
                            curLoop.father);
                }
                beenLiftOnce.add(node.getContent());
                node.getParentList().removeNode(node);
                preHeadblock.addInstr(node, curLoop.level - 1);
            } else {
                for(DoubleNode<RiscvInstr> ins: node.getContent().getDefReg().getUser()) {
                    if(insts2Ins.contains(ins)) {
                        insts2Ins.remove(ins);
                    }
                }
            }
        }
    }

    private void preHeadInsert(RiscvBlock entrance) {
        for (int i = 0; i < curFunction.getBlocks().size(); i++) {
            if (curFunction.getBlocks().get(i).equals(entrance)) {
                curFunction.getBlocks().add(i, preHeadblock);
                break;
            }
        }
        Loop son = curLoop;
        while (son.father != null) {
            ArrayList<RiscvBlock> fatherBlocks = riscvLoop.get(son.father);
            for (int i = 0; i < fatherBlocks.size(); i++) {
                if (fatherBlocks.get(i) == entrance) {
                    fatherBlocks.add(i, preHeadblock);
                    break;
                }
            }
            son = son.father;
        }
    }

    private void brModify(RiscvBlock entrance) {
        //循环内Br依旧跳往riscvModule.getValue2Label().get(curLoop.blocks.get(0))
        //其他跳往preHeadBlock
        for (RiscvBlock riscvBlock : curFunction.getBlocks()) {
            if (!riscvLoop.get(curLoop).contains(riscvBlock)) {
                DoubleNode<RiscvInstr> ins = riscvBlock.getRiscvInstrs().getTail();
                while (ins != null && (ins.getContent() instanceof RiscvBranch
                        || ins.getContent() instanceof RiscvJ)) {
                    if (ins.getContent() instanceof RiscvBranch) {
                        if (ins.getContent().getOperands().get(2) == entrance) {
                            ins.getContent().getOperands().remove(2);
                            ins.getContent().getOperands().add(2, preHeadblock);
                            preHeadblock.getUser().add(ins);
                            entrance.getUser().remove(ins);
                        }
                    } else {
                        if (ins.getContent().getOperands().get(0) == entrance) {
                            ins.getContent().getOperands().remove(0);
                            ins.getContent().getOperands().add(0, preHeadblock);
                            preHeadblock.getUser().add(ins);
                            entrance.getUser().remove(ins);
                        }
                    }
                    ins = ins.getPred();
                }
            }
        }
        rebuildGraph();
    }

    private void rebuildGraph() {
        for (int i = 0; i < curFunction.getBlocks().size(); i++) {
            RiscvBlock block = curFunction.getBlocks().get(i);
            block.getPreds().clear();
            block.getSuccs().clear();
        }
        for (int i = 0; i < curFunction.getBlocks().size(); i++) {
            RiscvBlock block = curFunction.getBlocks().get(i);
            if (block.getRiscvInstrs().getTail().getContent() instanceof RiscvBranch
                    && i < curFunction.getBlocks().size() - 1) {
                curFunction.getBlocks().get(i + 1).addPreds(block);
                block.addSuccs(curFunction.getBlocks().get(i + 1));
            }
            DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getTail();
            while (insNode != null && (insNode.getContent() instanceof RiscvBranch ||
                    insNode.getContent() instanceof RiscvJ)) {
                RiscvBlock jblock = (insNode.getContent() instanceof RiscvJ) ?
                        (RiscvBlock) insNode.getContent().getOperands().get(0)
                        : (RiscvBlock) insNode.getContent().getOperands().get(2);
                block.addSuccs(jblock);
                jblock.addPreds(block);
                insNode = insNode.getPred();
            }
        }
    }

    private void markInst(RiscvBlock entrance) {
        boolean changed = true;
        insts.clear();
        while (changed) {
            changed = false;
            LinkedHashSet<RiscvBlock> visitBlockSet = new LinkedHashSet<>();
            ArrayList<RiscvBlock> blocks = new ArrayList<>();
            blocks.add(entrance);
            visitBlockSet.add(entrance);
            //深度优先搜索
            while (!blocks.isEmpty()) {
                RiscvBlock analyzeBlock = blocks.remove(0);
                if(analyzeBlock == null) {
                    continue;
                }
                changed |= markBlock(analyzeBlock);
                for (RiscvBlock block1 : analyzeBlock.getSuccs()) {
                    if(!riscvLoop.get(curLoop).contains(block1)) {
                        continue;
                    }
                    if (!visitBlockSet.contains(block1)) {
                        blocks.add(block1);
                        visitBlockSet.add(block1);
                    }
                }
            }
        }
    }

    private boolean markBlock(RiscvBlock block) {
        boolean changed = false;
        if(block == null) {
            return false;
        }
        for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
             insNode != null; insNode = insNode.getSucc()) {
            RiscvInstr ins = insNode.getContent();
            if (!insts.contains(ins)) {
                if ((ins instanceof RiscvBinary || ins instanceof RiscvLui || ins instanceof RiscvLi) &&
                        ins.getDefReg() != null && ins.getDefReg() instanceof RiscvVirReg
                        && checkOperands(insNode)) {
                    queue.add(new Pair<>(insNode, block));
                    changed = true;
                    insts.add(ins);
                }
            }
        }
        return changed;
    }

    private boolean checkOperands(DoubleNode<RiscvInstr> ins) {
        Boolean flag = true;
        for (RiscvOperand op : ins.getContent().getOperands()) {
            if (op instanceof RiscvImm) {
                flag &= true;
            } else if (op instanceof RiscvVirReg) {
                if (!reg2InstsDef.containsKey(op)) {
                    flag &= true;
                } else if (reg2InstsDef.get(op).size() == 1
                        && insts.contains(reg2InstsDef.get(op).get(0).getContent())) {
                    //加强条件 一旦有定值 要求该定值与该使用在同一基本块内，并且在该使用之前
                    DoubleNode<RiscvInstr> defNode = reg2InstsDef.get(op).get(0);
                    if(defNode.getParentList() == ins.getParentList()) {
                        DoubleNode<RiscvInstr> insNode = defNode.getParentList().getHead();
                        while(insNode != null) {
                            if(insNode == defNode) {
                                break;
                            } else if(insNode == ins){
                                flag = false;
                            }
                            insNode = insNode.getSucc();
                        }
                    } else {
                        flag = false;
                    }
                } else {
                    flag = false;
                }
            } else {
                flag = false;
            }
        }
        return flag;
    }

    private void moveInvar(RiscvBlock entrance) {
        for (var insAndBlock : queue) {
            boolean flag = true;
            DoubleNode<RiscvInstr> insNode = insAndBlock.a;
            RiscvBlock block = insAndBlock.b;
            DoubleNode<RiscvInstr> iterator = block.getRiscvInstrs().getHead();
            while(iterator != null) {
                for(RiscvOperand op:iterator.getContent().getOperands()) {
                    if(op.equals(insNode.getContent().getDefReg())) {
                        flag = false;
                    }
                }
                if(iterator == insNode) {
                    break;
                }
                iterator = iterator.getSucc();
            }
            if(entrance != block && flag) {
                LinkedHashSet<RiscvBlock> allBlock = new LinkedHashSet<>();
                allBlock.addAll(outer);
                allBlock.addAll(reg2BlocksUse.
                        getOrDefault(insNode.getContent().getDefReg(), new ArrayList<>()));
                allBlock.remove(block);
                LinkedHashSet<RiscvBlock> beenDfs = new LinkedHashSet<>();
                dfs(allBlock, beenDfs, block, entrance);
                if(allBlock.size() != 0) {
                    flag = false;
                }
            }
            if (flag && hasLift < 200 && !beenLiftOnce.contains(insAndBlock.a.getContent())) {
                insertPreHead(insAndBlock.a);
            }
        }
    }

    private void dfs(LinkedHashSet<RiscvBlock> allBlock,
                     LinkedHashSet<RiscvBlock> beenDfs, RiscvBlock notAllowed, RiscvBlock from) {
        if(allBlock.contains(from)) {
            allBlock.remove(from);
        }
        beenDfs.add(from);
        for(RiscvBlock block:from.getSuccs()) {
            if(!block.equals(notAllowed) && !beenDfs.contains(block)) {
                dfs(allBlock, beenDfs, notAllowed, block);
            }
        }
    }

    //进行循环内活跃性分析
    private void analyzeLoop(Loop loop) {
        //出于正确性考虑、加强条件 一旦循环内存在对该变量的赋值，且该赋值不是循环不变量则不可以提出
        reg2InstsDef.clear();
        reg2BlocksUse.clear();
        for (RiscvBlock block : riscvLoop.get(loop)) {
            if(block == null) {
                continue;
            }
            DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
            while (insNode != null) {
                if (insNode.getContent().getDefReg() != null &&
                        insNode.getContent().getDefReg() instanceof RiscvVirReg) {
                    if (!reg2InstsDef.containsKey(insNode.getContent().getDefReg())) {
                        reg2InstsDef.put(insNode.getContent().getDefReg(), new ArrayList<>());
                    }
                    reg2InstsDef.get(insNode.getContent().getDefReg()).add(insNode);
                }
                for (RiscvOperand op : insNode.getContent().getOperands()) {
                    if (op instanceof RiscvVirReg) {
                        if (!reg2BlocksUse.containsKey((RiscvReg) op)) {
                            reg2BlocksUse.put((RiscvReg) op, new ArrayList<>());
                        }
                        reg2BlocksUse.get(op).add(block);
                    }
                }
                insNode = insNode.getSucc();
            }
        }
    }

    private void getOuter() {
        outer.clear();
        LinkedHashSet<RiscvBlock> blocks1 = new LinkedHashSet<>();
        for(RiscvBlock block:riscvLoop.get(curLoop)) {
            if(block != null) {
                blocks1.add(block);
            }
        }
        for (RiscvBlock block : blocks1) {
            DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getTail();
            while (insNode != null && (insNode.getContent() instanceof RiscvJ
                    || insNode.getContent() instanceof RiscvBranch)) {
                if (insNode.getContent() instanceof RiscvJ) {
                    if (insNode.getContent().getOperands().get(0) instanceof RiscvBlock) {
                        if (!blocks1.contains((RiscvBlock) insNode.getContent().getOperands().get(0))) {
                            outer.add(block);
                        }
                    }
                } else {
                    if (insNode.getContent().getOperands().get(2) instanceof RiscvBlock) {
                        if (!blocks1.contains((RiscvBlock) insNode.getContent().getOperands().get(2))) {
                            outer.add(block);
                        }
                    }
                }
                insNode = insNode.getPred();
            }
        }
    }
}
