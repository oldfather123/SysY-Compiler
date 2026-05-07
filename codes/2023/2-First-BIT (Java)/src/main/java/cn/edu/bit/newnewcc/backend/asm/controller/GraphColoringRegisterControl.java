package cn.edu.bit.newnewcc.backend.asm.controller;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.allocator.StackAllocator;
import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.*;
import cn.edu.bit.newnewcc.backend.asm.util.AsmInstructions;
import cn.edu.bit.newnewcc.backend.asm.util.ComparablePair;
import cn.edu.bit.newnewcc.backend.asm.util.Pair;
import cn.edu.bit.newnewcc.backend.asm.util.Registers;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.pass.ir.structure.LoopForest;

import java.util.*;

public class GraphColoringRegisterControl extends RegisterControl {
    public GraphColoringRegisterControl(AsmFunction function, StackAllocator allocator) {
        super(function, allocator);
        lifeTimeController = function.getLifeTimeController();
    }

    private List<Register> physicRegisters;
    private final LifeTimeController lifeTimeController;
    private final List<LifeTimeInterval> intervals = new ArrayList<>();
    private final Map<LifeTimeInterval, LifeTimeInterval> intervalValues = new HashMap<>();
    private final Set<Register> registerAcrossCall = new HashSet<>();

    private final Map<Register, Set<Register>> interferenceEdges = new HashMap<>();
    private final Map<Register, Set<Register>> coalescentEdges = new HashMap<>();
    private final Set<Register> uncoloredReg = new HashSet<>();
    private final Set<Register> coalescentReg = new HashSet<>();

    private final Stack<Register> stack = new Stack<>();
    private final Map<Register, Register> physicRegisterMap = new HashMap<>();
    private final Map<Register, Integer> spillCost = new HashMap<>();
    private List<AsmInstruction> instList;
    private final static int inf = 0x3f3f3f3f;
    private final IntRegister addressReg = IntRegister.getPhysical(5);

    /**
     * 获取寄存器被spill到内存中的代价，物理寄存器的代价总是inf*2，保证不能被spill
     */
    private void getRegisterCost(List<Register> registers) {
        var irFunction = function.getBaseFunction();
        var loopForest = LoopForest.buildOver(irFunction);
        var basicBlockLoopMap = loopForest.getBasicBlockLoopMap();
        Map<LifeTimeIndex, BasicBlock> indexBlockMap = new HashMap<>();
        List<LifeTimeIndex> blockIndexList = new ArrayList<>();
        for (var instr : instList) {
            if (instr instanceof AsmLabel label) {
                LifeTimeIndex index = LifeTimeIndex.getInstIn(lifeTimeController, instr);
                blockIndexList.add(index);
                indexBlockMap.put(index, function.getBasicBlockByLabel(label));
            }
        }
        for (var reg : registers) {
            int nowBlockId = 0;
            spillCost.put(reg, 0);
            for (var point : lifeTimeController.getPoints(reg)) {
                if (AsmInstructions.getInstRegID(point.getIndex().getSourceInst(), reg).isEmpty()) {
                    continue;
                }
                while (nowBlockId + 1 < blockIndexList.size() && blockIndexList.get(nowBlockId + 1).compareTo(point.getIndex()) < 0) {
                    nowBlockId += 1;
                }
                var index = blockIndexList.get(nowBlockId);
                var block = indexBlockMap.get(index);
                var loop = basicBlockLoopMap.get(block);
                int val = 1;
                if (loop != null) {
                    int loopDepth = loop.getLoopDepth();
                    for (int i = 0; i <= loopDepth; i++) {
                        val = Math.toIntExact(Long.min(inf, val * 10L));
                    }
                }
                spillCost.put(reg, Math.min(inf, spillCost.get(reg) + val));
            }
            if (!reg.isVirtual()) {
                spillCost.remove(reg);
            }
        }
    }

    /**
     * 添加双向干涉边，存在干涉边的两个变量不能存在可并边
     * @param u 边节点
     * @param v 边节点
     */
    private void addInterferenceEdge(Register u, Register v) {
        if (!u.equals(v)) {
            coalescentEdges.get(u).remove(v);
            coalescentEdges.get(v).remove(u);
            interferenceEdges.get(u).add(v);
            interferenceEdges.get(v).add(u);
        }
    }

