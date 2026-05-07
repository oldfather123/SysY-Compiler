package backend.operand;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.Param;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.GetParam;
import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.MoveIR;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Push;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Instruction.ZextTo;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import mid.Optimizer.ControllFlow.ControlFlowGraph;
import utils.tools.UnionFindSet;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.stream.Collectors;

public class RegisterAlloc {
    private final HashMap<BasicBlock, HashSet<Value>> inValues = new HashMap<>();
    private final HashMap<BasicBlock, HashSet<Value>> outValues = new HashMap<>();
    private final HashMap<BasicBlock, HashSet<Value>> defValues = new HashMap<>();
    private final HashMap<BasicBlock, HashSet<Value>> useValues = new HashMap<>();
    private final HashMap<Value, HashSet<Value>> iConflictMapCopy = new HashMap<>();

    private final HashMap<Value, HashSet<Value>> iConflictMap = new HashMap<>();
    private final HashMap<Value, HashSet<Value>> fConflictMapCopy = new HashMap<>();
    private final HashMap<Value, HashSet<Value>> fConflictMap = new HashMap<>();
    private final LinkedList<Value> conflictMapValueStack = new LinkedList<>();

    //在call执行之前的活跃变量集合
    private final HashMap<Function, HashMap<Call, HashSet<Value>>> activeValueWhenCall = new HashMap<>();
    // 对于跨调用活跃的变量，要尽量将其分配到调用者保存的寄存器中
    private final HashSet<Value> activeValueCrossCall = new HashSet<>();

    private final Module module;
    private final ControlFlowGraph CFG;

    private final ArrayList<BackIReg> iRegisterPool = new ArrayList<>();
    private final ArrayList<BackIReg> fRegisterPool = new ArrayList<>();

    private final HashMap<Value, ArrayList<Value>> mergedMoves = new HashMap<>();

    private final HashMap<Value, ArrayList<Value>> moveRelated = new HashMap<>();

    private final HashSet<MoveIR> movesToMerge = new HashSet<>();

    private final HashSet<Value> spilledValues = new HashSet<>();

    private Function curFunction;

    private UnionFindSet valueMergeSet;

    private int spill = 0;

    public static RegisterAlloc INSTANCE = new RegisterAlloc();

    private RegisterAlloc() {
        module = IRManager.getModule();
        CFG = (new ControlFlowGraph());
        CFG.analyze();

        for (int i = 0; i < 6; i++) { //t6暂时不分配
            iRegisterPool.add(BackRegTable.getBackReg("t" + i));
        }
        for (int i = 0; i < 7; i++) {
            iRegisterPool.add(BackRegTable.getBackReg("a" + i));
        }
        for (int i = 0; i < 12; i++) {
            iRegisterPool.add(BackRegTable.getBackReg("s" + i));
        }
        for (int i = 0; i < 11; i++) {
            fRegisterPool.add(BackFRegTable.getBackReg("ft" + i)); //ft11暂时不分配
        }
        for (int i = 0; i < 7; i++) {
            fRegisterPool.add(BackFRegTable.getBackReg("fa" + i));
        }
        for (int i = 0; i < 7; i++) {
            fRegisterPool.add(BackFRegTable.getBackReg("fs" + i));
        }
    }


    public void alloc() {
//        constGepMove();
        insertConstantMoves();
        paramToStack();
        for (Function f : module.getDecledFunctions()) {
            curFunction = f;
            RegisterValueMap.instance().setCurFunction(curFunction);

            do {
                spilledValues.clear();
                iConflictMap.clear();
                iConflictMapCopy.clear();
                fConflictMap.clear();
                fConflictMapCopy.clear();
                conflictMapValueStack.clear();
                movesToMerge.clear();
                moveRelated.clear();
                mergedMoves.clear();
                activeValueCrossCall.clear();
                activeValueWhenCall.put(f, new HashMap<>());
                RegisterValueMap.instance().clear();
                valueMergeSet = new UnionFindSet();
                preColor();

                for (BasicBlock b : f.getBlocks()) {
                    inValues.put(b, new HashSet<>());
                    outValues.put(b, new HashSet<>());
                    defValues.put(b, new HashSet<>());
                    useValues.put(b, new HashSet<>());
                }

                blockLevelAnalyze();
                instructionLevelAnalyze();
                do {
                    while (!simplify()) {
                        coalesce();
                    }

                    if (!movesToMerge.isEmpty()) {
                        freeze();
                    } else {
                        spill();
                    }
                } while (!(iConflictMap.keySet().stream().allMatch(v -> RegisterValueMap.instance().getRegOf(v) != null) &&
                        fConflictMap.keySet().stream().allMatch(v -> RegisterValueMap.instance().getRegOf(v) != null)));
                select();
                spill += spilledValues.size();
                restart();
                System.out.println(spill);
                System.out.println(spill);
                // 但愿不会死循环
            } while (!spilledValues.isEmpty());
        }
    }

