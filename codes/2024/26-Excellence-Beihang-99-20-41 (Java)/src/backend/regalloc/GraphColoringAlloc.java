package backend.regalloc;

import backend.component.*;
import backend.instruction.*;
import backend.operand.*;
import utils.Pair;

import java.util.*;
import java.util.stream.Collectors;

enum FloatOrInt {
    FLOAT, INT
}

public class GraphColoringAlloc {
    private final ObjModule module;
    private final HashMap<ObjFunction, Integer> originFuncStack = new HashMap<>();
    private FloatOrInt floatOrInt;

    private int K;
    private HashSet<ObjOperand> all = new HashSet<>();
    private HashSet<ObjOperand> initials = new HashSet<>();
    private HashSet<ObjOperand> S = new HashSet<>();
    private HashSet<ObjOperand> T = new HashSet<>();

    private int procedure;

    private HashMap<ObjOperand, HashSet<ObjOperand>> adjList = new HashMap<>();

    private HashSet<Pair<ObjOperand, ObjOperand>> adjSet = new HashSet<>();

    private HashSet<ObjOperand> simplifyWorklist = new HashSet<>();

    private HashSet<ObjOperand> freezeWorklist = new HashSet<>();

    private HashSet<ObjOperand> spillWorklist = new HashSet<>();

    private HashSet<ObjOperand> spilledNodes = new HashSet<>();

    private HashSet<ObjOperand> coloredNodes = new HashSet<>();

    private Stack<ObjOperand> selectStack = new Stack<>();

    private HashMap<ObjOperand, Integer> degree = new HashMap<>();

    private HashMap<ObjOperand, Integer> color = new HashMap<>();
    private final HashMap<ObjOperand, Integer> loopDepths = new HashMap<>();
    private final HashMap<ObjFunction, Integer> maxUseIa = new HashMap<>();
    private final HashMap<ObjFunction, Integer> maxUseFa = new HashMap<>();

    public GraphColoringAlloc(ObjModule module) {
        this.module = module;
    }

    public void run() {
        for (ObjFunction function : module.getFunctions()) {
            getNeedChangeDestination(function);
        }

        floatOrInt = FloatOrInt.FLOAT;
        for (ObjFunction function : module.getFunctions()) {
            process(function);
            finalAllocate(function);
            saveStage(function);
        }

        floatOrInt = FloatOrInt.INT;
        for (ObjFunction func : module.getFunctions()) {
            process(func);
            finalAllocate(func);
            saveStage(func);
            check();
        }
        resetStackSize();

        for (ObjFunction func : module.getFunctions()) {
            resetNeedChangeDestination(func);
        }
        replaceLiveInfo();
    }

    private void initFull() {
        S = new HashSet<>();
        T = new HashSet<>();
        all = new HashSet<>();
        initials = new HashSet<>();
        adjList = new HashMap<>();
        adjSet = new HashSet<>();
        degree = new HashMap<>();
        initPartial();
    }

    private void initPartial() {
        simplifyWorklist = new HashSet<>();
        freezeWorklist = new HashSet<>();
        spillWorklist = new HashSet<>();
        spilledNodes = new HashSet<>();
        coloredNodes = new HashSet<>();
        selectStack = new Stack<>();
        color = new HashMap<>();
    }


    private void getNeedChangeDestination(ObjFunction function) {
        originFuncStack.put(function, function.getStackSize());
    }

    private void process(ObjFunction function) {
        int[] enough = new int[]{0};
        initFull();
        livenessAnalysis(function);
        procedure = 1;
        K = 12;
        if (processMain(S, function, enough)) {
            return;
        }
        procedure = 2;
        if (floatOrInt == FloatOrInt.INT) {
            K = 7;
            int maxCount = maxUseIa.get(function);

            if (maxCount < 17) {
                K += (17 - maxCount);
            }
        }
        if (floatOrInt == FloatOrInt.FLOAT) {
            K = 12;
            int maxCount = maxUseFa.get(function);

            if (maxCount < 17) {
                K += (17 - maxCount);
            }
        }
        initPartial();
        processMain(T, function, enough); // return;
    }

