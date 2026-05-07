package backend.asm.regalloc;

import IO.Arg;
import backend.asm.insfact.InstrFactory;
import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.ASMInstruction;
import backend.asm.instr.tags.CallIns;
import backend.asm.instr.tags.LoadIns;
import backend.asm.instr.tags.MoveIns;
import backend.asm.instr.tags.StoreIns;
import backend.asm.register.IReg;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.phy.PhyIReg;
import backend.asm.register.phy.PhyReg;
import backend.asm.register.vir.VirFReg;
import backend.asm.register.vir.VirIReg;
import backend.asm.register.vir.VirReg;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMModel;
import util.CustomList;

import java.util.*;

/**
 * 采用寄存器池分配临时寄存器、图着色分配全局寄存器的策略
 */
public class RegAllocator {
    private static RegAllocator UNITED_REG_ALLOCATOR = null;

    private RegStore regStore;
    private InstrFactory instrFactory;

    private final Map<CallIns, Set<PhyReg>> activePhyRegSetMap = new LinkedHashMap<>();   // 记录函数调用时正在活跃的，需要保存的物理寄存器
    private final Set<VirReg> activeVirRegSetAcrossCall = new LinkedHashSet<>();   // 记录函数调用时正在活跃的，需要保存的物理寄存器
    private final List<PhyReg> intColorRegList = new ArrayList<>();
    private final List<PhyReg> intArgRegList = new ArrayList<>();
    private final List<PhyReg> fltColorRegList = new ArrayList<>();
    private final List<PhyReg> fltArgRegList = new ArrayList<>();

    private int spillSwCnt;
    private int spillLwCnt;
    private int protectCnt;

    private RegAllocator() {
    }

    public static RegAllocator getInstance() {
        if (UNITED_REG_ALLOCATOR == null) {
            UNITED_REG_ALLOCATOR = new RegAllocator();
        }
        return UNITED_REG_ALLOCATOR;
    }

    public void allocRegister(ASMModel asmModel, RegStore regStore, InstrFactory instrFactory) {
        this.regStore = regStore;
        this.instrFactory = instrFactory;

        analyzeCallRelationship(asmModel);  // 分析调用关系

        // 将主函数放到最后，从而保证分析到的函数，其调用者都被分析过
        List<ASMFunction> functionList = new ArrayList<>(asmModel.getFunctionList());
        if (functionList.getFirst().isMain()) {
            ASMFunction mainFunction = functionList.getFirst();
            functionList.remove(mainFunction);
            functionList.add(mainFunction);
        }

        // 开始图着色，以函数位单位
        for (ASMFunction function : functionList) {
            initColorRegLists(function);
            allocRegs4Function(function, regStore);

            renewActivePhyRegSetMap(function);
            funcCallProtect(function);
            ensureStackAlignment(function);

            if (function.isParallelLoopBody()) {
                calleeProtect(function);
            }
        }

        if (Arg.ARG.debug()){
            System.err.println("spillSwCnt: " + spillSwCnt);
            System.err.println("spillLwCnt: " + spillLwCnt);
            System.err.println("protectCnt: " + protectCnt);
        }
    }

    private void ensureStackAlignment(ASMFunction asmFunction) {
        int stackSize = asmFunction.getStackSize();
        int padding = (16 - (stackSize & 15)) & 15;
        asmFunction.addAllocatedSize(padding);
    }

    private void renewActivePhyRegSetMap(ASMFunction asmFunction) {
        liveVariableAnalysis(asmFunction);
        for (CustomList.Node blkNode : asmFunction.getBasicBlockList()) {
            for (CustomList.Node insNode : ((ASMBasicBlock) blkNode).getInstructionList()) {
                if (insNode instanceof CallIns) {
                    Set<PhyReg> activeRegSet = activePhyRegSetMap.computeIfAbsent(
                            (CallIns) insNode, _ -> new LinkedHashSet<>()
                    );
                    for (VirReg liveOut : ((ASMInstruction) insNode).getLiveOutSet()) {
                        activeRegSet.add(liveOut.getPhyReg());
                    }
                }
            }
        }
    }

    private void renewActiveVirRegSet(ASMFunction asmFunction) {
        liveVariableAnalysis(asmFunction);
        activeVirRegSetAcrossCall.clear();
        for (CustomList.Node blkNode : asmFunction.getBasicBlockList()) {
            for (CustomList.Node insNode : ((ASMBasicBlock) blkNode).getInstructionList()) {
                if (insNode instanceof CallIns) {
                    activeVirRegSetAcrossCall.addAll(((ASMInstruction) insNode).getLiveOutSet());
                }
            }
        }
    }