    /**
     * 在不存在干涉边的情况下添加可并干涉边
     * @param u 边节点
     * @param v 边节点
     */
    private void addCoalesceEdge(Register u, Register v) {
        if (!u.equals(v) && !interferenceEdges.get(u).contains(v)) {
            coalescentEdges.get(u).add(v);
            coalescentEdges.get(v).add(u);
        }
    }

    private void removeEdge(Register u, Register v) {
        if (interferenceEdges.get(u).contains(v)) {
            interferenceEdges.get(u).remove(v);
            interferenceEdges.get(v).remove(u);
        }
        if (coalescentEdges.get(u).contains(v)) {
            coalescentEdges.get(u).remove(v);
            coalescentEdges.get(v).remove(u);
        }
    }

    private LifeTimeInterval getIntervalValue(LifeTimeInterval interval) {
        if (!intervalValues.containsKey(interval)) {
            intervalValues.put(interval, interval);
        }
        return intervalValues.get(interval);
    }

    /**
     * 为虚拟寄存器和代码中含有的可变物理寄存器建立干涉图
     */
    private void buildGraph(boolean freezeAll, List<Register> registers) {
        intervals.clear();
        interferenceEdges.clear();
        coalescentEdges.clear();
        for (var reg : registers) {
            intervals.addAll(lifeTimeController.getInterval(reg));
            interferenceEdges.put(reg, new HashSet<>());
            coalescentEdges.put(reg, new HashSet<>());
        }
        Collections.sort(intervals);
        Set<LifeTimeInterval> activeSet = new HashSet<>();
        Map<Register, LifeTimeInterval> lastActive = new HashMap<>();
        for (var inst : instList) {
            if (inst instanceof AsmMove asmMove && !freezeAll) {
                var regPair = AsmInstructions.getMoveReg(asmMove);
                if (registers.contains(regPair.a) && registers.contains(regPair.b)) {
                    addCoalesceEdge(regPair.a, regPair.b);
                }
            }
        }
        intervalValues.clear();
        for (var reg : Registers.CONSTANT_REGISTERS) {
            lastActive.put(reg, new LifeTimeInterval(reg, new ComparablePair<>(null, null)));
        }
        for (var now : intervals) {
            activeSet.removeIf((r) -> r.range.b.compareTo(now.range.a) < 0);
            var defInst = now.range.a.getSourceInst();
            if (defInst instanceof AsmMove asmMove && !freezeAll) {
                var sourceReg = AsmInstructions.getMoveReg(asmMove).b;
                intervalValues.put(now, getIntervalValue(lastActive.get(sourceReg)));
            }
            for (var last : activeSet) {
                var ru = now.reg;
                var rv = last.reg;
                if (getIntervalValue(last).equals(getIntervalValue(now))) {
                    addCoalesceEdge(ru, rv);
                } else {
                    addInterferenceEdge(ru, rv);
                }
            }
            activeSet.add(now);
            lastActive.put(now.reg, now);
        }
        uncoloredReg.clear();
        coalescentReg.clear();
        for (var reg : registers) {
            if (reg.isVirtual()) {
                if (coalescentEdges.get(reg).size() == 0) {
                    uncoloredReg.add(reg);
                } else {
                    coalescentReg.add(reg);
                }
            }
        }
    }

    /**
     * 获取生命周期跨过call指令的虚拟寄存器，并将其保存
     */
    void getRegisterAcrossCall() {
        registerAcrossCall.clear();
        int intervalId = 0;
        Set<LifeTimeInterval> activeSet = new HashSet<>();
        for (var inst : instList) {
            LifeTimeIndex inIndex = LifeTimeIndex.getInstIn(lifeTimeController, inst);
            activeSet.removeIf((interval) -> interval.range.b.compareTo(inIndex) <= 0);

            if (inst instanceof AsmCall) {
                for (var interval : activeSet) {
                    registerAcrossCall.add(interval.reg);
                }
            }

            LifeTimeIndex outIndex = LifeTimeIndex.getInstOut(lifeTimeController, inst);
            while (intervalId < intervals.size() && intervals.get(intervalId).range.a.compareTo(outIndex) <= 0) {
                activeSet.add(intervals.get(intervalId));
                intervalId += 1;
            }
        }
    }