    private boolean processMain(HashSet<ObjOperand> init, ObjFunction function, int[] enough) {
        initials = init;
        build(function);
        makeWorkList();
        while (!(simplifyWorklist.isEmpty() && freezeWorklist.isEmpty() && spillWorklist.isEmpty())) {
            if (!simplifyWorklist.isEmpty()) {
                simplify();
            } else if (!freezeWorklist.isEmpty()) {
                freeze();
            } else {
                selectSpill();
            }
        }
        assignColors(function);
        if (!spilledNodes.isEmpty()) {
            rewriteProgram(function);
            enough[0] = 1;
            process(function);
        }
        if (enough[0] == 1) {
            return true;
        }
        allocate();
        return false;
    }

    private void livenessAnalysis(ObjFunction func) {
        getDefUse(func);
        DFSGetBBInOut(func);
        getInstInOut(func);
        removeNotUsedDefinition(func);
        fillST(func);
    }

    private void getDefUse(ObjFunction func) {
        maxUseIa.put(func, 9);
        maxUseFa.put(func, 9);
        for (ObjBlock bb : func.getBlocks()) {
            bb.Use.clear();
            bb.Def.clear();
            bb.liveIns.clear();
            bb.liveOuts.clear();
            bb.LocalInterfere.clear();
            for (ObjInstr inst : bb.getInstrs()) {
                for (ObjReg r : inst.regUse) {
                    performDefUseAnalysis(func, r);
                    if (!(bb.Use.contains(r)) && !(bb.Def.contains(r)) && ((floatOrInt == FloatOrInt.INT && r instanceof ObjVirReg || floatOrInt == FloatOrInt.FLOAT && r instanceof ObjFVirReg))) {//
                        bb.Use.add(r);
                    }
                }
                for (ObjReg r : inst.regDef) {
                    performDefUseAnalysis(func, r);
                    if (!(bb.Use.contains(r)) && !bb.Def.contains(r) && ((floatOrInt == FloatOrInt.INT && r instanceof ObjVirReg || floatOrInt == FloatOrInt.FLOAT && r instanceof ObjFVirReg))) {
                        bb.Def.add(r);
                    }
                }

            }

        }
    }

    private void performDefUseAnalysis(ObjFunction func, ObjReg r) {
        if (r instanceof ObjPhyReg pr) {
            if (pr.getIndex() >= 10 && pr.getIndex() <= 17) {
                if (pr.getIndex() > maxUseIa.get(func))
                    maxUseIa.put(func, pr.getIndex());
            }
        }
        if (r instanceof ObjFPhyReg pr) {
            if (pr.getIndex() >= 10 && pr.getIndex() <= 17) {
                if (pr.getIndex() > maxUseFa.get(func))
                    maxUseFa.put(func, pr.getIndex());
            }
        }

        if (floatOrInt == FloatOrInt.INT && r instanceof ObjVirReg || floatOrInt == FloatOrInt.FLOAT && r instanceof ObjFVirReg) {
            all.add(r);
        }
    }

    private int GetInOut(ObjBlock block) {
        //changed return 1
        ArrayList<ObjReg> InBackup = new ArrayList<>(block.liveIns);
        block.liveOuts.clear();
        for (ObjBlock nextBlock : block.getNextBlocks()) {
            for (ObjReg v : nextBlock.liveIns) {
                if (!block.liveOuts.contains(v)) {
                    block.liveOuts.add(v);
                }
            }
        }
        block.liveIns.clear();
        block.liveIns.addAll(block.Use);
        for (ObjReg v : block.liveOuts) {
            if (!block.liveIns.contains(v)) block.liveIns.add(v);
        }
        block.liveIns.removeIf(block.Def::contains);
        boolean theSame = InBackup.size() == block.liveIns.size();
        for (ObjReg x : InBackup) {
            if (!block.liveIns.contains(x)) {
                theSame = false;
                break;
            }
        }

        if (theSame) {
            return 0;
        } else {
            return 1;
        }
    }

    private final ArrayList<ObjBlock> dsfEd = new ArrayList<>();

    private int dfs(ObjBlock block) {
        //changed return 1
        if (dsfEd.contains(block)) {
            return 0;
        }
        dsfEd.add(block);
        int changed = GetInOut(block);
        for (ObjBlock preBlock : block.getPreBlocks()) {
            if (dfs(preBlock) == 1) changed = 1;
        }
        return changed;
    }