    private void preColor() {
        // 对函数形参进行预着色
        Param fparams = curFunction.getParam();
        int iCnt = 0, fCnt = 0;
        for (int i = 0; i < fparams.getParams().size(); i++) {
            Value fparam = fparams.getParams().get(i);
            BackIReg reg;
            if (isIntReg(fparam) && iCnt <= 7) {
                reg = BackRegTable.getBackReg("a" + iCnt);
                iCnt++;
            } else if (!isIntReg(fparam) && fCnt <= 7) {
                reg = BackFRegTable.getBackReg("fa" + fCnt);
                fCnt++;
            } else {
                continue;
            }
            RegisterValueMap.instance().setRegOf(fparam, reg);
        }
    }

    private void blockLevelAnalyze() {
        boolean hasChanged;
        do {
            hasChanged = false;
            ArrayList<BasicBlock> reverseList = CFG.reverseBfsBlocksOf(curFunction);

            for (BasicBlock b : reverseList) {
                //先求出use和def来 use先使用后定义；def先定义后使用
                HashSet<Value> defOfB = defValues.get(b);
                HashSet<Value> useOfB = useValues.get(b);

                for (Instruction instruction : b.getInstructionList()) {
                    if (instruction instanceof MoveIR moveIR && !(moveIR.getSrc() instanceof ConstNumber)) {
                        movesToMerge.add(moveIR);
                    }

                    if (instruction.isDefInstr() && !useOfB.contains(instruction)) {
                        if (instruction instanceof MoveIR moveIR) {
                            defOfB.add(moveIR.getOriginPhi());
                        } else {
                            defOfB.add(instruction);
                        }
                    }
                    for (Value operand : instruction.getOperandList()) {
                        if (!defOfB.contains(operand) && (operand instanceof Instruction ||
                                curFunction.getParam().getParams().contains(operand))) {
                            //不需要判断move，因为这里只会用到phi而不会用到move的value
                            useOfB.add(operand);
                        }
                    }
                }
            }

            for (BasicBlock b : reverseList) {
                //out[B] = \cup_{children} in[child]
                HashSet<BasicBlock> nextBlocks = CFG.getChildren(b);
                if (nextBlocks != null && nextBlocks.size() != 0) {
                    for (BasicBlock nextBlock : nextBlocks) {
                        outValues.get(b).addAll(inValues.get(nextBlock));
                    }
                }

                //in[B] = use[B] \cup (out[B] - def[B])
                HashSet<Value> originInValues = new HashSet<>(inValues.get(b));
                HashSet<Value> set = new HashSet<>(outValues.get(b));
                set.removeAll(defValues.get(b));

                inValues.get(b).addAll(set);
                inValues.get(b).addAll(useValues.get(b));
                if (!originInValues.equals(inValues.get(b))) {
                    hasChanged = true;
                }
            }
        } while (hasChanged);
    }

    private void instructionLevelAnalyze() {
        /*
            在基本块内部，也可以把每个指令看作一个基本块，上述活跃变量分析的公式依然适用
            从后向前遍历，def->去掉，use->加上，从而求出每个定义点处的active集合
            每次加入active集合时，都要将这些节点两两相连，也即active中这些节点构成完全图
        */

        for (BasicBlock block : curFunction.getBlocks()) {
            HashSet<Value> activeValues = new HashSet<>(outValues.get(block));
            buildCompleteMap(activeValues);
            ArrayList<Instruction> instructions = new ArrayList<>(block.getInstructionList());
            for (int i = instructions.size() - 1; i >= 0; i--) {
                Instruction instruction = instructions.get(i);
                if (instruction.isDefInstr()) {
                    Value activeValue;
                    if (instruction instanceof MoveIR moveIR) {
                        activeValue = moveIR.getOriginPhi();
                    } else {
                        activeValue = instruction;
                    }
                    activeValues.remove(activeValue);
                }

                for (Value operand : instruction.getOperandList()) {
                    if (operand instanceof Instruction || curFunction.getParam().getParams().contains(operand)) {
                        activeValues.add(operand);
                    }
                }
                buildCompleteMap(activeValues);
                if (instruction instanceof Call call) {
                    activeValueWhenCall.get(curFunction).put(call, new HashSet<>(activeValues));
                    activeValueCrossCall.addAll(activeValues);
                }
            }

//            if (block.equals(curFunction.getEntranceBlock())) {
//                // 这里是所有形式参数的定义点
//                for (Value param:curFunction.getParam().getParams()) {
//                    activeValues.remove(param);
//                    buildCompleteMap(activeValues);
//                }
//            }
        }

        for (MoveIR moveIR : movesToMerge) {
            if (!moveRelated.containsKey(moveIR.getOriginPhi())) {
                moveRelated.put(moveIR.getOriginPhi(), new ArrayList<>());
            }
            moveRelated.get(moveIR.getOriginPhi()).add(moveIR.getOperandList().get(0));
        }
    }