    /*private final boolean debug = true;
    private List<Register> debug_registers;
    void outputDebugInfo(String name) {
        if (debug) {
            System.out.println(name);
            System.out.println("stack size = " + stack.size());
            System.out.println("registers size = " + debug_registers.size());
            System.out.println("uncolored:");
            for (var reg : uncoloredReg) {
                if (!debug_registers.contains(reg)) {
                    System.out.println(reg);
                }
            }
            System.out.println("coalesce:");
            for (var reg : coalescentReg) {
                if (!debug_registers.contains(reg)) {
                    System.out.println(reg);
                }
            }
            System.out.println("stack:");
            for (var reg : stack) {
                if (!debug_registers.contains(reg)) {
                    System.out.println(reg);
                }
            }
        }
    }

    void debug_check(Register register) {
        if (debug) {
            if (debug_registers.contains(register)) {
                System.out.println("have register");
            }
            for (int i = 0; i < instList.size(); i++) {
                var inst = instList.get(i);
                if (!AsmInstructions.getInstRegID(inst, register).isEmpty()) {
                    System.out.println(i + ": " + inst);
                }
            }
        }
    }*/

    /**
     * 未当前剩下的未着色的寄存器进行着色，若成功则重建图并构建出每个寄存器对应的物理寄存器
     * @return 若成功染色返回true，否则返回false
     */
    private boolean color() {
        Queue<Register> queue = new ArrayDeque<>();
        Set<Register> visited = new HashSet<>();

        for (var x : uncoloredReg) {
            if (interferenceEdges.get(x).size() < physicRegisters.size()) {
                queue.add(x);
                visited.add(x);
            }
        }
        while (!queue.isEmpty()) {
            var v = queue.remove();
            stack.push(v);
            uncoloredReg.remove(v);
            for (var u : Set.copyOf(interferenceEdges.get(v))) {
                removeEdge(u, v);
                if (interferenceEdges.get(u).size() < physicRegisters.size()) {
                    if (uncoloredReg.contains(u) && !visited.contains(u)) {
                        queue.add(u);
                        visited.add(u);
                    }
                }
            }
        }
        return uncoloredReg.size() + coalescentReg.size() <= 0;
    }


    void colorToRegisterMap(List<Register> registers) {
        buildGraph(true, registers);
        getRegisterAcrossCall();
        for (var reg : registers) {
            if (!reg.isVirtual()) {
                physicRegisterMap.put(reg, reg);
            }
        }
        Set<Register> usedRegister = new HashSet<>();
        while (!stack.empty()) {
            var v = stack.pop();
            Map<Register, Integer> selectScore = new HashMap<>();
            for (var reg : physicRegisters) {
                selectScore.put(reg, 0);
            }
            for (var u : interferenceEdges.get(v)) {
                if (physicRegisterMap.containsKey(u)) {
                    selectScore.put(physicRegisterMap.get(u), -4);
                }
            }
            for (var pReg : physicRegisters) {
                if ((registerAcrossCall.contains(v) && Registers.isPreservedAcrossCalls(pReg)) ||
                        (!registerAcrossCall.contains(v) && !Registers.isPreservedAcrossCalls(pReg))) {
                    selectScore.put(pReg, selectScore.get(pReg) + 2);
                }
                if (usedRegister.contains(pReg)) {
                    selectScore.put(pReg, selectScore.get(pReg) + 1);
                }
            }
            Register result = null;
            for (var pReg : physicRegisters) {
                if (result == null || selectScore.get(result) < selectScore.get(pReg)) {
                    result = pReg;
                }
            }
            physicRegisterMap.put(v, result);
            usedRegister.add(result);
        }
    }

    private boolean BriggsCheck(Register u, Register v) {
        if (interferenceEdges.get(u).size() < interferenceEdges.get(v).size()) {
            var tmp = u;
            u = v;
            v = tmp;
        }
        int degree = interferenceEdges.get(u).size();
        for (var x : interferenceEdges.get(v)) {
            if (degree >= physicRegisters.size()) {
                break;
            }
            degree += interferenceEdges.get(u).contains(x) ? 0 : 1;
        }
        return degree < physicRegisters.size();
    }

    private boolean GeorgeCheck(Register u, Register v) {
        for (var x : interferenceEdges.get(v)) {
            if (!interferenceEdges.get(u).contains(x)) {
                return false;
            }
        }
        return true;
    }