    private void DFSGetBBInOut(ObjFunction func) {
        var tails = func.getExits();
        for (var tail : tails) {
            if (tail == null) {
                continue;
            }
            int changed = 1;
            while (changed == 1) {
                dsfEd.clear();
                changed = dfs(tail);
            }
        }
    }

    private final HashSet<ObjOperand> everUsed = new HashSet<>();
    private void getInstInOut(ObjFunction func) {
        everUsed.clear();
        for (ObjBlock block : func.getBlocks()) {
            LinkedList<ObjInstr> instrs = block.getInstrs();
            ListIterator<ObjInstr> it = instrs.listIterator(instrs.size());
            ArrayList<ObjReg> tmpOut = block.liveOuts;
            while (it.hasPrevious()) {
                ObjInstr tmpInst = it.previous();
                everUsed.addAll(tmpInst.regUse);
                if (!it.hasPrevious()) {
                    tmpInst.livein.clear();
                    tmpInst.livein.addAll(block.liveIns);
                    block.LocalInterfere.add(block.liveIns);
                    break;
                }
                ArrayList<ObjReg> tmpIn = new ArrayList<>(tmpOut);
                tmpIn.removeIf(tmpInst.regDef::contains);
                for (ObjReg x : tmpInst.regUse) {
                    if ((floatOrInt == FloatOrInt.INT && x instanceof ObjVirReg || floatOrInt == FloatOrInt.FLOAT && x instanceof ObjFVirReg) && !tmpIn.contains(x))
                        tmpIn.add(x);
                }
                block.LocalInterfere.add(tmpIn);
                tmpInst.livein.clear();
                tmpInst.livein.addAll(tmpIn);
                tmpOut = tmpIn;
            }
        }
    }

    public void removeNotUsedDefinition(ObjFunction func) {
        for (ObjBlock block : func.getBlocks()) {
            HashSet<ObjInstr> toDelete = new HashSet<>();
            for (ObjInstr instr : block.getInstrs()) {
                if (!instr.regDef.isEmpty() && ((instr.regDef.get(0) instanceof ObjVirReg && floatOrInt == FloatOrInt.INT) || (instr.regDef.get(0) instanceof ObjFVirReg && floatOrInt == FloatOrInt.FLOAT)) && !everUsed.contains(instr.regDef.get(0))) {
                    toDelete.add(instr);
                    all.remove(instr.regDef.get(0));
                }
            }
            block.getInstrs().removeAll(toDelete);
        }
    }

    public void fillST(ObjFunction func) {
        S.clear();
        T.clear();
        for (ObjBlock block : func.getBlocks()) {
            S.addAll(block.liveIns);
            LinkedList<ObjInstr> instrs = block.getInstrs();
            ListIterator<ObjInstr> it = instrs.listIterator(instrs.size());
            ArrayList<ObjInstr> temp = new ArrayList<>();
            ObjInstr instr = null;
            ObjInstr nxt;
            boolean check = false;
            while (it.hasPrevious()) {
                nxt = instr;
                instr = it.previous();
                if (!check) {
                    check = true;
                } else {
                    if (instr instanceof ObjCall) {
                        temp.add(nxt);
                    }
                }
            }
            for (ObjInstr x : temp) {
                S.addAll(x.livein);
            }
        }
        for (ObjOperand x : all) {
            if (!S.contains(x)) T.add(x);
        }
    }