    private void allocRegs4Function(ASMFunction asmFunction, RegStore regStore) {
        ConflictGraph intConflictGraph = new ConflictGraph();
        ConflictGraph fltConflictGraph = new ConflictGraph();
        initConflictGraphs(asmFunction, intConflictGraph, fltConflictGraph);

        liveVariableAnalysis(asmFunction);

        rebuildConflictGraph(asmFunction, intConflictGraph);
        rebuildConflictGraph(asmFunction, fltConflictGraph);

        colorConflictGraph(asmFunction, intConflictGraph, intColorRegList, intArgRegList, regStore);
        colorConflictGraph(asmFunction, fltConflictGraph, fltColorRegList, fltArgRegList, regStore);
    }

    private void colorConflictGraph(ASMFunction asmFunction, ConflictGraph conflictGraph,
                                    List<PhyReg> colorRegList, List<PhyReg> argRegList, RegStore regStore) {
        Stack<ConflictGraph.Node> nodeStack = new Stack<>();
        boolean foundSpill = false;
        while (!conflictGraph.nodeMap.isEmpty()) {
            // 简化
            boolean simplified = false;
            for (ConflictGraph.Node node : conflictGraph.nodeMap.values()) {
                if (node.degree < colorRegList.size() && !(conflictGraph.moveNode2Source.containsKey(node))) {
                    for (int i = node.conflictNodes.nextSetBit(0); i >= 0; i = node.conflictNodes.nextSetBit(i + 1)) {
                        conflictGraph.index2Node.get(i).degree--;
                    }
                    conflictGraph.nodeMap.remove(node.virReg, node);
                    nodeStack.push(node);
                    simplified = true;
                    break;
                }
            }
            if (simplified) {
                continue;
            }

            // 合并
            boolean coalesced = false;
            if (!foundSpill) {  // 如果出现潜在的溢出则不再进行合并
                BitSet newConflictSet = new BitSet();
                for (ConflictGraph.Node dstNode : conflictGraph.moveNode2Source.keySet()) {
                    newConflictSet.clear();
                    ConflictGraph.Node srcNode = conflictGraph.moveNode2Source.get(dstNode);
                    newConflictSet.or(dstNode.conflictNodes);
                    newConflictSet.or(srcNode.conflictNodes);
                    int highDegreeNodeCnt = getHighDegreeNodeCnt(
                            dstNode.index, newConflictSet, conflictGraph, srcNode.index, colorRegList
                    );
                    if (highDegreeNodeCnt < colorRegList.size()) {
                        dstNode.virReg.replaceWith(srcNode.virReg);
                        conflictGraph.nodeMap.remove(dstNode.virReg, dstNode);
                        conflictGraph.moveNode2Source.remove(dstNode, srcNode);
                        conflictGraph.moveNode2Source.remove(srcNode, dstNode); // 合并之后不应该让他们反过来再合并一次
                        for (ConflictGraph.Node destOfDest : conflictGraph.moveNode2Source.keySet()) {
                            if (conflictGraph.moveNode2Source.get(destOfDest) == dstNode) {
                                conflictGraph.moveNode2Source.put(destOfDest, srcNode);
                            }
                        }
                        for (ASMInstruction defIns : dstNode.virReg.getDefInsSet()) {
                            defIns.removeFromList();
                        }

                        srcNode.conflictNodes.clear();
                        srcNode.conflictNodes.or(newConflictSet);
                        for (int i = newConflictSet.nextSetBit(0); i >= 0; i = newConflictSet.nextSetBit(i + 1)) {
                            ConflictGraph.Node confNode = conflictGraph.index2Node.get(i);
                            if (confNode.conflictNodes.get(srcNode.index)) {
                                confNode.degree--;
                            } else {
                                confNode.conflictNodes.set(srcNode.index);
                                confNode.conflictNodes.clear(dstNode.index);
                            }
                        }
                        
                        srcNode.degree = srcNode.conflictNodes.cardinality();

                        coalesced = true;
                        break;
                    }
                }
            }
            if (coalesced) {
                continue;
            }

            // 冻结
            ConflictGraph.Node toFreeze = null;
            for (ConflictGraph.Node dstNode : conflictGraph.moveNode2Source.keySet()) {
                if (toFreeze == null || dstNode.degree < toFreeze.degree) {
                    toFreeze = dstNode;
                }
            }
            if (toFreeze != null) {
                conflictGraph.moveNode2Source.remove(toFreeze);
                continue;
            }

            // 溢出（潜在溢出，未来可能被着色）
            ConflictGraph.Node toSpill = null;
            for (ConflictGraph.Node node : conflictGraph.nodeMap.values()) {
                if (toSpill == null || toSpill.degree / toSpill.frequency < node.degree / node.frequency) {
                    toSpill = node;
                }
            }
            if (toSpill == null) {
                throw new RuntimeException("都到这步了怎么还不溢出呢");
            }
            conflictGraph.nodeMap.remove(toSpill.virReg, toSpill);
            nodeStack.push(toSpill);
            foundSpill = true;
        }

        renewActiveVirRegSet(asmFunction);

        Set<Reg> conflictRegs = new LinkedHashSet<>();
        Set<ConflictGraph.Node> assignedNodes = new LinkedHashSet<>();
        boolean spilled = false;
        while (!nodeStack.isEmpty()) {
            ConflictGraph.Node curNode = nodeStack.pop();
            conflictRegs.clear();
            BitSet bs = curNode.conflictNodes;
            for (int i = bs.nextSetBit(0); i >= 0; i = bs.nextSetBit(i + 1)) {
                ConflictGraph.Node conflictNode = conflictGraph.index2Node.get(i);
                if (conflictNode.assignedRegister != null) {
                    conflictRegs.add(conflictNode.assignedRegister);
                }
            }

            boolean activeAcrossCall = activeVirRegSetAcrossCall.contains(curNode.virReg);
            if (!activeAcrossCall && !usedByMoveToArg(curNode.virReg)) {
                for (PhyReg reg : argRegList) {
                    if (!conflictRegs.contains(reg)) {
                        curNode.assignedRegister = reg;
                        break;
                    }
                }
            }
            if (curNode.assignedRegister == null) {
                if (activeAcrossCall) {
                    for (PhyReg reg : colorRegList) {
                        if (!conflictRegs.contains(reg)) {
                            curNode.assignedRegister = reg;
                            break;
                        }
                    }
                } else {
                    for (int i = colorRegList.size() - 1; i >= 0; i--) {
                        PhyReg reg = colorRegList.get(i);
                        if (!conflictRegs.contains(reg)) {
                            curNode.assignedRegister = reg;
                            break;
                        }
                    }
                }

            }

            if (curNode.assignedRegister == null) {
                spilled = true;
                VirReg curReg = curNode.virReg;
                ASMFunction parentFunc = curNode.parentFunc;

                boolean isFlt = curReg instanceof VirFReg;
                boolean isDouble = curReg instanceof VirIReg virIReg && virIReg.isDouble();
                boolean isVector = curReg instanceof VirFReg virFReg && virFReg.isVector();

                Set<ASMInstruction> userSet = new LinkedHashSet<>(curReg.getUserSet()); // 必须在新建 sw 之前，不然 sw 也会进到这个集合里

                Set<ASMInstruction> defSet = curReg.getDefInsSet();
                if (defSet.size() == 1 && defSet.iterator().next() instanceof LoadIns loadIns &&
                        loadIns.getBase() == regStore.getStackPtr()) {
                    // 从栈上 load 要溢出就不要 load 了，用的时候再 load（限制 base 为 SP 是为了避免增大存指针的通用寄存器的活跃范围）
                    ASMValue base = loadIns.getBase();
                    ASMImmediate offset = loadIns.getOffset();
                    for (ASMInstruction userIns : userSet) {
                        ASMInstruction lw;
                        if (isFlt) {
                            lw = instrFactory.createLoad(offset, base, null, new VirFReg(isVector), false, true);
                        } else {
                            VirIReg vir = new VirIReg();
                            vir.setDouble(isDouble);
                            lw = instrFactory.createLoad(offset, base, null, vir, isDouble, false);
                        }
                        lw.insertBefore(userIns);
                        spillLwCnt++;

                        userIns.replaceUsedVal(curReg, lw.getRegister());
                    }
                    
                    ((ASMInstruction) loadIns).removeFromList();
                } else if (defSet.size() == 1 && defSet.iterator().next() instanceof MoveIns moveIns) {
                    // move 要溢出就不要 move 了，直接溢出来源寄存器

                    if (moveIns.getSrc() == regStore.getStackPtr()) {
                        // SP 寄存器不应该被溢出，因为它在整个函数里就不应该被修改，没必要保存在存储器中
                        for (ASMInstruction userIns : userSet) {
                            userIns.replaceUsedVal(curReg, regStore.getStackPtr());
                        }
                    } else {
                        ASMInstruction defIns = (ASMInstruction) moveIns;
                        int basicOffset = parentFunc.getReservedParamSize() + parentFunc.getAllocatedSize();
                        int padding = 0;    // 为了实现对齐进行补位
                        if (isDouble && (basicOffset & 7) != 0) {
                            padding = 4;
                        }
                        ASMImmediate myOffset = new ASMImmediate(basicOffset + padding);
                        parentFunc.addAllocatedSize(padding + (isDouble ? 8 : 4));

                        ASMInstruction sw;
                        Reg srcReg = moveIns.getSrc();
                        if (srcReg instanceof PhyIReg phyIReg && isDouble) {
                            srcReg = new VirIReg();
                            ((VirIReg) srcReg).setDouble(true);
                            ((VirIReg) srcReg).setPhyReg(phyIReg);
                        }
                        if (isFlt) {
                            sw = instrFactory.createStore(myOffset, regStore.getStackPtr(), srcReg,
                                    null, false, true);
                        } else {
                            sw = instrFactory.createStore(myOffset, regStore.getStackPtr(), srcReg,
                                    null, isDouble, false);
                        }
                        sw.insertAfter(defIns);

                        spillSwCnt++;

                        reLoad(regStore, curReg, isFlt, isDouble, isVector, userSet, myOffset);
                    }

                    ((ASMInstruction) moveIns).removeFromList();
                } else {
                    int basicOffset = parentFunc.getReservedParamSize() + parentFunc.getAllocatedSize();
                    int padding = 0;    // 为了实现对齐进行补位
                    if (isDouble && (basicOffset & 7) != 0) {
                        padding = 4;
                    }
                    ASMImmediate myOffset = new ASMImmediate(basicOffset + padding);
                    parentFunc.addAllocatedSize(padding + (isDouble ? 8 : 4));
                    
                    for (ASMInstruction defIns : defSet) {
                        ASMInstruction sw;
                        if (isFlt) {
                            sw = instrFactory.createStore(myOffset, regStore.getStackPtr(), defIns.getRegister(),
                                    null, false, true);
                        } else  {
                            sw = instrFactory.createStore(myOffset, regStore.getStackPtr(), defIns.getRegister(),
                                    null, isDouble, false);
                        }
                        sw.insertAfter(defIns);
                        
                        spillSwCnt++;
                    }
                    
                    reLoad(regStore, curReg, isFlt, isDouble, isVector, userSet, myOffset);
                }
                
            } else {
                assignedNodes.add(curNode);
            }
        }
        if (spilled) {
            allocRegs4Function(asmFunction, regStore);
        } else {
            for (ConflictGraph.Node assignedNode : assignedNodes) {
                assignedNode.virReg.setPhyReg(assignedNode.assignedRegister);
                for (ASMInstruction defIns : assignedNode.virReg.getDefInsSet()) {
                    defIns.getParentBlock().getParentFunction().addUsedReg(assignedNode.assignedRegister);
                }
                for (ASMInstruction userIns : assignedNode.virReg.getUserSet()) {
                    userIns.getParentBlock().getParentFunction().addUsedReg(assignedNode.assignedRegister);
                }
            }
        }
    }