    /**
     * 检查是否能将寄存器v合并入寄存器u
     * @param u 并入的寄存器
     * @param v 被合并的寄存器
     * @return 是否能够合并
     */
    private boolean checkCoalesce(Register u, Register v) {
        if (!v.isVirtual() || !coalescentEdges.containsKey(u) || !coalescentEdges.get(u).contains(v)) {
            return false;
        }
        return BriggsCheck(u, v) || GeorgeCheck(u, v);
    }

    void mergeEdges(Register u, Register v) {
        removeEdge(u, v);
        for (var x : Set.copyOf(interferenceEdges.get(v))) {
            addInterferenceEdge(u, x);
            removeEdge(v, x);
        }
        for (var x : Set.copyOf(coalescentEdges.get(v))) {
            addCoalesceEdge(u, x);
            removeEdge(v, x);
        }
        interferenceEdges.remove(v);
        coalescentEdges.remove(v);
    }

    /**
     * 执行将寄存器v合并入寄存器u的过程，分为替换指令，合并生存点和合并干涉图三步，最后删除所有和节点v相关的信息
     * @param u 并入的寄存器
     * @param v 被合并的寄存器
     */
    private void practiseCoalesce(Register u, Register v, List<Register> registers) {
        for (var point : lifeTimeController.getPoints(v)) {
            /*if (debug) {
                System.out.println(v + ": " + point);
            }*/
            var inst = point.getIndex().getSourceInst();
            for (var id : AsmInstructions.getInstRegID(inst, v)) {
                inst.setOperand(id, ((RegisterReplaceable)inst.getOperand(id)).withRegister(u));
            }
        }
        lifeTimeController.mergePoints(u, v);
        for (var point : Set.copyOf(lifeTimeController.getPoints(u))) {
            var inst = point.getIndex().getSourceInst();
            if (inst instanceof AsmMove iMove) {
                var regs = AsmInstructions.getMoveReg(iMove);
                if (regs.a.equals(regs.b)) {
                    lifeTimeController.removeLifeTimePoint(u, point);
                    if (lifeTimeController.containsInst(inst)) {
                        int idx = lifeTimeController.getInstID(inst);
                        var nop = new AsmNop();
                        lifeTimeController.replaceInst(inst, nop);
                        instList.set(idx, nop);
                    }
                }
            }
        }
        lifeTimeController.constructInterval(u);
        mergeEdges(u, v);
        if (coalescentEdges.get(u).size() == 0 && u.isVirtual()) {
            coalescentReg.remove(u);
            uncoloredReg.add(u);
        }
        coalescentReg.remove(v);
        registers.remove(v);
        spillCost.remove(v);
        /*if (debug) {
            System.out.println(u + ", " + v);
            debug_check(v);
        }*/
    }

    /**
     * 寄存器合并过程，若成功合并寄存器，则重新进行着色检查
     * @return 是否成功合并寄存器
     */
    private boolean coalesce(List<Register> registers) {
        List<Pair<Register, Register>> coalescePair = new ArrayList<>();
        for (var u : coalescentReg) {
            for (var v : coalescentEdges.get(u)) {
                coalescePair.add(new Pair<>(u, v));
            }
        }
        boolean coalesced = false;
        for (var p : coalescePair) {
            if (checkCoalesce(p.a, p.b)) {
                practiseCoalesce(p.a, p.b, registers);
                coalesced = true;
            } else if (checkCoalesce(p.b, p.a)) {
                practiseCoalesce(p.b, p.a, registers);
                coalesced = true;
            }
        }
        return coalesced;
    }

    boolean freeze() {
        Register freezeReg = null;
        for (var v : coalescentReg) {
            if (freezeReg == null || interferenceEdges.get(v).size() < interferenceEdges.get(freezeReg).size()) {
                freezeReg = v;
            }
        }
        if (freezeReg != null) {
            for (var u : coalescentEdges.get(freezeReg)) {
                coalescentEdges.get(u).remove(freezeReg);
            }
            coalescentReg.remove(freezeReg);
            uncoloredReg.add(freezeReg);
            return true;
        }
        return false;
    }

    /**
     * 根据未分配度数和spill代价衡量spill对象的选择
     * @param reg 待spill寄存器
     * @return 代价估计值
     */
    private double costFunction(Register reg) {
        if (!spillCost.containsKey(reg)) {
            return inf * 2;
        }
        double degree = interferenceEdges.get(reg).size();
        return spillCost.get(reg) / degree;
    }