    public void build(ObjFunction func) {
        for (ObjBlock block : func.getBlocks()) {
            for (ObjInstr tmpInstr : block.getInstrs()) {
                for (ObjReg r : new LinkedList<ObjReg>() {{
                    addAll(tmpInstr.regUse);
                    addAll(tmpInstr.regDef);
                }}) {
                    if ((floatOrInt == FloatOrInt.INT && r instanceof ObjVirReg || floatOrInt == FloatOrInt.FLOAT && r instanceof ObjFVirReg)) {
                        if (procedure == 1 && S.contains(r) || procedure == 2 && T.contains(r)) {
                            degree.put(r, 0);
                            adjList.put(r, new HashSet<>());
                        }
                    }
                }
            }
        }
        for (ObjBlock block : func.getBlocks()) {
            int depth = block.depth + 1; // TODO: depth calculation
            HashMap<ObjOperand, Integer> defAndUse = new HashMap<>();
            for (ObjInstr tmpInstr : block.getInstrs()) {
                for (ObjReg r : new LinkedList<ObjReg>() {{
                    addAll(tmpInstr.regUse);
                    addAll(tmpInstr.regDef);
                }}) {
                    if ((floatOrInt == FloatOrInt.INT && r instanceof ObjVirReg || floatOrInt == FloatOrInt.FLOAT && r instanceof ObjFVirReg)) {
                        if (procedure == 1 && S.contains(r) || procedure == 2 && T.contains(r)) {
                            defAndUse.putIfAbsent(r, 0);
                            int cur = defAndUse.get(r);
                            cur++;
                            defAndUse.put(r, cur);
                        }
                    }
                }
            }
            for (HashMap.Entry<ObjOperand, Integer> entry : defAndUse.entrySet()) {
                ObjOperand key = entry.getKey();
                int val = entry.getValue();
                loopDepths.putIfAbsent(key, 0);
                loopDepths.computeIfPresent(key, (k, cur) -> cur + 10 * depth * val);
            }
        }
        for (ObjBlock block : func.getBlocks()) {
            ArrayList<ObjReg> ins = block.liveIns;
            addEdges(ins);
            for (ArrayList<ObjReg> x : block.LocalInterfere) {
                addEdges(x);
            }
        }
    }

    private void addEdges(ArrayList<ObjReg> ins) {
        int len = ins.size();
        for (int i = 0; i < len; i++) {
            for (int j = i + 1; j < len; j++) {
                if (procedure == 1) {
                    if (S.contains(ins.get(i)) && S.contains(ins.get(j))) addEdge(ins.get(i), ins.get(j));
                } else {
                    if (T.contains(ins.get(i)) && T.contains(ins.get(j))) addEdge(ins.get(i), ins.get(j));
                }

            }
        }
    }

    public void addEdge(ObjReg in, ObjReg out) {
        if (!adjSet.contains(new Pair<>((ObjOperand) in, (ObjOperand) out)) && !in.equals(out)) {
            adjSet.add(new Pair<>(in, out));
            adjSet.add(new Pair<>(out, in));
            if (!in.isPrecolored()) {
                adjList.putIfAbsent(in, new HashSet<>());
                adjList.get(in).add(out);
                degree.compute(in, (key, value) -> value == null ? 0 : value + 1);

            }
            if (!out.isPrecolored()) {
                adjList.putIfAbsent(out, new HashSet<>());
                adjList.get(out).add(in);
                degree.compute(out, (key, value) -> value == null ? 0 : value + 1);
            }
        }
    }

    private void makeWorkList() {
        for (ObjOperand o : initials) {

            if (degree.get(o) >= K) {
                spillWorklist.add(o);
            } else {
                simplifyWorklist.add(o);
            }
        }
    }

    private void simplify() {
        ObjOperand n = simplifyWorklist.iterator().next();
        simplifyWorklist.remove(n);
        selectStack.push(n);
        for (ObjOperand m : adjacent(n)) {
            decrementDegree(m);
        }
    }

    private void decrementDegree(ObjOperand m) {
        int d = degree.get(m);
        degree.put(m, d - 1);
        if (d == K) {
            spillWorklist.remove(m);
            simplifyWorklist.add(m);
        }
    }

    private HashSet<ObjOperand> adjacent(ObjOperand x) {
        HashSet<ObjOperand> ret = new HashSet<>(adjList.get(x));
        for (ObjOperand i : selectStack) {
            ret.remove(i);
        }
        return ret;
    }

    private void freeze() {
        ObjOperand u = freezeWorklist.iterator().next();
        freezeWorklist.remove(u);
        simplifyWorklist.add(u);
    }

    HashSet<ObjOperand> spilled = new HashSet<>();
    private void selectSpill() {
        ArrayList<ObjOperand> spillList = new ArrayList<>(spillWorklist);
        Collections.shuffle(spillList);
        ObjOperand m = spillList.stream().max((l, r) -> {
            double value1 = degree.getOrDefault(l, 0).doubleValue() / loopDepths.getOrDefault(l, 0);
            double value2 = degree.getOrDefault(r, 0).doubleValue() / loopDepths.getOrDefault(r, 0);
            return Double.compare(value1, value2);
        }).orElseThrow();
        spillWorklist.remove(m);
        spilled.add(m);
        simplifyWorklist.add(m);
        spillWorklist.remove(m);
    }

