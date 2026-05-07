package Backend.process.RegAllocator;

import Backend.component.AsmBlock;
import Backend.component.AsmFunction;
import Backend.component.AsmModule;
import Backend.instruction.*;
import Backend.operand.*;
import Utils.ImmutablePair;
import Backend.component.AsmInstr;

import java.util.*;

import static Backend.operand.AllPhysicalReg.*;

public class Allocator {
    private final static int indexBeforeA = 9;
    private final static int indexMinA = 10;
    private final static int indexMaxA = 17;
    private final AsmModule asmModule;
    private int K;
    private final HashSet<AsmOperand> SForOperands = new HashSet<>();
    private final HashSet<AsmOperand> TForOperands = new HashSet<>();
    private final HashMap<AsmBlock, LinkedList<AsmReg>> UseReg = new HashMap<>();
    private final HashMap<AsmBlock, LinkedList<AsmReg>> DefReg = new HashMap<>();
    private final HashMap<AsmBlock, LinkedList<AsmReg>> liveInsReg = new HashMap<>();
    private final HashMap<AsmBlock, LinkedList<AsmReg>> liveOutsReg = new HashMap<>();
    private final HashMap<AsmBlock, LinkedList<LinkedList<AsmReg>>> blockLocalInterfere = new HashMap<>();
    private final HashSet<AsmOperand> allOperands = new HashSet<>();
    private final HashMap<AsmOperand, HashSet<AsmOperand>> adjList = new HashMap<>();
    private final HashMap<AsmOperand, Integer> degree = new HashMap<>();
    private final HashMap<AsmOperand, Integer> spillWeight = new HashMap<>();
    private final HashSet<ImmutablePair<AsmOperand, AsmOperand>> adjSet = new HashSet<>();
    private final HashSet<AsmOperand> spillWorkList = new HashSet<>();
    private final HashSet<AsmOperand> simWorkList = new HashSet<>();
    private final HashSet<AsmOperand> spilledNodes = new HashSet<>();
    private final Stack<AsmOperand> stackForSelect = new Stack<>();
    private final HashSet<AsmOperand> coloredNodes = new HashSet<>();
    private final HashMap<AsmOperand, Integer> color = new HashMap<>();
    private final ArrayList<AsmBlock> hasBeenDfs = new ArrayList<>();
    private final HashMap<AsmFunction, Integer> maxUsePhyA = new HashMap<>();
    private final HashMap<AsmFunction, Integer> maxUseFPhyA = new HashMap<>();

    public Allocator(AsmModule asmModule) {
        this.asmModule = asmModule;
    }