    private void buildCompleteMap(HashSet<Value> activeSet) {
        HashSet<Value> iActiveSet = activeSet.stream().filter(this::isIntReg).collect(
                Collectors.toCollection(HashSet::new)
        );
        HashSet<Value> fActiveSet = new HashSet<>(activeSet);
        fActiveSet.removeAll(iActiveSet);
        for (Value value : iActiveSet) {
            if (!iConflictMap.containsKey(value)) {
                iConflictMap.put(value, new HashSet<>());
                iConflictMapCopy.put(value, new HashSet<>());
            }
        }
        for (Value value : fActiveSet) {
            if (!fConflictMap.containsKey(value)) {
                fConflictMap.put(value, new HashSet<>());
                fConflictMapCopy.put(value, new HashSet<>());
            }
        }

        for (Value v : iActiveSet) {
            if (v instanceof GlobalDecl || v instanceof Alloca) {
                continue;
            }
            for (Value u : iActiveSet) {
                if (u instanceof GlobalDecl || u instanceof Alloca || u.equals(v)) {
                    continue;
                }
                if (!iConflictMap.get(u).contains(v) && !u.equals(v)) {
                    iConflictMap.get(u).add(v);
                    iConflictMap.get(v).add(u);
                    iConflictMapCopy.get(u).add(v);
                    iConflictMapCopy.get(v).add(u);
                }
            }
        }

        for (Value v : fActiveSet) {
            if (v instanceof GlobalDecl) {
                continue;
            }
            for (Value u : fActiveSet) {
                if (u instanceof GlobalDecl || u.equals(v)) {
                    continue;
                }
                if (!fConflictMap.get(u).contains(v) && !u.equals(v)) {
                    fConflictMap.get(u).add(v);
                    fConflictMap.get(v).add(u);
                    fConflictMapCopy.get(u).add(v);
                    fConflictMapCopy.get(v).add(u);
                }
            }
        }
    }

    private boolean simplify() {
        int iK = iRegisterPool.size();
        int fK = fRegisterPool.size();
        int exeCnt = 0;
        boolean hasChanged;

        do {
            exeCnt++;
            hasChanged = false;
            HashSet<Value> iValues = new HashSet<>(iConflictMap.keySet());
            HashSet<Value> fValues = new HashSet<>(fConflictMap.keySet());


            HashSet<Value> moveRelatedValues = new HashSet<>();
            for (Value key : moveRelated.keySet()) {
                if (!moveRelated.get(key).isEmpty()) {
                    moveRelatedValues.add(key);
                    moveRelatedValues.addAll(moveRelated.get(key));
                }
            }

            for (Value v : iValues) {
                //如果move-related，那先不要入栈
                if (iConflictMap.get(v).size() < iK && !moveRelatedValues.contains(v)) {
                    removeFromConflictMap(v);
                    conflictMapValueStack.push(v);
                    hasChanged = true;
                    break;
                }
            }

            for (Value v : fValues) {
                if (fConflictMap.get(v).size() < fK && !moveRelatedValues.contains(v)) {
                    removeFromConflictMap(v);
                    conflictMapValueStack.push(v);
                    hasChanged = true;
                    break;
                }
            }
            checkEmptyConflictMap();
        } while (hasChanged);

        return exeCnt == 1;
    }

    private void removeFromConflictMap(Value v) {
        if (isIntReg(v)) {
            ArrayList<Value> conflictValues = new ArrayList<>(iConflictMap.get(v));
            for (Value neighbor : conflictValues) {
                if (iConflictMap.containsKey(neighbor) && iConflictMap.get(neighbor) != null) {
                    iConflictMap.get(neighbor).remove(v);
                }
            }
            iConflictMap.remove(v);
        } else {
            ArrayList<Value> conflictValues = new ArrayList<>(fConflictMap.get(v));
            for (Value neighbor : conflictValues) {
                if (fConflictMap.containsKey(neighbor) && fConflictMap.get(neighbor) != null) {
                    fConflictMap.get(neighbor).remove(v);
                }
            }
            fConflictMap.remove(v);
        }
    }