    private void assignColors(ObjFunction func) {
        while (!selectStack.empty()) {
            ObjOperand n = selectStack.pop();
            HashSet<Integer> okColors = new HashSet<>();
            if (floatOrInt == FloatOrInt.INT) {
                if (procedure == 1) {
                    // S
                    okColors.addAll(List.of(8, 9));
                    for (int i = 18; i <= 27; i++) {
                        okColors.add(i);
                    }
                } else {
                    // T
                    okColors.addAll(List.of(5, 6, 7));
                    for (int i = 28; i <= 31; i++) {
                        okColors.add(i);
                    }
                    for (int ii = maxUseIa.get(func) + 1; ii <= 17; ii++) {
                        okColors.add(ii);
                    }
                }
            }
            if (floatOrInt == FloatOrInt.FLOAT) {
                if (procedure == 1) {
                    // S
                    okColors.addAll(List.of(8, 9));
                    for (int i = 18; i <= 27; i++) {
                        okColors.add(i);
                    }
                } else {
                    //T
                    for (int i = 0; i <= 7; i++) {
                        okColors.add(i);
                    }
                    for (int i = 28; i <= 31; i++) {
                        okColors.add(i);
                    }
                    for (int ii = maxUseFa.get(func) + 1; ii <= 17; ii++) {
                        okColors.add(ii);
                    }
                }
            }
            for (ObjOperand w : adjList.get(n)) {
                if (coloredNodes.contains(getAlias(w))) {
                    okColors.remove(color.get(getAlias(w)));
                }
            }
            if (okColors.isEmpty()) {
                spilledNodes.add(n);
            } else {
                coloredNodes.add(n);
                int c = okColors.iterator().next();
                okColors.remove(c);
                color.put(n, c);
            }
        }
    }

    private ObjOperand getAlias(ObjOperand n) {
        return n;
    }

    private void rewriteProgram(ObjFunction func) {
        int nowOffset = func.getStackSize();
        HashSet<ObjInstr> toRewrite = new HashSet<>();
        HashMap<ObjInstr, ObjBlock> fromWhichBlock = new HashMap<>();
        for (ObjBlock block : func.getBlocks()) {
            for (ObjInstr instr : block.getInstrs()) {
                ArrayList<ObjOperand> tmp = new ArrayList<>(instr.regUse);
                for (ObjReg j : instr.regDef) tmp.remove(j);
                tmp.addAll(instr.regDef);
                for (ObjOperand x : tmp) {
                    if (spilledNodes.contains(x)) {
                        toRewrite.add(instr);
                        fromWhichBlock.put(instr, block);
                        if (x.spillPlace == -1) {
                            x.spillPlace = nowOffset;
                            nowOffset += 8;
                            func.addAllocaSize(8);
                        }
                    }
                }
            }
        }
        for (ObjInstr i : toRewrite) {
            for (ObjOperand x : i.regUse) {
                if (spilledNodes.contains(x)) {
                    if (x instanceof ObjFVirReg) {
                        getLoadSp(x, x.spillPlace, "fld", func, i, 0, fromWhichBlock.get(i));
                    } else {
                        getLoadSp(x, x.spillPlace, "ld", func, i, 0, fromWhichBlock.get(i));
                    }
                }
            }
            for (ObjOperand x : i.regDef) {
                if (spilledNodes.contains(x)) {
                    if (x instanceof ObjFVirReg)
                        getStoreSp(x, x.spillPlace, "fsd", func, i, 0, fromWhichBlock.get(i));
                    else
                        getStoreSp(x, x.spillPlace, "sd", func, i, 0, fromWhichBlock.get(i));
                }
            }

        }
    }

    private ObjOperand genTmpReg(ObjFunction objFunction) {
        ObjVirReg tmpReg = new ObjVirReg();
        objFunction.addUsedVirReg(tmpReg);
        return tmpReg;
    }

    static public void insertAfter(ObjFunction ignore, ObjBlock block, ObjInstr instr, ObjInstr toInsert) {
        LinkedList<ObjInstr> instrs = block.getInstrs();
        int index = instrs.indexOf(instr);
        if (index != -1) {
            if (index == instrs.size() - 1)
                instrs.add(toInsert);
            else instrs.add(index + 1, toInsert);
        }
    }