    private void initReg() {
        for (AsmFunction objFunction : this.asmModule.getFunctions()) {
            for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
                UseReg.put(asmBlock, new LinkedList<>());
                DefReg.put(asmBlock, new LinkedList<>());
                liveInsReg.put(asmBlock, new LinkedList<>());
                liveOutsReg.put(asmBlock, new LinkedList<>());
                blockLocalInterfere.put(asmBlock, new LinkedList<>());
            }
        }
    }

    private void blockClearRegs(AsmBlock asmBlock, String type) {
        switch (type) {
            case "all" -> {
                UseReg.get(asmBlock).clear();
                DefReg.get(asmBlock).clear();
                liveInsReg.get(asmBlock).clear();
                liveOutsReg.get(asmBlock).clear();
                blockLocalInterfere.get(asmBlock).clear();
            }
            case "liveIns" -> liveInsReg.get(asmBlock).clear();
            case "liveOuts" -> liveOutsReg.get(asmBlock).clear();
        }
    }

    private boolean ifContainsReg(AsmBlock asmBlock, AsmReg reg) {
        return !(UseReg.get(asmBlock).contains(reg)) && !(DefReg.get(asmBlock).contains(reg));
    }

    public void run() {
        initReg();
        for (AsmFunction func : asmModule.getFunctions()) {
            process(func, true);
            finalAllocate(func, true);
            if (!func.isIfHasParam()) {
                func.reserveSRegs(true);
            }
        }
        for (AsmFunction func : asmModule.getFunctions()) {
            process(func, false);
            finalAllocate(func, false);
            if (!func.isIfHasParam()) {
                func.reserveSRegs(false);
            }
        }
    }

    public void clear(String type) {
        switch (type) {
            case "ST" -> {
                SForOperands.clear();
                TForOperands.clear();
            }
            case "all" -> {
                SForOperands.clear();
                TForOperands.clear();
                allOperands.clear();
                adjList.clear();
                adjSet.clear();
                simWorkList.clear();
                spillWorkList.clear();
                spilledNodes.clear();
                coloredNodes.clear();
                stackForSelect.clear();
                degree.clear();
                color.clear();
            }
            case "some" -> {
                simWorkList.clear();
                spillWorkList.clear();
                spilledNodes.clear();
                coloredNodes.clear();
                stackForSelect.clear();
                color.clear();
            }
        }
    }

    private void analyzeLiveNess(AsmFunction objFunction, boolean isFloat) {
        analyzeRegisterUsage(objFunction, isFloat);
        analyzeBlock(objFunction);
        analyzeInstInOut(objFunction, isFloat);
        dealWithST(objFunction);
    }

    private void analyzeRegisterUsage(AsmFunction objFunction, boolean isFloat) {
        maxUsePhyA.put(objFunction, indexBeforeA);
        maxUseFPhyA.put(objFunction, indexBeforeA);
        for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
            blockClearRegs(asmBlock, "all");
            for (AsmInstr instr : asmBlock.getInstrList()) {
                for (AsmReg regUse : instr.getRegUse()) {
                    analyzeRegisters(regUse, 1, asmBlock, allOperands, objFunction, isFloat);
                }
                AsmReg regDef = instr.getRegDef();
                analyzeRegisters(regDef, 0, asmBlock, allOperands, objFunction, isFloat);
            }
        }
    }

    private void analyzeRegisters(AsmReg reg, int isUse, AsmBlock asmBlock, HashSet<AsmOperand> all, AsmFunction objFunction, boolean isFloat) {
        if (reg instanceof PhysicalReg && ((PhysicalReg) reg).getIndex() >= indexMinA && ((PhysicalReg) reg).getIndex() <= indexMaxA && ((PhysicalReg) reg).getIndex() > maxUsePhyA.get(objFunction)) {
            maxUsePhyA.put(objFunction, ((PhysicalReg) reg).getIndex());
        } else if (reg instanceof FPhysicalReg && ((FPhysicalReg) reg).getIndex() >= indexMinA && ((FPhysicalReg) reg).getIndex() <= indexMaxA && ((FPhysicalReg) reg).getIndex() > maxUseFPhyA.get(objFunction)) {
            maxUseFPhyA.put(objFunction, ((FPhysicalReg) reg).getIndex());
        }
        if (isFloat) {
            if (reg instanceof FVirtualReg) {
                all.add(reg);
                if (ifContainsReg(asmBlock, reg) && isUse == 1) {
                    UseReg.get(asmBlock).add(reg);
                } else if (ifContainsReg(asmBlock, reg) && isUse == 0) {
                    DefReg.get(asmBlock).add(reg);
                }
            }
        } else {
            if (reg instanceof VirtualReg) {
                all.add(reg);
                if (ifContainsReg(asmBlock, reg) && isUse == 1) {
                    UseReg.get(asmBlock).add(reg);
                } else if (ifContainsReg(asmBlock, reg) && isUse == 0) {
                    DefReg.get(asmBlock).add(reg);
                }
            }
        }
    }

    private void analyzeBlock(AsmFunction objFunction) {
        boolean changed;
        do {
            hasBeenDfs.clear();
            changed = dfs(objFunction.getExitBlock());
        } while (changed);
    }

    private boolean dfs(AsmBlock asmBlock) {
        if (hasBeenDfs.contains(asmBlock)) {
            return false;
        }
        hasBeenDfs.add(asmBlock);
        boolean blockChanged = computeLiveInOut(asmBlock);

        for (AsmBlock preBlock : asmBlock.getPreBlocks()) {
            boolean preBlockChanged = dfs(preBlock);
            blockChanged |= preBlockChanged; // 如果前驱块有变化，则当前块也视为变化
        }

        return blockChanged;
    }

    private boolean computeLiveInOut(AsmBlock asmBlock) {
        ArrayList<AsmReg> inBackup = new ArrayList<>(liveInsReg.get(asmBlock));
        blockClearRegs(asmBlock, "liveOuts");
        for (AsmBlock nextBlock : asmBlock.getNextBlocks()) {
            for (AsmReg reg : liveInsReg.get(nextBlock)) {
                if (!liveOutsReg.get(asmBlock).contains(reg)) {
                    liveOutsReg.get(asmBlock).add(reg);
                }
            }
        }
        blockClearRegs(asmBlock, "liveIns");
        liveInsReg.get(asmBlock).addAll(UseReg.get(asmBlock));
        for (AsmReg reg : liveOutsReg.get(asmBlock)) {
            if (!liveInsReg.get(asmBlock).contains(reg)) {
                liveInsReg.get(asmBlock).add(reg);
            }
        }
        liveInsReg.get(asmBlock).removeIf(DefReg.get(asmBlock)::contains);
        boolean theSame = inBackup.size() == liveInsReg.get(asmBlock).size();
        if (theSame) {
            for (AsmReg reg : inBackup) {
                if (!liveInsReg.get(asmBlock).contains(reg)) {
                    theSame = false;
                    break;
                }
            }
        }
        return !theSame;
    }

    private AsmInstr getPreInstr(AsmInstr instr, LinkedList<AsmInstr> list) {
        int index = list.indexOf(instr);
        if (index == 0) {
            return null;
        } else {
            return list.get(index - 1);
        }
    }

    private void analyzeInstInOut(AsmFunction objFunction, boolean isFloat) {
        for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
            AsmInstr tmpInst = asmBlock.getInstrList().getLast();
            while (tmpInst != null) {
                if (getPreInstr(tmpInst, asmBlock.getInstrList()) == null) {
                    instrClearRegs(tmpInst);
                    tmpInst.getLiveIn().addAll(liveInsReg.get(asmBlock));
                    blockLocalInterfere.get(asmBlock).add(liveInsReg.get(asmBlock));
                    break;
                }
                LinkedList<AsmReg> tmpIn = new LinkedList<>(liveOutsReg.get(asmBlock));
                tmpIn.remove(tmpInst.getRegDef());
                for (AsmReg reg : tmpInst.getRegUse()) {
                    if (!tmpIn.contains(reg)) {
                        if (isFloat) {
                            if (reg instanceof FVirtualReg) {
                                tmpIn.add(reg);
                            }
                        } else {
                            if (reg instanceof VirtualReg) {
                                tmpIn.add(reg);
                            }
                        }
                    }
                }
                blockLocalInterfere.get(asmBlock).add(tmpIn);
                instrClearRegs(tmpInst);
                tmpInst.getLiveIn().addAll(tmpIn);
                liveOutsReg.replace(asmBlock, tmpIn);
                tmpInst = getPreInstr(tmpInst, asmBlock.getInstrList());
            }
        }
    }

    private void instrClearRegs(AsmInstr instr) {
        instr.getLiveIn().clear();
    }

    private void dealWithST(AsmFunction objFunction) {
        clear("ST");
        for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
            SForOperands.addAll(liveInsReg.get(asmBlock));
            for (AsmInstr instr : asmBlock.getInstrList()) {
                if (getPreInstr(instr, asmBlock.getInstrList()) != null && getPreInstr(instr, asmBlock.getInstrList()) instanceof AsmCall) {
                    SForOperands.addAll(instr.getLiveIn());
                }
            }
        }
        for (AsmOperand operand : allOperands) {
            if (!SForOperands.contains(operand)) {
                TForOperands.add(operand);
            }
        }
    }

    public void BuildGraph(AsmFunction objFunction, boolean isFloat, boolean isFirstStage) {
        for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
            HashMap<AsmReg, Integer> regFrequency = new HashMap<>();
            for (AsmInstr tmpInst : asmBlock.getInstrList()) {
                for (AsmReg reg : tmpInst.getRegUse()) {
                    multiResult(regFrequency, reg, isFloat, isFirstStage);
                }
                multiResult(regFrequency, tmpInst.getRegDef(), isFloat, isFirstStage);
            }
            for (AsmReg reg : regFrequency.keySet()) {
                int value = regFrequency.get(reg);
                //int value2 = reg.getUseFrequency();
                spillWeight.compute(reg, (k, oldVal) -> {
                    if (oldVal == null) {
                        oldVal = 0; // 初始值
                    }
                    return oldVal + 10 * value * (asmBlock.getDepth() + 1);
                });
            }
        }
        for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
            for (int i = 0; i < liveInsReg.get(asmBlock).size(); i++) {
                for (int j = i + 1; j < liveInsReg.get(asmBlock).size(); j++) {
                    multiStage(liveInsReg.get(asmBlock).get(i), liveInsReg.get(asmBlock).get(j), isFirstStage);
                }
            }
            for (LinkedList<AsmReg> regList : blockLocalInterfere.get(asmBlock)) {
                for (int i = 0; i < regList.size(); i++) {
                    for (int j = i + 1; j < regList.size(); j++) {
                        multiStage(regList.get(i), regList.get(j), isFirstStage);
                    }
                }
            }
        }
    }

    private void multiResult(HashMap<AsmReg, Integer> regFrequency, AsmReg reg, boolean isFloat, boolean isFirstStage) {
        if (isFloat) {
            if (reg instanceof FVirtualReg) {
                if (isFirstStage) {
                    if (SForOperands.contains(reg)) {
                        helpBuild(regFrequency, reg);
                    }
                } else {
                    if (TForOperands.contains(reg)) {
                        helpBuild(regFrequency, reg);
                    }
                }
            }
        } else {
            if (reg instanceof VirtualReg) {
                if (isFirstStage) {
                    if (SForOperands.contains(reg)) {
                        helpBuild(regFrequency, reg);
                    }
                } else {
                    if (TForOperands.contains(reg)) {
                        helpBuild(regFrequency, reg);
                    }
                }
            }
        }
    }

    private void helpBuild(HashMap<AsmReg, Integer> regFrequency, AsmReg reg) {
        degree.put(reg, 0);
        adjList.put(reg, new HashSet<>());
        regFrequency.merge(reg, 1, Integer::sum);
    }

    private void multiStage(AsmReg leftReg, AsmReg rightReg, boolean isFirstStage) {
        if (isFirstStage) {
            if (SForOperands.contains(leftReg) && SForOperands.contains(rightReg)) {
                AddEdge(leftReg, rightReg);
            }
        } else {
            if (TForOperands.contains(leftReg) && TForOperands.contains(rightReg)) {
                AddEdge(leftReg, rightReg);
            }
        }
    }

    private void AddEdge(AsmOperand left, AsmOperand right) {
        if (!adjSet.contains(new ImmutablePair<>(left, right)) && !left.equals(right)) {
            adjSet.add(new ImmutablePair<>(left, right));
            adjSet.add(new ImmutablePair<>(right, left));
            if (left.isPreColored()) {
                adjList.putIfAbsent(left, new HashSet<>());
                adjList.get(left).add(right);
                int currentValue = degree.getOrDefault(left, 0);
                degree.put(left, currentValue + 1);
            }
            if (right.isPreColored()) {
                adjList.putIfAbsent(right, new HashSet<>());
                adjList.get(right).add(left);
                int currentValue = degree.getOrDefault(right, 0);
                degree.put(right, currentValue + 1);
            }
        }
    }

    public void makeWorkList(boolean isFirstStage) {
        HashSet<AsmOperand> SorT;
        if (isFirstStage) {
            SorT = SForOperands;
        } else {
            SorT = TForOperands;
        }
        for (AsmOperand operand : SorT) {
            if (!degree.containsKey(operand)) {
                continue;
            }
            if (degree.get(operand) >= K) {
                spillWorkList.add(operand);
            } else {
                simWorkList.add(operand);
            }
        }
    }

    public void simplify() {
        Iterator<AsmOperand> iterator = simWorkList.iterator();
        AsmOperand operand = iterator.next();
        simWorkList.remove(operand);
        stackForSelect.push(operand);
        HashSet<AsmOperand> result = Adjacent(operand);
        for (AsmOperand objOperand : result) {
            decrementDegree(objOperand);
        }
    }

    private HashSet<AsmOperand> Adjacent(AsmOperand operand) {
        HashSet<AsmOperand> result = new HashSet<>(adjList.get(operand));
        for (AsmOperand objOperand : stackForSelect) {
            result.remove(objOperand);
        }
        return result;
    }

    private void decrementDegree(AsmOperand operand) {
        degree.put(operand, degree.get(operand) - 1);
        if (degree.get(operand) == K - 1) {
            spillWorkList.remove(operand);
            simWorkList.add(operand);
        }
    }

    public void selectSpill() {
        boolean first = true;
        AsmOperand spillOperand = null;
        for (AsmOperand operand : spillWorkList) {
            if (first) {
                spillOperand = operand;
                first = false;
            } else {
                if (getMaxForSpill(spillOperand) < getMaxForSpill(operand)) {
                    spillOperand = operand;
                }
            }
        }
        spillWorkList.remove(spillOperand);
        simWorkList.add(spillOperand);
    }

    private double getMaxForSpill(AsmOperand operand) {
        return ((double) degree.getOrDefault(operand, 0)) / spillWeight.getOrDefault(operand, 0);
    }

    public void assignColors(AsmFunction objFunction, boolean isFloat, boolean isFirstStage) {
        while (!stackForSelect.isEmpty()) {
            AsmOperand operand = stackForSelect.pop();
            HashSet<Integer> baseOkColors = new HashSet<>();
            if (isFloat) {
                if (isFirstStage) {
                    baseOkColors.addAll(Arrays.asList(8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27));
                } else {
                    baseOkColors.addAll(Arrays.asList(0, 1, 2, 3, 4, 5, 6, 7, 28, 29, 30, 31));
                    int maxUseA = maxUseFPhyA.get(objFunction);
                    for (int i = maxUseA + 1; i <= indexMaxA; i++) {
                        baseOkColors.add(i);
                    }
                }
            } else {
                if (isFirstStage) {
                    baseOkColors.addAll(Arrays.asList(8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27));
                } else {
                    baseOkColors.addAll(Arrays.asList(5, 6, 7, 28, 29, 30, 31));
                    int maxUseA = maxUsePhyA.get(objFunction);
                    for (int i = maxUseA + 1; i <= indexMaxA; i++) {
                        baseOkColors.add(i);
                    }
                }
            }
            for (AsmOperand objOperand : adjList.get(operand)) {
                if (coloredNodes.contains(objOperand)) {
                    baseOkColors.remove(color.get(objOperand));
                }
            }
            if (baseOkColors.isEmpty()) {
                spilledNodes.add(operand);
            } else {
                coloredNodes.add(operand);
                int colorValue = baseOkColors.iterator().next();
                baseOkColors.remove(colorValue);
                color.put(operand, colorValue);
            }
        }
    }

    private void insertLoadInstr(AsmOperand dst, int immediate, String type, AsmFunction objFunction, AsmInstr instr) {
        if (immediate >= -2048 && immediate <= 2047) {
            AsmLoad load = new AsmLoad(Arrays.asList(dst, SP, new AsmImm(immediate, true)), type);
            objFunction.addInstBefore(instr, load);
        } else {
            AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), new AsmImm(immediate, false)));
            AsmBinary add = new AsmBinary("add", Arrays.asList(new VirtualReg(), SP, move.getDst()));
            AsmLoad load = new AsmLoad(Arrays.asList(dst, add.getDst(), new AsmImm(0, true)), type);
            objFunction.addInstBefore(instr, load);
            objFunction.addInstBefore(load, add);
            objFunction.addInstBefore(add, move);
        }
    }

    private void insertStoreInstr(AsmOperand src, int immediate, String type, AsmFunction objFunction, AsmInstr instr) {
        if (immediate >= -2048 && immediate <= 2047) {
            AsmStore store = new AsmStore(Arrays.asList(src, SP, new AsmImm(immediate, true)), type);
            objFunction.addInstAfter(instr, store);
        } else {
            AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), new AsmImm(immediate, false)));
            AsmBinary add = new AsmBinary("add", Arrays.asList(new VirtualReg(), SP, move.getDst()));
            AsmStore store = new AsmStore(Arrays.asList(src, add.getDst(), new AsmImm(0, true)), type);
            objFunction.addInstAfter(instr, move);
            objFunction.addInstAfter(move, add);
            objFunction.addInstAfter(add, store);
        }
    }

    public void rewriteProgram(AsmFunction objFunction) {
        int nowOffset = objFunction.getStackSize();
        HashSet<AsmInstr> needRewrite = new HashSet<>();
        for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
            for (AsmInstr instr : asmBlock.getInstrList()) {
                ArrayList<AsmOperand> tmp = new ArrayList<>(instr.getRegUse());
                tmp.remove(instr.getRegDef());
                tmp.add(instr.getRegDef());
                for (AsmOperand operand : tmp) {
                    if (spilledNodes.contains(operand)) {
                        needRewrite.add(instr);
                        if (operand.getSpillPlace() == -1) {
                            operand.setSpillPlace(nowOffset);
                            nowOffset += 8;
                            objFunction.addAllocSize(8);
                        }
                    }
                }
            }
        }
        for (AsmInstr instr : needRewrite) {
            for (AsmOperand regUse : instr.getRegUse()) {
                if (spilledNodes.contains(regUse)) {
                    if (regUse instanceof FVirtualReg) {
                        AllPhysicalReg.addCounterForFPhy();
                        insertLoadInstr(regUse, regUse.getSpillPlace(), "fld", objFunction, instr);
                    } else {
                        insertLoadInstr(regUse, regUse.getSpillPlace(), "ld", objFunction, instr);
                    }
                }
            }
            AsmOperand regDef = instr.getRegDef();
            if (spilledNodes.contains(regDef)) {
                if (regDef instanceof FVirtualReg) {
                    insertStoreInstr(regDef, regDef.getSpillPlace(), "fsd", objFunction, instr);
                } else {
                    insertStoreInstr(regDef, regDef.getSpillPlace(), "sd", objFunction, instr);
                }
            }
        }
    }

    public void allocate() {
        for (HashMap.Entry<AsmOperand, Integer> entry : color.entrySet()) {
            AsmOperand key = entry.getKey();
            key.setColor(entry.getValue());
        }
    }

    public void process(AsmFunction objFunction, boolean isFloat) {
        clear("all");
        analyzeLiveNess(objFunction, isFloat);
        K = 12;
        BuildGraph(objFunction, isFloat, true);
        makeWorkList(true);
        while (!(simWorkList.isEmpty() && spillWorkList.isEmpty())) {
            if (!simWorkList.isEmpty()) {
                simplify();
            } else {
                selectSpill();
            }
        }
        assignColors(objFunction, isFloat, true);
        if (!spilledNodes.isEmpty()) {
            rewriteProgram(objFunction);
            process(objFunction, isFloat);
            return;
        }
        allocate();
        if (!isFloat) {
            K = 7;
            int maxUseA = maxUsePhyA.get(objFunction);
            if (maxUseA < indexMaxA) {
                K += (indexMaxA - maxUseA);
            }
        } else {
            K = 12;
            int maxUseA = maxUseFPhyA.get(objFunction);
            if (maxUseA < indexMaxA) {
                K += (indexMaxA - maxUseA);
            }
        }
        clear("some");
        BuildGraph(objFunction, isFloat, false);
        makeWorkList(false);
        while (!(simWorkList.isEmpty() && spillWorkList.isEmpty())) {
            if (!simWorkList.isEmpty()) {
                simplify();
            } else {
                selectSpill();
            }
        }
        assignColors(objFunction, isFloat, false);
        if (!spilledNodes.isEmpty()) {
            rewriteProgram(objFunction);
            process(objFunction, isFloat);
            return;
        }
        allocate();
    }

    public void finalAllocate(AsmFunction objFunction, boolean isFloat) {
        for (AsmBlock asmBlock : objFunction.getObjBlocks()) {
            for (AsmInstr instr : asmBlock.getInstrList()) {
                ArrayList<AsmReg> instrReg = new ArrayList<>(instr.getRegUse());
                for (AsmReg reg : instrReg) {
                    helpAllocate(instr, reg, isFloat);
                }
                AsmReg reg = instr.getRegDef();
                helpAllocate(instr, reg, isFloat);
            }
        }
    }

    private void helpAllocate(AsmInstr instr, AsmReg reg, boolean isFloat) {
        if (isFloat) {
            if (reg instanceof FVirtualReg) {
                instr.replaceReg(reg, fPhyAllRegs.get(reg.getColor()));
            }
        } else {
            if (reg instanceof VirtualReg) {
                instr.replaceReg(reg, phyAllRegs.get(reg.getColor()));
            }
        }
    }
}