    /**
     * 计算寄存器加载或存储到栈空间时的生存点
     * @param tmpReg 需要构建生存点的寄存器
     * @param last 产生寄存器值的指令
     * @param next 使用寄存器值的指令
     */
    void workOnRegisterValue(Register tmpReg, AsmInstruction last, AsmInstruction next) {
        LifeTimeIndex lastIndex = LifeTimeIndex.getInstOut(lifeTimeController, last);
        lifeTimeController.insertLifeTimePoint(tmpReg, LifeTimePoint.getDef(lastIndex));
        LifeTimeIndex nextIndex = LifeTimeIndex.getInstIn(lifeTimeController, next);
        lifeTimeController.insertLifeTimePoint(tmpReg, LifeTimePoint.getUse(nextIndex));
    }

    /**
     * 执行spill操作的过程，包含重建指令列表和添加生存点两个部分
     * @param spilledRegs 被spill的寄存器存储的栈空间位置
     */
    private void practiseSpill(Map<Register, StackVar> spilledRegs, List<Register> registers) {
        for (var reg : spilledRegs.keySet()) {
            if (!reg.isVirtual()) {
                throw new RuntimeException("spilled physical register");
            }
        }
        registers.removeAll(spilledRegs.keySet());
        List<AsmInstruction> spilledInstrList = new ArrayList<>();
        //IntRegister addressReg = function.getRegisterAllocator().allocateInt();
        for (var inst : instList) {
            for (var reg : AsmInstructions.getReadRegSet((inst))) {
                if (spilledRegs.containsKey(reg)) {
                    var rLoad = function.getRegisterAllocator().allocate(reg);
                    var tmpl = loadFromStackVar(rLoad, spilledRegs.get(reg), addressReg);
                    workOnRegisterValue(rLoad, tmpl.get(tmpl.size() - 1), inst);
                    for (int i : AsmInstructions.getReadVRegId(inst)) {
                        if (inst.getOperand(i) instanceof RegisterReplaceable rp && rp.getRegister().equals(reg)) {
                            inst.setOperand(i, rp.withRegister(rLoad));
                        }
                    }
                    spilledInstrList.addAll(tmpl);
                    registers.add(rLoad);
                }
            }
            spilledInstrList.add(inst);
            for (var reg : AsmInstructions.getWriteRegSet(inst)) {
                if (spilledRegs.containsKey(reg)) {
                    var rStore = function.getRegisterAllocator().allocate(reg);
                    var tmpl = saveToStackVar(rStore, spilledRegs.get(reg), addressReg);
                    workOnRegisterValue(rStore, inst, tmpl.get(tmpl.size() - 1));
                    for (int i : AsmInstructions.getWriteVRegId(inst)) {
                        if (inst.getOperand(i) instanceof RegisterReplaceable rp && rp.getRegister().equals(reg)) {
                            inst.setOperand(i, rp.withRegister(rStore));
                        }
                    }
                    spilledInstrList.addAll(tmpl);
                    registers.add(rStore);
                }
            }
        }
        instList = spilledInstrList;
        for (var reg : spilledRegs.keySet()) {
            lifeTimeController.removeReg(reg);
        }
        lifeTimeController.rebuildLifeTimeInterval(instList);
    }

    private Register getSpillDest() {
        Register dest = null;
        for (var reg : uncoloredReg) {
            if (dest == null || costFunction(reg) < costFunction(dest)) {
                dest = reg;
            }
        }
        if (dest == null) {
            throw new RuntimeException("no uncolored register to spill");
        }
        return dest;
    }