    static public void insertBefore(ObjFunction ignore, ObjBlock block, ObjInstr instr, ObjInstr toInsert) {
        LinkedList<ObjInstr> instrs = block.getInstrs();
        int index = instrs.indexOf(instr);
        if (index != -1) {
            instrs.add(index, toInsert);
        }
    }

    private void getStoreSp(ObjOperand src, int immediate, String ty,
                            ObjFunction objFunction, ObjInstr instr, int needAlloc, ObjBlock block) {
        if (immediate >= -2048 && immediate <= 2047) {
            ObjImm Imm = new ObjImm(immediate);
            ObjStore objStore = new ObjStore(ObjPhyReg.SP, src, Imm, ty);
            insertAfter(objFunction, block, instr, objStore);
        } else {
            ObjMove objMove;
            ObjBinary objAdd;
            ObjStore objStore;
            if (needAlloc == 1) {
                ObjImm tmpImm = new ObjImm(immediate);
                objMove = new ObjMove(tmpImm, ObjPhyReg.regs.get(5));
                objAdd = new ObjBinary("add", "i32", ObjPhyReg.regs.get(5), ObjPhyReg.SP, ObjPhyReg.regs.get(5));
                objStore = new ObjStore(ObjPhyReg.regs.get(5), src, new ObjImm(0), ty);
            } else {
                ObjOperand tmp = genTmpReg(objFunction);
                ObjImm tmpImm = new ObjImm(immediate);
                objMove = new ObjMove(tmpImm, tmp);
                ObjOperand addr2 = genTmpReg(objFunction);
                objAdd = new ObjBinary("add", "i32", addr2, ObjPhyReg.SP, tmp);
                objStore = new ObjStore(addr2, src, new ObjImm(0), ty);
            }
            insertAfter(objFunction, block, instr, objMove);
            insertAfter(objFunction, block, objMove, objAdd);
            insertAfter(objFunction, block, objAdd, objStore);
        }
    }

    private void getLoadSp(ObjOperand dst, int immediate, String ty,
                           ObjFunction objFunction, ObjInstr instr, int needAlloc, ObjBlock block) {
        if (immediate >= -2048 && immediate <= 2047) {
            ObjImm Imm = new ObjImm(immediate);
            ObjLoad objLoad = new ObjLoad(ty, dst, ObjPhyReg.SP, Imm);
            insertBefore(objFunction, block, instr, objLoad);
        } else {
            ObjMove objMove;
            ObjBinary objAdd;
            ObjLoad objLoad;

            if (needAlloc == 1) {
                ObjImm tmpImm = new ObjImm(immediate);
                objMove = new ObjMove(tmpImm, ObjPhyReg.regs.get(5));
                objAdd = new ObjBinary("add", "i32", ObjPhyReg.regs.get(5), ObjPhyReg.SP, ObjPhyReg.regs.get(5));
                objLoad = new ObjLoad(ty, dst, ObjPhyReg.regs.get(5), new ObjImm(0));
            } else {
                ObjOperand tmp = genTmpReg(objFunction);
                ObjImm tmpImm = new ObjImm(immediate);
                objMove = new ObjMove(tmpImm, tmp);

                ObjOperand addr2 = genTmpReg(objFunction);
                objAdd = new ObjBinary("add", "i32", addr2, ObjPhyReg.SP, tmp);

                objLoad = new ObjLoad(ty, dst, addr2, new ObjImm(0));
            }
            insertBefore(objFunction, block, instr, objLoad);
            insertBefore(objFunction, block, objLoad, objAdd);
            insertBefore(objFunction, block, objAdd, objMove);
        }
    }

    public void allocate() {
        for (HashMap.Entry<ObjOperand, Integer> entry : color.entrySet()) {
            ObjOperand key = entry.getKey();
            key.color = entry.getValue();
        }
    }