    private void coalesce() {
        HashSet<MoveIR> movesToMergeIt = new HashSet<>(movesToMerge);
        for (MoveIR moveIR : movesToMergeIt) {
            Value src = moveIR.getSrc();
            Phi phi = moveIR.getOriginPhi();
            Value a = valueMergeSet.findHead(phi);
            Value b = valueMergeSet.findHead(src);
            if (a.equals(b)) {
                continue;
            }
            //合并之后，度数>=K的结点数不会增加
            if (canMerge(a, b)) {
                Value v1 = a;
                Value v2 = b;
                if (RegisterValueMap.instance().getRegOf(b) != null) {
                    v1 = b;
                    v2 = a;
                }
                //把v2合并到v1

                if (isIntReg(v1)) {
                    if (iConflictMap.containsKey(v1) && iConflictMap.containsKey(v2)) {
                        iConflictMap.get(v1).addAll(iConflictMap.get(v2));
                        iConflictMapCopy.get(v1).addAll(iConflictMapCopy.get(v2));
                        iConflictMap.get(v1).remove(v2);
                        iConflictMapCopy.get(v1).remove(v2);

                        HashSet<Value> neighbors = new HashSet<>(iConflictMap.get(v2));
                        for (Value neighbor : neighbors) {
                            if (iConflictMap.containsKey(neighbor)) {
                                iConflictMap.get(neighbor).add(v1);
                                iConflictMapCopy.get(neighbor).add(v1);
                                iConflictMap.get(neighbor).remove(v2);
                                iConflictMapCopy.get(neighbor).remove(v2);
                            }
                        }
                        iConflictMap.remove(v2);
                        iConflictMapCopy.remove(v2);
                    }
                } else {
                    if (fConflictMap.containsKey(v1) && fConflictMap.containsKey(v2)) {
                        fConflictMap.get(v1).addAll(fConflictMap.get(v2));
                        fConflictMapCopy.get(v1).addAll(fConflictMapCopy.get(v2));
                        fConflictMap.get(v1).remove(v2);
                        fConflictMapCopy.get(v1).remove(v2);

                        HashSet<Value> neighbors = new HashSet<>(fConflictMap.get(v2));
                        for (Value neighbor : neighbors) {
                            if (fConflictMap.containsKey(neighbor)) {
                                fConflictMap.get(neighbor).add(v1);
                                fConflictMapCopy.get(neighbor).add(v1);
                                fConflictMap.get(neighbor).remove(v2);
                                fConflictMapCopy.get(neighbor).remove(v2);
                            }
                        }
                        fConflictMap.remove(v2);
                        fConflictMapCopy.remove(v2);
                    }
                }

                //在分配寄存器后，为move的两个操作数设置相同的寄存器
                Value phiHead = valueMergeSet.findHead(phi);
                if (!mergedMoves.containsKey(phiHead)) {
                    mergedMoves.put(phiHead, new ArrayList<>());
                }
                if (moveRelated.containsKey(phiHead)) {
                    moveRelated.get(phiHead).remove(src);
                    if (moveRelated.get(phiHead).isEmpty()) {
                        moveRelated.remove(phiHead);
                    }
                }
                mergedMoves.get(phiHead).add(src);
                movesToMerge.remove(moveIR);
                valueMergeSet.add(v1);
                valueMergeSet.add(v2);
                valueMergeSet.union(v2, v1);
                // v1可能还不是moveRelated，需要更新
                if (!moveRelated.containsKey(v1)) {
                    moveRelated.put(v1, moveRelated.getOrDefault(v2, new ArrayList<>()));
                } else {
                    moveRelated.get(v1).addAll(moveRelated.getOrDefault(v2, new ArrayList<>()));
                }
                moveRelated.remove(v2);

                if (RegisterValueMap.instance().getRegOf(v1) != null) {
                    setRegInMergeMap(v2, RegisterValueMap.instance().getRegOf(v1), new HashSet<>());
                }
            }
        }
        checkEmptyConflictMap();
    }