    private void reLoad(RegStore regStore, VirReg curReg, boolean isFlt, boolean isDouble, boolean isVector,
                        Set<ASMInstruction> userSet, ASMImmediate myOffset) {
        for (ASMInstruction userIns : userSet) {
            ASMInstruction lw;
            if (isFlt) {
                lw = instrFactory.createLoad(myOffset, regStore.getStackPtr(), null, new VirFReg(isVector), false, true);
            } else {
                VirIReg vir = new VirIReg();
                vir.setDouble(isDouble);
                lw = instrFactory.createLoad(myOffset, regStore.getStackPtr(),
                        null, vir, isDouble, false);
            }
            lw.insertBefore(userIns);
            spillLwCnt++;

            userIns.replaceUsedVal(curReg, lw.getRegister());
        }
    }
    
    /**
     * 检验一个虚拟寄存器是不是被用来向参数寄存器传值或者存储参数寄存器的值，如果是则不能分配参数寄存器
     */
    private boolean usedByMoveToArg(VirReg virReg) {
        for (ASMInstruction ins : virReg.getUserSet()) {
            if (ins instanceof MoveIns && ins.getRegister() instanceof PhyReg) {
                return true;
            }
            if (ins instanceof StoreIns) {
                return true;
            }
        }
        for (ASMInstruction ins : virReg.getDefInsSet()) {
            if (ins instanceof MoveIns moveIns && moveIns.getSrc() instanceof PhyReg) {
                return true;
            }
            if (ins instanceof LoadIns) {
                return true;
            }
        }
        return false;
    }