    /**
     * 执行spill操作，从未着色寄存器中找出代价最小的寄存器进行spill
     */
    private void spill(List<Register> registers) {
        List<Register> spilledList = new ArrayList<>();
        while (uncoloredReg.size() > 0) {
            Queue<Register> queue = new ArrayDeque<>();
            var dest = getSpillDest();
            spilledList.add(dest);
            queue.add(dest);
            uncoloredReg.remove(dest);
            while (!queue.isEmpty()) {
                var u = queue.remove();
                for (var v : Set.copyOf(interferenceEdges.get(u))) {
                    removeEdge(u, v);
                    if (interferenceEdges.get(v).size() < physicRegisters.size() && uncoloredReg.contains(v)) {
                        queue.add(v);
                        uncoloredReg.remove(v);
                    }
                }
            }
        }
        buildGraph(true, spilledList);
        Set<StackVar> savedSet = new HashSet<>();
        Map<Register, StackVar> spilledRegSaved = new HashMap<>();
        for (var u : spilledList) {
            Set<StackVar> occupied = new HashSet<>();
            for (var v : interferenceEdges.get(u)) {
                if (spilledRegSaved.containsKey(v)) {
                    occupied.add(spilledRegSaved.get(v));
                }
            }
            for (var stk : savedSet) {
                if (!occupied.contains(stk)) {
                    spilledRegSaved.put(u, stk);
                    break;
                }
            }
            if (!spilledRegSaved.containsKey(u)) {
                var save = stackPool.pop();
                spilledRegSaved.put(u, save);
                savedSet.add(save);
            }
        }
        //System.out.println("spilled size = " + spilledRegSaved.size());
        practiseSpill(spilledRegSaved, registers);
    }

    /**
     * 为registers列表中的每个虚拟寄存器分配物理寄存器，当发生一次溢出后即停止进行合并过程
     * @param instructionList 分配前的指令列表
     * @return 分配后的指令列表
     */
    public List<AsmInstruction> allocatePhysicalRegisters(List<AsmInstruction> instructionList, List<Register> registers) {
        /*if (debug) {
            debug_registers = registers;
        }*/
        instList = instructionList;
        getRegisterCost(registers);
        buildGraph(false, registers);
        stack.clear();

        //coalesce(registers);
        while (!color()) {
            if (!coalesce(registers)) {
                if (!freeze()) {
                    spill(registers);
                    buildGraph(false, registers);
                    stack.clear();
                }
            }
        }

        colorToRegisterMap(registers);
        return instList;
    }