    private boolean canMerge(Value v1, Value v2) {
        if (isIntReg(v1) && isIntReg(v2)) {
            if (iConflictMap.containsKey(v1) && iConflictMap.containsKey(v2)) {
                if (iConflictMap.get(v1).contains(v2) || iConflictMap.get(v2).contains(v1)) {
                    return false;
                }
                // 都在冲突图中，需要在图里合并
                HashSet<Value> v1Neighbors = new HashSet<>(iConflictMap.get(v1));
                v1Neighbors.addAll(iConflictMap.get(v2));
                return v1Neighbors.size() < iRegisterPool.size();
            } else {
                // 不在冲突图里，说明已经被简化出去了，冲突关系未知，不能合并
                return false;
            }
        } else if (!isIntReg(v1) && !isIntReg(v2)) {
            if (fConflictMap.containsKey(v1) && fConflictMap.containsKey(v2)) {
                if (fConflictMap.get(v1).contains(v2) || fConflictMap.get(v2).contains(v1)) {
                    return false;
                }
                HashSet<Value> v1Neighbors = new HashSet<>(fConflictMap.get(v1));
                v1Neighbors.addAll(fConflictMap.get(v2));
                return v1Neighbors.size() < fRegisterPool.size();
            } else {
                return false;
            }
        } else {
            //如果类型不同，一定不能合并
            return false;
        }
    }

    private void freeze() {
        MoveIR freezeMove = (MoveIR) movesToMerge.toArray()[0];
        movesToMerge.remove(freezeMove);
        Value phiHead = valueMergeSet.findHead(freezeMove.getOriginPhi());
        Value src = freezeMove.getSrc();
        if (moveRelated.containsKey(phiHead)) {
            moveRelated.get(phiHead).remove(src);
            if (moveRelated.get(phiHead).isEmpty()) {
                moveRelated.remove(phiHead);
            }
        }
    }

    private void spill() {
        //随机选择一个节点，将其移走，重复简化
        //或者选择一个cost最小的节点，cost=usewt*(\sum 10^useDepth)+defwt*(\sum 10^defDepth)-copywt*(\sum 10^copyDepth)
        double weight;
        double minWeight = Double.MAX_VALUE;
        Value bestV = null;

        for (Value v : iConflictMap.keySet()) {
            if (RegisterValueMap.instance().getRegOf(v) != null) {
                continue;
            }
            if (v instanceof GetParam) {
                removeFromConflictMap(v);
                conflictMapValueStack.push(v);
                spilledValues.add(v);
                return;
            }
            weight = valueWeight(v);
            if (weight < minWeight) {
                minWeight = weight;
                bestV = v;
            }
        }

        for (Value v : fConflictMap.keySet()) {
            if (RegisterValueMap.instance().getRegOf(v) != null) {
                continue;
            }
            if (v instanceof GetParam) {
                removeFromConflictMap(v);
                conflictMapValueStack.push(v);
                spilledValues.add(v);
                return;
            }
            weight = valueWeight(v);
            if (weight < minWeight) {
                minWeight = weight;
                bestV = v;
            }
//            for (Value u : fConflictMap.get(v)) {
//                weight -= valueWeight(u);
//            }
        }

        if (bestV != null) {
            removeFromConflictMap(bestV);
            conflictMapValueStack.push(bestV);
            spilledValues.add(bestV);
        } else if (!iConflictMap.keySet().stream().allMatch(v -> RegisterValueMap.instance().getRegOf(v) != null)) {
            for (Value v : iConflictMap.keySet()) {
                if (RegisterValueMap.instance().getRegOf(v) != null) {
                    continue;
                }
                removeFromConflictMap(v);
                conflictMapValueStack.push(v);
                spilledValues.add(v);
                break;
            }
        } else if (!fConflictMap.keySet().stream().allMatch(v -> RegisterValueMap.instance().getRegOf(v) != null)) {
            for (Value v : fConflictMap.keySet()) {
                if (RegisterValueMap.instance().getRegOf(v) != null) {
                    continue;
                }
                removeFromConflictMap(v);
                conflictMapValueStack.push(v);
                spilledValues.add(v);
                break;
            }
        }
    }