    private int getHighDegreeNodeCnt(int dstIdx, BitSet newConflictSet, ConflictGraph conflictGraph,
                                     int srcIdx, List<PhyReg> colorRegList) {
        int highDegreeNodeCnt = 0;
        for (int i = newConflictSet.nextSetBit(0); i >= 0; i = newConflictSet.nextSetBit(i + 1)) {
            ConflictGraph.Node newConflictNode = conflictGraph.index2Node.get(i);
            if (newConflictNode.conflictNodes.get(dstIdx)
                    && newConflictNode.conflictNodes.get(srcIdx)) {
                highDegreeNodeCnt += newConflictNode.degree - 1 >= colorRegList.size() ? 1 : 0;
            } else {
                highDegreeNodeCnt += newConflictNode.degree >= colorRegList.size() ? 1 : 0;
            }
        }
        return highDegreeNodeCnt;
    }

    private void initColorRegLists(ASMFunction function) {
        intColorRegList.clear();
        intArgRegList.clear();
        fltColorRegList.clear();
        fltArgRegList.clear();

        intArgRegList.addAll(regStore.getIntArgRegs().subList(1, regStore.getIntArgRegs().size()));
        fltArgRegList.addAll(regStore.getFltArgRegs().subList(1, regStore.getFltArgRegs().size()));

        Map<PhyReg, Integer> regCntMap = new LinkedHashMap<>();

        for (PhyReg phyReg : regStore.getAllTempRegs()) {
            regCntMap.put(phyReg, 0);
        }
        for (PhyReg phyReg : regStore.getAllIntSavedRegs()) {
            regCntMap.put(phyReg, 0);
        }
        for (PhyReg phyReg : regStore.getAllFltSavedRegs()) {
            regCntMap.put(phyReg, 0);
        }

        for (ASMFunction callee : function.getCalleeSet()) {
            for (PhyReg phyReg : callee.getUsedRegs()) {
                if (regCntMap.containsKey(phyReg)) {
                    regCntMap.put(phyReg, regCntMap.get(phyReg) + 1);
                }
            }
        }

        List<Map.Entry<PhyReg, Integer>> sortList = new ArrayList<>(regCntMap.entrySet());
        sortList.sort(Map.Entry.comparingByValue());

        for (Map.Entry<PhyReg, Integer> entry : sortList) {
            if (entry.getKey() instanceof PhyIReg iReg) {
                intColorRegList.add(iReg);
            } else if (entry.getKey() instanceof PhyFReg fReg) {
                fltColorRegList.add(fReg);
            }
        }
    }