    List<AsmInstruction> replacePhysicRegisters(List<AsmInstruction> instructionList) {
        Map<Register, StackVar> regSavedLoc = new HashMap<>();
        intervals.clear();
        for (var reg : physicRegisterMap.values()) {
            updateRegisterPreserve(reg);
        }
        for (var reg : physicRegisterMap.keySet()) {
            intervals.addAll(lifeTimeController.getInterval(reg));
        }
        Collections.sort(intervals);
        int intervalId = 0;
        Set<LifeTimeInterval> activeSet = new HashSet<>();

        Map<LifeTimeIndex, List<AsmInstruction>> newInstList = new HashMap<>();
        Map<LifeTimeIndex, List<Pair<Register, LifeTimePoint>>> pointsOnIndex = new HashMap<>();
        Map<AsmInstruction, LifeTimeIndex> instIn = new HashMap<>(), instOut = new HashMap<>();
        for (var inst : instructionList) {
            instIn.put(inst, LifeTimeIndex.getInstIn(lifeTimeController, inst));
            instOut.put(inst, LifeTimeIndex.getInstOut(lifeTimeController, inst));
            newInstList.put(instIn.get(inst), new ArrayList<>());
            newInstList.put(instOut.get(inst), new ArrayList<>());
            pointsOnIndex.put(instIn.get(inst), new ArrayList<>());
            pointsOnIndex.put(instOut.get(inst), new ArrayList<>());
        }

        Map<Register, LifeTimeIndex> defOnce = new HashMap<>();
        for (var reg : physicRegisterMap.keySet()) {
            if (!reg.isVirtual()) {
                defOnce.put(reg, null);
            }
            for (var p : lifeTimeController.getPoints(reg)) {
                var inst = p.getIndex().getSourceInst();
                var pReg = physicRegisterMap.get(reg);
                if (p.getIndex().isIn()) {
                    pointsOnIndex.get(instIn.get(inst)).add(new Pair<>(pReg, p));
                } else {
                    pointsOnIndex.get(instOut.get(inst)).add(new Pair<>(pReg, p));
                }
                if (p.isDef() && p.isTruePoint()) {
                    if (defOnce.containsKey(pReg)) {
                        defOnce.put(pReg, null);
                    } else {
                        defOnce.put(pReg, p.getIndex());
                    }
                }
            }
        }

        Map<Register, LifeTimeIndex> lastDef = new HashMap<>();
        Map<Register, StackVar> preserved = new HashMap<>();
        Map<Register, LifeTimeIndex> lastPreservedIn = new HashMap<>();

        for (var inst : instructionList) {
            LifeTimeIndex inIndex = LifeTimeIndex.getInstIn(lifeTimeController, inst);
            activeSet.removeIf((interval) -> interval.range.b.compareTo(inIndex) <= 0);

            for (var p : pointsOnIndex.get(instIn.get(inst))) {
                var pReg = p.a;
                var point = p.b;
                if (point.isUse() && preserved.containsKey(pReg)) {
                    var tmpl = loadFromStackVar(pReg, preserved.get(pReg), addressReg);
                    newInstList.get(instIn.get(inst)).addAll(tmpl);
                    preserved.remove(pReg);
                }
            }

            for (int i = 1; i <= 3; i++) {
                if (inst.getOperand(i) instanceof RegisterReplaceable rp && rp.getRegister().isVirtual()) {
                    Register physicRegister = physicRegisterMap.get(rp.getRegister());
                    inst.setOperand(i, rp.withRegister(physicRegister));
                }
            }

            if (inst instanceof AsmCall) {
                for (var interval : activeSet) {
                    var physicReg = physicRegisterMap.get(interval.reg);
                    if (!Registers.isPreservedAcrossCalls(physicReg) && !preserved.containsKey(physicReg)) {
                        if (!regSavedLoc.containsKey(physicReg)) {
                            StackVar stk = stackPool.pop();
                            regSavedLoc.put(physicReg, stk);
                        }
                        preserved.put(physicReg, regSavedLoc.get(physicReg));
                        if (!lastDef.get(physicReg).equals(lastPreservedIn.get(physicReg))) {
                            var tmpl = saveToStackVar(physicReg, regSavedLoc.get(physicReg), addressReg);
                            newInstList.get(lastDef.get(physicReg)).addAll(tmpl);
                            lastPreservedIn.put(physicReg, lastDef.get(physicReg));
                        }
                    }
                }
            }


            LifeTimeIndex outIndex = LifeTimeIndex.getInstOut(lifeTimeController, inst);
            while (intervalId < intervals.size() && intervals.get(intervalId).range.a.compareTo(outIndex) <= 0) {
                activeSet.add(intervals.get(intervalId));
                intervalId += 1;
            }

            for (var p : pointsOnIndex.get(instOut.get(inst))) {
                var reg = p.a;
                var point = p.b;
                if (point.isDef() && (defOnce.get(reg) == null || point.isTruePoint())) {
                    lastDef.put(reg, instOut.get(inst));
                    preserved.remove(reg);
                }
            }
        }

        List<AsmInstruction> instList = new ArrayList<>();
        for (var inst : instructionList) {
            instList.addAll(newInstList.get(instIn.get(inst)));
            instList.add(inst);
            instList.addAll(newInstList.get(instOut.get(inst)));
        }
        return instList;
    }

    @Override
    public List<AsmInstruction> work(List<AsmInstruction> instructionList) {
        List<Register> intRegList = new ArrayList<>(), floatRegList = new ArrayList<>();
        List<Register> intPRegList = new ArrayList<>(), floatPRegList = new ArrayList<>();
        physicRegisterMap.clear();
        for (var reg : lifeTimeController.getRegKeySet()) {
            if (reg instanceof IntRegister) {
                intRegList.add(reg);
            } else {
                floatRegList.add(reg);
            }
        }
        //图着色分配器可以将非静态寄存器全部分配
        for (int i = 0; i <= 31; i++) {
            Register reg = IntRegister.getPhysical(i);
            if (!Registers.CONSTANT_REGISTERS.contains(reg)) {
                intPRegList.add(reg);
            }
            reg = FloatRegister.getPhysical(i);
            if (!Registers.CONSTANT_REGISTERS.contains(reg)) {
                floatPRegList.add(reg);
            }
        }

        //update : 保留了寄存器x5(t0)作为地址加载寄存器
        intPRegList.remove(addressReg);

        physicRegisters = intPRegList;
        instructionList = allocatePhysicalRegisters(instructionList, intRegList);
        physicRegisters = floatPRegList;
        instructionList = allocatePhysicalRegisters(instructionList, floatRegList);
        instructionList = replacePhysicRegisters(instructionList);
        return instructionList;
    }
}