    private double valueWeight(Value v) {
        final int useWt = 1;
        final int defWt = 1;
        final double alpha = 10;

        if (v.getUserList().size() == 1 && v instanceof Instruction i && i.getBlock().equals(v.getUserList().get(0).getBlock())
                && Math.abs(i.getBlock().getInstructionList().indexOf(i) - i.getBlock().getInstructionList().indexOf(v.getUserList().get(0))) < 5) {
            // if liveRange < 5 then w = max
            return Double.MAX_VALUE;
        }

        int degree = Math.max(iConflictMap.getOrDefault(v, new HashSet<>()).size(),
                fConflictMap.getOrDefault(v, new HashSet<>()).size());
        int loopdepth = 0;
        if (v instanceof Phi phi) {
            for (MoveIR move : phi.getMoveIrs()) {
                loopdepth = Math.max(move.getBlock().getLoopDepth(), loopdepth);
            }
        } else if (v instanceof Instruction) {
            loopdepth = ((Instruction) v).getBlock().getLoopDepth();
        }
        return Math.pow(alpha, loopdepth) / degree * 1024;

//        double weight = 0;
//        if (v instanceof Phi phi) {
//            for (MoveIR move : phi.getMoveIrs()) {
//                weight += defWt * Math.pow(alpha, move.getBlock().getLoopDepth());
//            }
//        } else if (v instanceof Instruction) {
//            weight += defWt * Math.pow(alpha, ((Instruction) v).getBlock().getLoopDepth());
//        } else {
//            return Double.MAX_VALUE;
//        }
//
//        for (User user : v.getUserList()) {
//            if (weight < 0) {
//                break;
//            }
//            if (user instanceof Instruction userInstr) {
//                weight += useWt * Math.pow(alpha, userInstr.getBlock().getLoopDepth());
//            }
//        }
//        return weight;
    }

    private void select() {
        /*
            尽量做到将跨调用活跃的变量分配到被调用者保存的寄存器(s)中，其他变量分配到调用者保护的寄存器(t)中
            meanwhile,尽量把所有寄存器都分配出去
         */
        while (!conflictMapValueStack.isEmpty()) {
            Value v = conflictMapValueStack.pop();
            //指定一个与所有邻接点的颜色不同的颜色
            if (RegisterValueMap.instance().getRegOf(v) != null || v instanceof ConstNumber) {
                continue;
            }
            HashSet<BackIReg> neighborRegs = new HashSet<>();
            if (isIntReg(v)) {
                for (Value neighbor : iConflictMapCopy.getOrDefault(v, new HashSet<>())) {
                    if (RegisterValueMap.instance().getRegOf(neighbor) != null) {
                        neighborRegs.add(RegisterValueMap.instance().getRegOf(neighbor));
                    }
                }
                ArrayList<BackIReg> registerPool = new ArrayList<>(iRegisterPool);
                if (activeValueCrossCall.contains(v)) {
                    Collections.reverse(registerPool);
                }

                if (neighborRegs.size() < registerPool.size()) {
                    //如果邻接点的颜色数小于等于K，则可以分配
                    for (BackIReg r : registerPool) {
                        if (!neighborRegs.contains(r)) {
                            RegisterValueMap.instance().setRegOf(v, r);
                            if (v instanceof Phi phi) {
                                setRegInMergeMap(phi, r, new HashSet<>());
                            }
                            break;
                        }
                    }
                } else {
                    spilledValues.add(v);
                }
            } else {
                for (Value neighbor : fConflictMapCopy.getOrDefault(v, new HashSet<>())) {
                    if (RegisterValueMap.instance().getRegOf(neighbor) != null) {
                        neighborRegs.add(RegisterValueMap.instance().getRegOf(neighbor));
                    }
                }

                ArrayList<BackIReg> registerPool = new ArrayList<>(fRegisterPool);
                if (activeValueCrossCall.contains(v)) {
                    Collections.reverse(registerPool);
                }

                if (neighborRegs.size() < registerPool.size()) {
                    //如果邻接点的颜色数小于等于K，则可以分配
                    for (BackIReg r : registerPool) {
                        if (!neighborRegs.contains(r)) {
                            RegisterValueMap.instance().setRegOf(v, r);
                            if (v instanceof Phi phi) {
                                setRegInMergeMap(phi, r, new HashSet<>());
                            }
                            break;
                        }
                    }
                } else {
                    spilledValues.add(v);
                }
            }
        }
    }

    public HashSet<Value> activeValuesWhenCall(Call call) {
        return activeValueWhenCall.getOrDefault(call.getBlock().getFunction(), new HashMap<>()).getOrDefault(call, new HashSet<>());
    }

    public void setRegInMergeMap(Value v, BackIReg r, HashSet<Value> visited) {
        if (visited.contains(v)) {
            return;
        }
        visited.add(v);
        RegisterValueMap.instance().setRegOf(v, r);
        HashSet<Value> neighborsInMergeMap = new HashSet<>();
        for (Value moveSrc : mergedMoves.getOrDefault(v, new ArrayList<>())) {
            if (moveSrc instanceof ConstNumber) {
                continue;
            }
            neighborsInMergeMap.add(moveSrc);
            if (!(moveSrc instanceof Phi)) {
                for (Value relatedPhi : mergedMoves.keySet()) {
                    if (!v.equals(relatedPhi) && mergedMoves.get(relatedPhi).contains(moveSrc)) {
                        neighborsInMergeMap.add(relatedPhi);
                    }
                }
            }
        }
        for (Value relatedPhi : mergedMoves.keySet()) {
            if (!v.equals(relatedPhi) && mergedMoves.get(relatedPhi).contains(v)) {
                neighborsInMergeMap.add(relatedPhi);
            }
        }
        neighborsInMergeMap.forEach(u -> setRegInMergeMap(u, r, visited));
    }