    /**
     * 建立冲突图的节点，只包含待分配的寄存器
     */
    private void initConflictGraphs(ASMFunction function, ConflictGraph intConflictGraph, ConflictGraph fltConflictGraph) {
        intConflictGraph.nodeMap.clear();
        intConflictGraph.moveNode2Source.clear();
        intConflictGraph.index2Node.clear();
        fltConflictGraph.nodeMap.clear();
        fltConflictGraph.moveNode2Source.clear();
        fltConflictGraph.index2Node.clear();

        int intIdx = 0;
        int fltIdx = 0;

        for (CustomList.Node blkNode : function.getBasicBlockList()) {
            for (CustomList.Node insNode : ((ASMBasicBlock) blkNode).getInstructionList()) {
                ASMInstruction ins = (ASMInstruction) insNode;
                if (ins.getRegister() instanceof VirReg virReg && virReg.stillVir()) {
                    if (virReg instanceof VirIReg && !intConflictGraph.nodeMap.containsKey(virReg)) {
                        ConflictGraph.Node node = new ConflictGraph.Node(virReg, function, intIdx);
                        intConflictGraph.index2Node.put(intIdx, node);
                        intIdx++;
                        intConflictGraph.nodeMap.put(virReg, node);
                    } else if (virReg instanceof VirFReg && !fltConflictGraph.nodeMap.containsKey(virReg)) {
                        ConflictGraph.Node node =new ConflictGraph.Node(virReg, function, fltIdx);
                        fltConflictGraph.index2Node.put(fltIdx, node);
                        fltIdx++;
                        fltConflictGraph.nodeMap.put(virReg, node);
                    }

                }
            }
        }
    }