    public void finalAllocate(ObjFunction func) {
        for (ObjBlock block : func.getBlocks()) {
            for (ObjInstr instr : block.getInstrs()) {
                for (var x : new LinkedList<ObjReg>() {{
                    addAll(instr.regDef);
                    addAll(instr.regUse);
                }}) {
                    if ((floatOrInt == FloatOrInt.INT && x instanceof ObjVirReg || floatOrInt == FloatOrInt.FLOAT && x instanceof ObjFVirReg)) {
                        assert x.color != -1;
                        if (floatOrInt == FloatOrInt.INT) instr.replaceReg(x, ObjPhyReg.regs.get(x.color));
                        if (floatOrInt == FloatOrInt.FLOAT) instr.replaceReg(x, ObjFPhyReg.regs.get(x.color));
                    }
                }
            }
        }
        for (ObjBlock block : func.getBlocks()) {
            ArrayList<ObjInstr> toDelete = new ArrayList<>();
            for (ObjInstr instr : block.getInstrs()) {
                if (instr instanceof ObjMove && ((ObjMove) instr).getDst().equals(((ObjMove) instr).getSrc())) {
                    toDelete.add(instr);
                }
            }
            block.getInstrs().removeAll(toDelete);
        }
    }

    private boolean isS(int i) {
        return i == 8 || i == 9 || i >= 18 && i <= 27;
    }

    private void saveStage(ObjFunction func) {
        HashSet<ObjPhyReg> changed = new HashSet<>();
        HashSet<ObjFPhyReg> changedF = new HashSet<>();
        for (ObjBlock block : func.getBlocks()) {

            for (ObjInstr instr : block.getInstrs()) {
                if (instr instanceof ObjBinary) {
                    if (((ObjBinary) instr).getDst() instanceof ObjPhyReg dst && floatOrInt == FloatOrInt.INT) {
                        if (isS(dst.getIndex())) {
                            changed.add(dst);
                        }
                    }
                    if (((ObjBinary) instr).getDst() instanceof ObjFPhyReg dst && floatOrInt == FloatOrInt.FLOAT) {
                        if (isS(dst.getIndex())) {
                            changedF.add(dst);
                        }
                    }

                }
                if (instr instanceof ObjLoad) {
                    if (((ObjLoad) instr).getDst() instanceof ObjPhyReg dst && floatOrInt == FloatOrInt.INT) {
                        if (isS(dst.getIndex())) {
                            changed.add(dst);
                        }
                    }
                    if (((ObjLoad) instr).getDst() instanceof ObjFPhyReg dst && floatOrInt == FloatOrInt.FLOAT) {
                        if (isS(dst.getIndex())) {
                            changedF.add(dst);
                        }
                    }
                }
                if (instr instanceof ObjMove) {
                    if (((ObjMove) instr).getDst() instanceof ObjPhyReg dst && floatOrInt == FloatOrInt.INT) {
                        if (isS(dst.getIndex())) {
                            changed.add(dst);
                        }
                    }
                    if (((ObjMove) instr).getDst() instanceof ObjFPhyReg dst && floatOrInt == FloatOrInt.FLOAT) {
                        if (isS(dst.getIndex())) {
                            changedF.add(dst);
                        }
                    }
                }
                if (instr instanceof ObjNot not) {
                    if (not.getResult() instanceof ObjPhyReg dst && floatOrInt == FloatOrInt.INT) {
                        if (isS(dst.getIndex())) {
                            changed.add(dst);
                        }
                    }
                }
                if (instr instanceof ObjConversion) {
                    if (((ObjConversion) instr).getDst() instanceof ObjPhyReg dst && floatOrInt == FloatOrInt.INT) {
                        if (isS(dst.getIndex())) {
                            changed.add(dst);
                        }
                    }
                    if (((ObjConversion) instr).getDst() instanceof ObjFPhyReg dst && floatOrInt == FloatOrInt.FLOAT) {
                        if (isS(dst.getIndex())) {
                            changedF.add(dst);
                        }
                    }
                }

            }
        }
        if (floatOrInt == FloatOrInt.INT)
            for (ObjPhyReg x : changed) {
                x.spillPlace = func.getStackSize();
                if (!func.getFirstBlock().getInstrs().isEmpty()) {
                    getStoreSp(x, func.getStackSize(), "sd", func,
                            func.getFirstBlock().getInstrs().getFirst(), 1, func.getFirstBlock());
                }
                for (var exit : func.getExits()) {
                    int size = exit.getInstrs().size();
                    if (size > 0)
                        getLoadSp(x, func.getStackSize(), "ld", func,
                            func.getFreeStack(exit), 1, exit);
                    else
                        getLoadSp(x, func.getStackSize(), "ld", func,
                            exit.getInstrs().get(0), 1, exit);
                }
                func.addAllocaSize(8);
            }
        if (floatOrInt == FloatOrInt.FLOAT)
            for (ObjFPhyReg x : changedF) {
                x.spillPlace = func.getStackSize();
                if (!func.getFirstBlock().getInstrs().isEmpty()) {
                    getStoreSp(x, func.getStackSize(), "fsd", func,
                        func.getFirstBlock().getInstrs().getFirst(), 1, func.getFirstBlock());
                }
                for (var exit : func.getExits()) {
                    int size = exit.getInstrs().size();
                    if (size > 0)
                        getLoadSp(x, func.getStackSize(), "fld", func,
                            func.getFreeStack(exit), 1, exit);
                    else
                        getLoadSp(x, func.getStackSize(), "fld", func,
                            exit.getInstrs().get(0), 1, exit);
                }
                func.addAllocaSize(8);
            }
    }