    public void restart() {
        for (Value v : new HashSet<>(spilledValues)) {
            if (!(v instanceof Instruction inst)) {
                continue;
            }
            if (v instanceof GetParam getParam) {
                for (User user : getParam.getUserList()) {
                    GetParam getParam1 = new GetParam(IRManager.getInstance().declareLocalVar(),
                            getParam.getOffset(), getParam.getType());
                    if (user instanceof Instruction instUser) {
                        instUser.getBlock().addInstructionAt(
                                instUser.getBlock().getInstructionList().indexOf(instUser), getParam1);
                    } else {
                        continue;
                    }
                    user.replaceOperand(inst, getParam1);
                }
                getParam.getBlock().removeInstruction(getParam);
                getParam.destroy();
                continue;
            }
            Alloca alloca;
            if (v.getType() == ValueType.I1) {
                alloca = new Alloca(ValueType.PI32);
            } else {
                alloca = new Alloca(v.getType());
            }
            curFunction.getEntranceBlock().addInstrAtEntry(alloca);
            if (v instanceof Phi phi) {
                for (MoveIR move : new HashSet<>(phi.getMoveIrs())) {
                    Store store = new Store(move.getSrc(), alloca);
                    move.getBlock().addInstructionAt(move.getBlock().getInstructionList().indexOf(move) + 1,
                            store);
                    move.getBlock().removeInstruction(move);
                    move.destroy();
                }

                for (User user : phi.getUserList()) {
                    Load load = new Load(IRManager.getInstance().declareTempVar(), alloca);
                    if (user instanceof Instruction instUser) {
                        instUser.getBlock().addInstructionAt(
                                instUser.getBlock().getInstructionList().indexOf(instUser), load);
                    } else {
                        continue;
                    }
                    user.replaceOperand(inst, load);
                }
            } else {
                Store store = new Store(inst, alloca);
                inst.getBlock().addInstructionAt(inst.getBlock().getInstructionList().indexOf(inst) + 1,
                        store);
                for (User user : inst.getUserList()) {
                    if (user.equals(store)) {
                        continue;
                    }
                    Load load = new Load(IRManager.getInstance().declareTempVar(), alloca);
                    if (user instanceof Instruction instUser) {
                        instUser.getBlock().addInstructionAt(
                                instUser.getBlock().getInstructionList().indexOf(instUser), load);
                    } else {
                        continue;
                    }
                    user.replaceOperand(inst, load);
                }
            }
        }
    }

    private boolean isIntReg(Value v) {
        return v.getType() != ValueType.FLT;
    }

    private void checkEmptyConflictMap() {
        HashSet<Value> emptyKeys = new HashSet<>();
        HashSet<Value> moveRelatedValues = new HashSet<>(moveRelated.keySet());
        for (ArrayList<Value> values : moveRelated.values()) {
            moveRelatedValues.addAll(values);
        }
        for (Value v : iConflictMap.keySet()) {
            if (iConflictMap.get(v).isEmpty() && !moveRelatedValues.contains(v)) {
                emptyKeys.add(v);
                conflictMapValueStack.push(v);
            }
        }
        emptyKeys.forEach(iConflictMap::remove);
        emptyKeys.clear();
        for (Value v : fConflictMap.keySet()) {
            if (fConflictMap.get(v).isEmpty()) {
                emptyKeys.add(v);
                conflictMapValueStack.push(v);
            }
        }
        emptyKeys.forEach(fConflictMap::remove);
    }

    private void insertConstantMoves() {
        for (Function f : module.getDecledFunctions()) {
            for (BasicBlock b : f.getBlocks()) {
                int index = 0;
                while (index < b.getInstructionList().size()) {
                    Instruction i = b.getInstructionList().get(index);
                    if (i instanceof MoveIR || i instanceof ZextTo || i instanceof Call || i instanceof GetElementPtr) {
                        index++;
                        continue;
                    }
                    for (int j = 0; j < i.getOperandList().size(); j++) {
                        Value operand = i.getOperandList().get(j);
                        if (operand instanceof ConstNumber n) {
                            if (i instanceof ALU alu && n.getVal().floatValue() >= -2048 &&
                                    n.getVal().floatValue() <= 2047) {
                                if (alu.getOperator().equals("+") || alu.getOperator().equals("-") &&
                                        (operand.equals(alu.getOperand2()))) {
                                    continue;
                                }
                            }
                            Phi phi = new Phi(n.getType(), IRManager.getInstance().declareTempVar());
                            phi.setBlock(b);
                            MoveIR move = new MoveIR(phi, n);
                            b.addInstructionAt(index, move);
                            // constnumber就不需要考虑use-def了
                            i.getOperandList().set(j, move.getOriginPhi());
                            phi.usedBy(i);
                            index++;
                        }
                    }
                    index++;
                }
            }
        }
    }