    /**
     * 节点已经在 init 里确定了，本方法要做的是重建冲突边
     */
    private void rebuildConflictGraph(ASMFunction asmFunction, ConflictGraph conflictGraph) {
        for (CustomList.Node blkNode : asmFunction.getBasicBlockList()) {
            for (CustomList.Node insNode : ((ASMBasicBlock) blkNode).getInstructionList()) {
                ASMInstruction ins = (ASMInstruction) insNode;
                if (!(ins.getRegister() instanceof VirReg virReg &&
                        conflictGraph.nodeMap.containsKey(virReg))) {
                    continue;
                }
                ConflictGraph.Node curNode = conflictGraph.nodeMap.get(virReg);

                if (ins instanceof MoveIns move && !move.isFrozen() && curNode.virReg.getDefInsSet().size() == 1) {
                    // 只有只定义了一次的点可以放心合并、标记了冻结的点不可以合并（一般是用来模拟 phi 的并行复制的）
                    ASMValue sourceOfMove = move.getSrc();

                    for (VirReg liveOut : ins.getLiveOutSet()) {
                        if (liveOut != sourceOfMove) {
                            ConflictGraph.Node conflictNode = conflictGraph.nodeMap.get(liveOut);
                            if (conflictNode == null) {
                                continue;
                            }
                            curNode.conflictNodes.set(conflictNode.index);
                            conflictNode.conflictNodes.set(curNode.index);
                        }
                    }
                    if (sourceOfMove instanceof VirReg) {
                        ConflictGraph.Node node = conflictGraph.nodeMap.get(sourceOfMove);
                        if (node != null) {
                            conflictGraph.moveNode2Source.put(curNode, node);
                        }
                    }
                } else {
                    for (VirReg liveOut : ins.getLiveOutSet()) {
                        ConflictGraph.Node conflictNode = conflictGraph.nodeMap.get(liveOut);
                        if (conflictNode == null) {
                            continue;
                        }
                        curNode.conflictNodes.set(conflictNode.index);
                        conflictNode.conflictNodes.set(curNode.index);
                    }
                }
            }
        }

        for (ConflictGraph.Node node : conflictGraph.nodeMap.values()) {
            node.degree = node.conflictNodes.cardinality();
            node.assignedRegister = null;
        }
    }

    private void liveVariableAnalysis(ASMFunction asmFunction) {
        for (CustomList.Node blkNode : asmFunction.getBasicBlockList()) {
            ((ASMBasicBlock) blkNode).initDefUse();
            ((ASMBasicBlock) blkNode).initPreSuc();
            ((ASMBasicBlock) blkNode).initLiveIO();
        }

        analyzeBasicBlocks(asmFunction.getBasicBlockList());

        for (CustomList.Node blkNode : asmFunction.getBasicBlockList()) {
            ASMBasicBlock basicBlock = (ASMBasicBlock) blkNode;
            analyzeInstructions(basicBlock.getInstructionList(), basicBlock.getLiveOutSet());
        }
    }

    private void analyzeInstructions(CustomList instructionList, Set<VirReg> blkLiveOutSet) {
        ASMInstruction ins = (ASMInstruction) instructionList.getTail();

        Set<VirReg> sucLiveInSet = new LinkedHashSet<>(blkLiveOutSet);

        while (ins != null) {
            Set<VirReg> curLiveOutSet = ins.getLiveOutSet();
            curLiveOutSet.clear();
            curLiveOutSet.addAll(sucLiveInSet);

            sucLiveInSet.clear();
            sucLiveInSet.addAll(curLiveOutSet);
            if (ins.getRegister() instanceof VirReg virReg) {
                sucLiveInSet.remove(virReg);
            }
            for (ASMValue usedVal : ins.getUsedValList()) {
                // 这里只考虑需要在这一步分配寄存器的指令
                if (usedVal instanceof VirReg virReg) {
                    sucLiveInSet.add(virReg);
                }
            }

            ins = (ASMInstruction) ins.getPrev();
        }
    }

    private void analyzeBasicBlocks(CustomList basicBlockList) {
        boolean modified = true;
        while (modified) {
            modified = false;
            ASMBasicBlock basicBlock = (ASMBasicBlock) basicBlockList.getTail();
            while (basicBlock != null) {
                modified = basicBlock.renewLiveIO() || modified;
                basicBlock = (ASMBasicBlock) basicBlock.getPrev();
            }
        }
    }