    private void check() {
        for (Pair<ObjOperand, ObjOperand> x : adjSet) {
            ObjOperand x1 = x.getFirst();
            ObjOperand x2 = x.getSecond();
            if (x1.color == x2.color)
                System.out.println("ALERT: " + x1 + " " + x2 + " -> " + ObjPhyReg.regs.get(x1.color));
        }
    }

    private void resetStackSize() {
        for (ObjFunction func : module.getFunctions()) {
            LinkedList<ObjInstr> instrs = func.getFirstBlock().getInstrs();
            if (instrs.isEmpty()) {
                // This function does not change the stack!
                return;
            }

            ObjOperand minus = getConstIntOperand(-func.getStackSize(), func, func.getAllocStack(), func.getFirstBlock());
            for (var exit : func.getExits()) {
                ObjOperand plus = getConstIntOperand(func.getStackSize(), func, func.getFreeStack(exit), exit);
                func.resetStackSize(plus, minus);
                func.resetArgsLoadTarget(func.getStackSize());
            }
        }
    }

    private ObjOperand getConstIntOperand(int immediate, ObjFunction objFunction,
                                          ObjInstr nxt_instr, ObjBlock block) {
        ObjImm imm = new ObjImm(immediate);
        ObjImm imm12 = new ObjImm(immediate);
        if ((immediate >= -2048 && immediate <= 2047))
            return imm12;
        else {
            ObjPhyReg dst = ObjPhyReg.regs.get(5);
            objFunction.addUsedVirReg(dst);
            ObjMove objMove = new ObjMove(imm, dst);
            insertBefore(objFunction, block, nxt_instr, objMove);
            return dst;
        }
    }

    private void resetNeedChangeDestination(ObjFunction func) {
        int newStackSize = func.getStackSize();
        int oldStackSize = originFuncStack.get(func);

        for (ObjBlock block : func.getBlocks()) {
            for (ObjInstr instr : block.getInstrs()) {
                if (instr instanceof ObjMove && ((ObjMove) (instr)).needchange) {
                    ObjMove li = (ObjMove) (instr);
                    int dueSize = newStackSize + ((ObjImm) (li.getSrc())).getImmediate() - oldStackSize;
                    li.setSrc(new ObjImm(dueSize));
                }
            }
        }
    }

    private void replaceLiveInfo() {
        for (ObjFunction func : module.getFunctions()) {
            for (ObjBlock block : func.getBlocks()) {
                List<List<ObjReg>> targets = new LinkedList<>() {{
                    add(block.liveIns);
                    add(block.liveOuts);
                    addAll(block.getInstrs().stream().map(x -> x.livein).collect(Collectors.toCollection(LinkedList::new)));
                }};
                for (var lst : targets) {
                    int size = lst.size();
                    for (int i = 0; i < size; i++) {
                        ObjReg x = lst.get(i);
                        if (i != size - 1) {
                            if (x instanceof ObjVirReg) {
                                lst.add(i, ObjPhyReg.regs.get(x.color));
                            }
                            if (x instanceof ObjFVirReg) {
                                lst.add(i, ObjFPhyReg.regs.get(x.color));
                            }
                        }
                    }
                }
            }
        }
    }
}