    private void paramToStack() {
        for (Function f : IRManager.getModule().getDecledFunctions()) {
            for (BasicBlock b : f.getBlocks()) {
                for (Instruction i : new ArrayList<>(b.getInstructionList())) {
                    if (i instanceof Call call && call.getOperandList().size() > 8) {
                        int intParams = 0;
                        int floatParams = 0;
                        int offset = 0;
                        int index = -1;
                        for (Value v : new ArrayList<>(call.getOperandList())) {
                            index++;
                            if (v instanceof Function) {
                                continue;
                            }
                            if (v.getType() == ValueType.FLT) {
                                floatParams++;
                                if (floatParams > 8) {
                                    if (v instanceof Instruction instr && !(instr instanceof Phi)) {
                                        instr.getBlock().addInstructionAt(
                                                instr.getBlock().getInstructionList().indexOf(instr) + 1,
                                                new Push(v, offset));
                                    } else {
                                        call.getBlock().addInstructionAt(
                                                call.getBlock().getInstructionList().indexOf(call),
                                                new Push(v, offset));
                                    }
                                    if (v instanceof ConstNumber) {
                                        call.getOperandList().remove(index);
                                    } else {
                                        call.removeOperand(v);
                                    }
                                    index--;
                                    offset += 4;
                                }
                            } else {
                                intParams++;
                                if (intParams > 8) {
                                    if (v instanceof Instruction instr && !(instr instanceof Phi)) {
                                        instr.getBlock().addInstructionAt(
                                                instr.getBlock().getInstructionList().indexOf(instr) + 1,
                                                new Push(v, offset));
                                    } else {
                                        call.getBlock().addInstructionAt(
                                                call.getBlock().getInstructionList().indexOf(call),
                                                new Push(v, offset));
                                    }
                                    if (v instanceof ConstNumber) {
                                        call.getOperandList().remove(index);
                                    } else {
                                        call.removeOperand(v);
                                    }
                                    index--;
                                    offset += 8;
                                }
                            }
                        }
                    }
                }
            }
        }


        for (Function f : IRManager.getModule().getDecledFunctions()) {
            ArrayList<Value> params = f.getParam().getParams();
            if (params.size() <= 8) {
                continue;
            }
            int iCnt = 0;
            int fCnt = 0;
            int offset = 0;
            for (int i = 0; i < params.size(); i++) {
                Value param = params.get(i);
                if (isIntReg(param)) {
                    iCnt++;
                    if (iCnt > 8) {
                        if (param.getUserList().isEmpty()) {
                            offset += 8;
                            continue;
                        }
                        GetParam getParam = new GetParam("param" + i, offset, param.getType());
                        f.getEntranceBlock().addInstrAtEntry(getParam);
                        param.beReplacedBy(getParam);
                        offset += 8;
                    }
                } else {
                    fCnt++;
                    if (fCnt > 8) {
                        if (param.getUserList().isEmpty()) {
                            offset += 4;
                            continue;
                        }
                        GetParam getParam = new GetParam("param" + i, offset, param.getType());
                        f.getEntranceBlock().addInstrAtEntry(getParam);
                        param.beReplacedBy(getParam);
                        offset += 4;
                    }
                }
            }
        }
    }

    public void constGepMove() {
        for (Function f : module.getDecledFunctions()) {
            for (BasicBlock b : f.getBlocks()) {
                for (Instruction i : new ArrayList<>(b.getInstructionList())) {
                    if (!(i instanceof GetElementPtr gep && gep.getElemIndex() instanceof ConstNumber n)) {
                        continue;
                    }
                    if (i.getUserList().size() == 1 && i.getUserList().get(0) instanceof Instruction instrUser &&
                            i.getBlock().getLoopDepth() == instrUser.getBlock().getLoopDepth()) {
                        i.getBlock().removeInstruction(i);
                        instrUser.getBlock().addInstructionAt(instrUser.getBlock().getInstructionList().indexOf(instrUser), i);
                    }
                }
            }
        }
    }
}