    private void calleeProtect(ASMFunction asmFunction) {
        Set<PhyReg> intRegsToProtect = new LinkedHashSet<>(regStore.getAllIntSavedRegs());
        Set<PhyReg> fltRegsToProtect = new LinkedHashSet<>(regStore.getAllFltSavedRegs());
        Set<PhyReg> usedRegs = asmFunction.getUsedRegs();
        intRegsToProtect.removeIf(reg -> !usedRegs.contains(reg));
        fltRegsToProtect.removeIf(reg -> !usedRegs.contains(reg));

        protectCnt += intRegsToProtect.size();
        protectCnt += fltRegsToProtect.size();

        ASMBasicBlock firstBlk = (ASMBasicBlock) asmFunction.getBasicBlockList().getHead();
        ASMInstruction firstIns = (ASMInstruction) firstBlk.getInstructionList().getHead();
        ASMBasicBlock lastBlk = (ASMBasicBlock) asmFunction.getBasicBlockList().getTail();
        ASMInstruction lastIns = (ASMInstruction) lastBlk.getInstructionList().getTail();

        int offset = asmFunction.getReservedParamSize() + asmFunction.getAllocatedSize();
        if (!intRegsToProtect.isEmpty()) {
            int padding = (16 - (offset & 15)) & 15;
            asmFunction.addAllocatedSize(padding);  // 十六字节对齐，方便可能的指令合并
            offset += padding;
        }
        for (PhyReg reg : intRegsToProtect) {
            ASMImmediate myOffset = new ASMImmediate(offset);
            offset += 8;
            asmFunction.addAllocatedSize(8);    // 物理寄存器就按照 64 位存储了,实在是不好区分单字双字
            VirIReg virIReg1 = new VirIReg();
            virIReg1.setDouble(true);
            virIReg1.setPhyReg(reg);
            ASMInstruction sw = instrFactory.createStore(myOffset, regStore.getStackPtr(),
                    virIReg1, null, true, false);
            sw.insertBefore(firstIns);
            VirIReg virIReg2 = new VirIReg();
            virIReg2.setDouble(true);
            virIReg2.setPhyReg(reg);
            ASMInstruction lw = instrFactory.createLoad(myOffset, regStore.getStackPtr(),
                    null, virIReg2, true, false);
            lw.insertBefore(lastIns);
        }

        if (!fltRegsToProtect.isEmpty()) {
            int padding = (16 - (offset & 15)) & 15;
            asmFunction.addAllocatedSize(padding);  // 十六字节对齐
            offset += padding;
        }
        for (PhyReg reg : fltRegsToProtect) {
            ASMImmediate myOffset = new ASMImmediate(offset);
            offset += 16;
            asmFunction.addAllocatedSize(16);

            VirFReg virFReg1 = new VirFReg(true);
            virFReg1.setPhyReg(reg);
            ASMInstruction sw = instrFactory.createStore(myOffset, regStore.getStackPtr(),
                    virFReg1, null, false, true);
            sw.insertBefore(firstIns);
            VirFReg virFReg2 = new VirFReg(true);
            virFReg2.setPhyReg(reg);
            ASMInstruction lw = instrFactory.createLoad(myOffset, regStore.getStackPtr(),
                    null, virFReg2, false, true);
            lw.insertBefore(lastIns);
        }
    }

    private void funcCallProtect(ASMFunction asmFunction) {
        for (CustomList.Node blkNode : asmFunction.getBasicBlockList()) {
            ASMInstruction ins = (ASMInstruction) ((ASMBasicBlock) blkNode).getInstructionList().getHead();
            while (ins != null) {
                final ASMInstruction nextIns = (ASMInstruction) ins.getNext();
                if (ins instanceof CallIns) {
                    saveAndRestoreRegisters((CallIns) ins, asmFunction);
                }
                ins = nextIns;
            }
        }
    }

    private void analyzeCallRelationship(ASMModel asmModel) {
        for (ASMFunction asmFunction : asmModel.getFunctionList()) {
            for (CustomList.Node blkNode : asmFunction.getBasicBlockList()) {
                ASMInstruction ins = (ASMInstruction) ((ASMBasicBlock) blkNode).getInstructionList().getHead();
                while (ins != null) {
                    final ASMInstruction nextIns = (ASMInstruction) ins.getNext();
                    if (ins instanceof CallIns callIns) {
                        asmFunction.addCallee(callIns.getTargetFunc());
                    }
                    ins = nextIns;
                }
            }
        }
    }

    private void saveAndRestoreRegisters(CallIns callIns, ASMFunction curFunc) {
        ASMFunction tar = callIns.getTargetFunc();
        Set<PhyReg> toBeUsed = callIns.getTargetFunc().toString().equals("OHMParallelFor") ?
                regStore.getAllTempRegs() : tar.getUsedRegs();
        ASMInstruction nxt = (ASMInstruction) ((ASMInstruction) callIns).getNext();

        List<PhyIReg> protectedIRegs = new ArrayList<>();
        List<PhyFReg> protectedFRegs = new ArrayList<>();
        for (PhyReg reg : activePhyRegSetMap.get(callIns)) {
            if (toBeUsed.contains(reg)) {
                if (reg instanceof IReg) {
                    protectedIRegs.add((PhyIReg) reg);
                } else {
                    protectedFRegs.add((PhyFReg) reg);
                }
                protectCnt++;
            }
        }

        int offset = curFunc.getReservedParamSize() + curFunc.getAllocatedSize();
        if (!protectedIRegs.isEmpty()) {
            int padding = (16 - (offset & 15)) & 15;
            curFunc.addAllocatedSize(padding);  // 十六字节对齐，方便可能的指令合并
            offset += padding;
        }
        for (PhyIReg reg : protectedIRegs) {
            ASMImmediate myOffset = new ASMImmediate(offset);
            offset += 8;
            curFunc.addAllocatedSize(8);    // 物理寄存器就按照 64 位存储了,实在是不好区分单字双字
            VirIReg virIReg1 = new VirIReg();
            virIReg1.setDouble(true);
            virIReg1.setPhyReg(reg);
            ASMInstruction sw = instrFactory.createStore(myOffset, regStore.getStackPtr(),
                    virIReg1, null, true, false);
            sw.insertBefore((ASMInstruction) callIns);
            VirIReg virIReg2 = new VirIReg();
            virIReg2.setDouble(true);
            virIReg2.setPhyReg(reg);
            ASMInstruction lw = instrFactory.createLoad(myOffset, regStore.getStackPtr(),
                    null, virIReg2, true, false);
            lw.insertBefore(nxt);
        }

        if (!protectedFRegs.isEmpty()) {
            int padding = (16 - (offset & 15)) & 15;
            curFunc.addAllocatedSize(padding);  // 十六字节对齐
            offset += padding;
        }
        for (PhyFReg reg : protectedFRegs) {
            ASMImmediate myOffset = new ASMImmediate(offset);
            offset += 16;
            curFunc.addAllocatedSize(16);

            VirFReg virFReg1 = new VirFReg(true);
            virFReg1.setPhyReg(reg);
            ASMInstruction sw = instrFactory.createStore(myOffset, regStore.getStackPtr(),
                    virFReg1, null, false, true);
            sw.insertBefore((ASMInstruction) callIns);
            VirFReg virFReg2 = new VirFReg(true);
            virFReg2.setPhyReg(reg);
            ASMInstruction lw = instrFactory.createLoad(myOffset, regStore.getStackPtr(),
                    null, virFReg2, false, true);
            lw.insertBefore(nxt);
        }
    }

    private static class ConflictGraph {
        private final Map<VirReg, Node> nodeMap;
        private final Map<Node, Node> moveNode2Source;
        private final Map<Integer, Node> index2Node;

        private ConflictGraph() {
            this.nodeMap = new LinkedHashMap<>();
            this.moveNode2Source = new LinkedHashMap<>();
            this.index2Node = new LinkedHashMap<>();
        }

        private static class Node {
            private final int index;
            private final VirReg virReg;
            private final BitSet conflictNodes;
            private final int frequency;    // 加一避免除零异常
            private int degree;
            private PhyReg assignedRegister;
            private final ASMFunction parentFunc;

            private Node(VirReg virReg, ASMFunction parentFunc, int index) {
                this.index = index;
                this.virReg = virReg;
                this.conflictNodes = new BitSet();   // 可能很大而且需要频繁resize，要用比较紧凑的数据结构
                int frequency = 1;
                for (ASMInstruction defIns : virReg.getDefInsSet()) {
                    frequency += defIns.getParentBlock().getLoopDepth() + 1;
                }
                for (ASMInstruction userIns : virReg.getUserSet()) {
                    frequency += userIns.getParentBlock().getLoopDepth() + 1;
                }
                this.frequency = frequency;

                this.parentFunc = parentFunc;
            }
        }
    }
}
