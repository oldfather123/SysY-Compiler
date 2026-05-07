package backend.RegAllocate;

import backend.component.RiscvBlock;
import backend.component.RiscvFunction;
import backend.component.RiscvInstr;
import backend.instruction.*;
import backend.operand.*;
import backend.opt.LiveInfo;
import backend.component.RiscvModule;
import org.antlr.v4.runtime.misc.Pair;
import tools.DoubleNode;
import tools.IrRegDispatcher;

import javax.imageio.plugins.tiff.TIFFDirectory;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Stack;

public class RegAllocator {
    private RiscvVirReg.RegType currentType;
    //    private RiscvFunction functionNow;暂时不用
    private final RiscvModule riscvModule;

    private final int INF = 0x7fffffff;

    //目前默认不动zero,ra,sp,gp,tp,因此27种
    private final int K = 27;

    private LinkedHashMap<RiscvBlock, LiveInfo> liveInfoMap;
    //保存每一个块的liveInfo信息

    /**
     * 虎书中的preColored和function.getAllVirRegUsed()是在创建寄存器的初始化了,不用单开数据结构
     * 同时colored代表寄存器已成功着色,
     */
    private LinkedHashSet<RiscvOperand> simplifyWorklist;
    /**
     * 低度数的传送无关的节点表
     */
    private LinkedHashSet<RiscvInstr> worklistMoves;
    private LinkedHashSet<RiscvInstr> activeMoves;
    private LinkedHashSet<RiscvInstr> coalescedMoves;
    private LinkedHashSet<RiscvInstr> constrainedMoves;
    private LinkedHashSet<RiscvInstr> frozenMoves;

    private LinkedHashSet<RiscvReg> coloredNodes;

    private LinkedHashSet<RiscvOperand> spillWorklist;
    /**
     * 低度数的传送有关的节点表
     */

    private LinkedHashSet<RiscvOperand> spilledNodes;
    /**
     * 本轮要溢出的结点，初始为空
     */
    private LinkedHashSet<RiscvOperand> coalescedNodes;
    /**
     * 已合并的节点的集合，比如将 u 合并到 v，那么将 u 加入这里，然后 v 加入其他集合
     */

    private LinkedHashMap<RiscvOperand, LinkedHashSet<RiscvMv>> moveList;

    private LinkedHashMap<RiscvOperand, LinkedHashSet<RiscvOperand>> adjList;
    /**
     *
     */
    private LinkedHashMap<RiscvOperand, Integer> degree;
    private LinkedHashSet<Pair<RiscvReg, RiscvReg>> adjSet;
    //无向图,双向维护
    private LinkedHashMap<RiscvOperand, RiscvOperand> alias;
    private LinkedHashSet<RiscvOperand> freezeWorklist;

    private Stack<RiscvOperand> selectStack;

    private LinkedHashMap<RiscvReg, Integer> color;
    private LinkedHashMap<RiscvVirReg, LinkedHashSet<RiscvReg>> crossConflicts;

    //
    public RegAllocator(RiscvModule riscvModule) {
        this.riscvModule = riscvModule;
    }

    public void init(RiscvFunction function) {
        crossConflicts = new LinkedHashMap<>();
        constrainedMoves = new LinkedHashSet<>();
        selectStack = new Stack<>();
        liveInfoMap = LiveInfo.liveInfoAnalysis(function);
        simplifyWorklist = new LinkedHashSet<>();
        worklistMoves = new LinkedHashSet<>();
        spillWorklist = new LinkedHashSet<>();
        spilledNodes = new LinkedHashSet<>();
        coalescedNodes = new LinkedHashSet<>();
        moveList = new LinkedHashMap<>();
        adjList = new LinkedHashMap<>();
        adjSet = new LinkedHashSet<>();
        alias = new LinkedHashMap<>();
        freezeWorklist = new LinkedHashSet<>();
        degree = new LinkedHashMap<>();
        activeMoves = new LinkedHashSet<>();
        coalescedMoves = new LinkedHashSet<>();
        frozenMoves = new LinkedHashSet<>();

        coloredNodes = new LinkedHashSet<>();
        color = new LinkedHashMap<>();
        coloredNodes.addAll(RiscvCPUReg.getAllReg().values());
        coloredNodes.addAll(RiscvFPUReg.getAllReg().values());
        RiscvCPUReg.getRiscvCPUReg(0);
        RiscvFPUReg.getRiscvFPUReg(0);
        RiscvCPUReg.getAllReg().values().forEach(reg -> {
            color.put(reg, reg.getIndex());
            degree.put(reg, INF);
        });
        RiscvFPUReg.getAllReg().values().forEach(reg -> {
            color.put(reg, reg.getIndex());
            degree.put(reg, INF);
        });
    }

    public void makeWorkList(RiscvFunction function) {
        for (var block : function.getBlocks()) {
            for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getTail();
                 insNode != null; insNode = insNode.getPred()) {
                RiscvInstr ins = insNode.getContent();
                if (ins instanceof RiscvMv) {
                    if (!adjList.containsKey(ins.getDefReg())) {
                        adjList.put(ins.getDefReg(), new LinkedHashSet<>());
                    }
                    if (adjList.get(ins.getDefReg()).contains(ins.getOperands().get(0))) {
                        worklistMoves.remove(ins);
                        constrainedMoves.add(ins);
                    }

                }
            }
        }
        //已经复制，修改此列表不影响第二次获取
        for (var n : function.getAllVirRegUsed()) {
            assert n instanceof RiscvVirReg;
            if (((RiscvVirReg) n).regType != currentType) {
                continue;
            }
            if (degree.getOrDefault(n, 0) >= K) {
                spillWorklist.add(n);
            } else if (moveRelated(n)) {
                freezeWorklist.add(n);
            } else {
                simplifyWorklist.add(n);
            }
        }
    }

    public ArrayList<RiscvPhyReg> getCallDefs() {
        if (currentType == RiscvVirReg.RegType.intType) {
            ArrayList<Integer> list =
                new ArrayList<>(
                    Arrays.asList(1, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31));
            ArrayList<Integer> list2 =
                new ArrayList<>(
                    Arrays.asList(0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29,
                        30, 31));
            ArrayList<RiscvPhyReg> ret = new ArrayList<>();
            list2.forEach(i -> ret.add(RiscvFPUReg.getRiscvFPUReg(i)));
            list.forEach(i -> ret.add(RiscvCPUReg.getRiscvCPUReg(i)));
            return ret;
        } else {
            ArrayList<Integer> list =
                new ArrayList<>(
                    Arrays.asList(0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29,
                        30, 31));
            ArrayList<RiscvPhyReg> ret = new ArrayList<>();
            list.forEach(i -> ret.add(RiscvFPUReg.getRiscvFPUReg(i)));
            return ret;
        }
    }


    public void buildGraph(RiscvFunction function) {
        ArrayList<RiscvBlock> blocks = function.getBlocks();
        boolean isFirstBlock = true;
        for (RiscvBlock block : blocks) {
//            printtime("buildgraph_visitblock");
            LinkedHashSet<RiscvReg> liveOut = liveInfoMap.get(block).getLiveOut();
            /**
             * 这里之所以需要专门提出 liveOut ，是因为他在逆序遍历指令的过程中，
             * 从块的LiveOut，逐条指令计算该指令的 LiveOut
             * 遍历到该指令时，里面装的就是该指令的 LiveOut
             */
            for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getTail();
                 insNode != null; insNode = insNode.getPred()) {
                RiscvInstr ins = insNode.getContent();
//                System.out.println(ins);
//                printtime("buildgraph_visitnode");
                if (ins instanceof RiscvMv) {
                    //TODO 这一句的原理是什么？
                    liveOut.remove(ins.getOperands().get(0));

                    if (!moveList.containsKey(ins.getOperands().get(0))) {
                        moveList.put(ins.getOperands().get(0), new LinkedHashSet<>());
                    }
                    moveList.get(ins.getOperands().get(0)).add((RiscvMv) ins);

                    if (!moveList.containsKey(ins.getDefReg())) {
                        moveList.put(ins.getDefReg(), new LinkedHashSet<>());
                    }
                    moveList.get(ins.getDefReg()).add((RiscvMv) ins);
                    worklistMoves.add((RiscvMv) ins);
                }
                //TODO 如果defReg能有多个 这里需要修改
                ArrayList<RiscvReg> defs = new ArrayList<>();
                if (ins.getDefReg() != null) {
                    defs.add(ins.getDefReg());
                    if (ins instanceof RiscvCall) {
                        defs.addAll(getCallDefs());
                    }
                    liveOut.addAll(defs);
                    for (var def : defs) {
                        for (RiscvReg reg : liveOut) {
                            addEdge(reg, def);
                        }
                    }
                    if(!(ins instanceof RiscvMv)){
                        liveOut.removeAll(defs);
                    }else{
                        if(!(isFirstBlock && ins.getDefReg() instanceof RiscvPhyReg)){
                            liveOut.removeAll(defs);
                        }
                    }
                }
//                printtime("buildgraph_visitnode-def");
                for (RiscvOperand operand : ins.getOperands()) {
                    if (operand instanceof RiscvReg) {
                        liveOut.add((RiscvReg) operand);
                    }
                }
                if (ins instanceof RiscvCall) {
                    liveOut.addAll(((RiscvCall) ins).usedRegs);
                }
//                printtime("buildgraph_visitnode-liveout");
            }
            isFirstBlock = false;
        }
    }

    public static long time0 = 0;

    public static double gettime() {
        long timenow = System.currentTimeMillis();
        double returntime = (timenow - time0) / 1000.0;
        time0 = timenow;
        return returntime;
    }

    public static void printtime(String str) {
        System.out.printf(String.format("%s time: %f %n", str, gettime()));
    }


    public void generateProtectionMove(DoubleNode<RiscvInstr> head, DoubleNode<RiscvInstr> tail,
                                       RiscvFunction function) {
        var dispatcher = new IrRegDispatcher(20000);
        if (currentType == RiscvVirReg.RegType.intType) {
            ArrayList<Integer> list =
                new ArrayList<>(Arrays.asList(8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27));
            for (int index : list) {
                var vrreg = new RiscvVirReg(dispatcher, RiscvVirReg.RegType.intType,
                    function);
                head.getParentList().insertBeforeNode(head, new DoubleNode<>(
                    new RiscvMv(RiscvCPUReg.getRiscvCPUReg(index), vrreg)
                ));
                tail.getParentList().insertBeforeNode(tail, new DoubleNode<>(
                    new RiscvMv(vrreg, RiscvCPUReg.getRiscvCPUReg(index))
                ));
            }
        } else {
            ArrayList<Integer> list =
                new ArrayList<>(Arrays.asList(8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27));
            for (int index : list) {
                var vrreg = new RiscvVirReg(dispatcher, RiscvVirReg.RegType.floatType, function);
                head.getParentList().insertBeforeNode(head, new DoubleNode<>(
                    new RiscvFmv(RiscvFPUReg.getRiscvFPUReg(index), vrreg)
                ));
                tail.getParentList().insertBeforeNode(tail, new DoubleNode<>(
                    new RiscvFmv(vrreg, RiscvFPUReg.getRiscvFPUReg(index))
                ));
            }
        }
//        var dispatcher = new IrRegDispatcher(20000);
//        ArrayList<Integer> list2 =
//            new ArrayList<>(Arrays.asList(8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27));
//        ArrayList<Integer> list =
//            new ArrayList<>(Arrays.asList(8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27));
//        if(currentType == int)
//        for (int index : list2) {
//            var vrreg = new RiscvVirReg(dispatcher, RiscvVirReg.RegType.floatType, function);
//            head.getParentList().insertBeforeNode(head, new DoubleNode<>(
//                new RiscvFmv(RiscvFPUReg.getRiscvFPUReg(index), vrreg)
//            ));
//            DoubleNode<RiscvInstr> node1 = new DoubleNode<>(
//                new RiscvFmv(vrreg, RiscvFPUReg.getRiscvFPUReg(index)));
//            tail.getParentList().insertBeforeNode(tail, node1
//            );
//            tail = node1;
//        }
//
//            for (int index : list) {
//                var vrreg = new RiscvVirReg(dispatcher, RiscvVirReg.RegType.intType,
//                    function);
//                head.getParentList().insertBeforeNode(head, new DoubleNode<>(
//                    new RiscvMv(RiscvCPUReg.getRiscvCPUReg(index), vrreg)
//                ));
//                tail.getParentList().insertBeforeNode(tail, new DoubleNode<>(
//                    new RiscvMv(vrreg, RiscvCPUReg.getRiscvCPUReg(index))
//                ));
//            }

    }

    public void Process() {
        gettime();
        for (RiscvFunction function : riscvModule.getFunctions().values()) {
            if (function.getBlocks().size() == 0) {
                continue;
            }
            var firstnode = function.getBlocks().get(0).getRiscvInstrs().getHead();
            DoubleNode<RiscvInstr> lastnode = null;
            for (var block : function.getBlocks()) {
                for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getTail();
                     insNode != null; insNode = insNode.getPred()) {
                    var instr = insNode.getContent();
                    if (instr instanceof RiscvJr) {
                        lastnode = insNode;
                    }
                }
            }

            currentType = null;
            while (true) {
                printtime("start");
                System.out.printf("run!");
                if (currentType == null) {
                    currentType = RiscvVirReg.RegType.floatType;
                } else if (currentType == RiscvVirReg.RegType.floatType) {
                    currentType = RiscvVirReg.RegType.intType;
                } else {
                    break;
                }
                if (!function.getName().equals("main")) {
                    generateProtectionMove(firstnode, lastnode, function);
                }
                boolean hasTailCall = true;
                while (hasTailCall) {
                    init(function);
                    printtime("intail");
                    hasTailCall = false;
                    printtime("finish init1 pass");
                    buildGraph(function);
                    printtime("finish init2 pass");
                    makeWorkList(function);
                    printtime("finish init3 pass");
                    System.out.printf("节点数%d\n", adjSet.size() / 2);
                    do {
                        if (!simplifyWorklist.isEmpty()) {
                            simplify();
                        } else if (!worklistMoves.isEmpty()) {
                            coalesce();
                        } else if (!freezeWorklist.isEmpty()) {
                            freeze();
                        } else if (!spillWorklist.isEmpty()) {
                            selectSpill();
                        }
//                        printtime("finish one pass");
                    } while (!(simplifyWorklist.isEmpty() && worklistMoves.isEmpty() &&
                        freezeWorklist.isEmpty() && spillWorklist.isEmpty()));
//                    printtime("finish once");
                    assignColor(function);
                    if (!spilledNodes.isEmpty()) {
                        rewriteProgram(function);
                        hasTailCall = true;
                    }
                    System.out.println("finished alloca pass");
                }
                for (var block : function.getBlocks()) {
                    for (DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                         insNode != null; insNode = insNode.getSucc()) {
                        var instr = insNode.getContent();
                        if (instr.getDefReg() instanceof RiscvVirReg &&
                            color.containsKey(instr.getDefReg())) {
                            if (((RiscvVirReg) instr.getDefReg()).regType ==
                                RiscvVirReg.RegType.intType &&
                                currentType == RiscvVirReg.RegType.intType) {
                                instr.replaceDefReg(
                                    RiscvCPUReg.getRiscvCPUReg(color.get(instr.getDefReg())),
                                    insNode);
                            } else if (((RiscvVirReg) instr.getDefReg()).regType ==
                                RiscvVirReg.RegType.floatType &&
                                currentType == RiscvVirReg.RegType.floatType) {
                                instr.replaceDefReg(
                                    RiscvFPUReg.getRiscvFPUReg(color.get(instr.getDefReg())),
                                    insNode);
                            }
                        }
                        LinkedHashMap<RiscvOperand, RiscvOperand> replace = new LinkedHashMap<>();
                        for (var ope : instr.getOperands()) {
                            if (ope instanceof RiscvVirReg && color.containsKey(ope)) {
                                if (((RiscvVirReg) ope).regType ==
                                    RiscvVirReg.RegType.intType &&
                                    currentType == RiscvVirReg.RegType.intType) {
                                    replace.put((RiscvReg) ope,
                                        RiscvCPUReg.getRiscvCPUReg(color.get(ope)));
                                } else if (((RiscvVirReg) ope).regType ==
                                    RiscvVirReg.RegType.floatType &&
                                    currentType == RiscvVirReg.RegType.floatType) {
                                    replace.put((RiscvReg) ope,
                                        RiscvFPUReg.getRiscvFPUReg(color.get(ope)));
                                }
                            }
                        }
                        for (var key : replace.keySet()) {
                            instr.replaceOperands((RiscvReg) key, (RiscvReg) replace.get(key),
                                insNode);
                        }
                    }
                }
            }
        }
        return;
    }

    static int edgecount = 0;

    public void addEdge(RiscvReg reg1, RiscvReg reg2) {
//        printtime("buildgraph_addEdgebegin");
        //需要确保reg1 reg2是同一类型的寄存器（float或者int）
        boolean type1 = reg1 instanceof RiscvCPUReg ||
            (reg1 instanceof RiscvVirReg
                && ((RiscvVirReg) reg1).regType == RiscvVirReg.RegType.intType);
        boolean type2 = reg2 instanceof RiscvCPUReg ||
            (reg2 instanceof RiscvVirReg
                && ((RiscvVirReg) reg2).regType == RiscvVirReg.RegType.intType);
        if (type2 == type1 && !adjSet.contains(new Pair<>(reg1, reg2)) && reg1 != reg2) {
            if (!((type1 && currentType == RiscvVirReg.RegType.intType) ||
                (!type1) && currentType == RiscvVirReg.RegType.floatType)) {
                return;
            }
            edgecount++;
            if (edgecount % 10000 == 0) {
                System.out.println(edgecount);
            }
            adjSet.add(new Pair<>(reg1, reg2));
            adjSet.add(new Pair<>(reg2, reg1));
            if (!reg1.isPreColored()) {
                if (!adjList.containsKey(reg1)) {
                    adjList.put(reg1, new LinkedHashSet<>());
                }
                adjList.get(reg1).add(reg2);
                if (!degree.containsKey(reg1)) {
                    degree.put(reg1, 1);
                } else {
                    degree.put(reg1, degree.get(reg1) + 1);
                }
            }
            if (!reg2.isPreColored()) {
                if (!adjList.containsKey(reg2)) {
                    adjList.put(reg2, new LinkedHashSet<>());
                }
                adjList.get(reg2).add(reg1);
                if (!degree.containsKey(reg2)) {
                    degree.put(reg2, 1);
                } else {
                    degree.put(reg2, degree.get(reg2) + 1);
                }
            }
        } else if (type2 != type1 && currentType == RiscvVirReg.RegType.intType) {
            //reg2是int，reg1不是，且reg2是虚拟寄存器，则记录
            if (type2 && reg2 instanceof RiscvVirReg) {
                assert reg1 instanceof RiscvFPUReg;
                if (!crossConflicts.containsKey(reg2)) {
                    crossConflicts.put((RiscvVirReg) reg2, new LinkedHashSet<>());
                }
                crossConflicts.get(reg2).add(reg1);
            } else if (type1 && reg1 instanceof RiscvVirReg) {
                assert reg2 instanceof RiscvFPUReg;
                if (!crossConflicts.containsKey(reg1)) {
                    crossConflicts.put((RiscvVirReg) reg1, new LinkedHashSet<>());
                }
                crossConflicts.get(reg1).add(reg2);
            }
        }
//        printtime("buildgraph_addEdge End");
    }


    LinkedHashSet<RiscvInstr> nodeMoves(RiscvOperand reg) {
        LinkedHashSet<RiscvInstr> result = null;
        if (moveList.containsKey(reg)) {
            result = new LinkedHashSet<>(moveList.get(reg));
        } else {
            result = new LinkedHashSet<>();
        }
        LinkedHashSet<RiscvInstr> tmpset = new LinkedHashSet<>(activeMoves);
        tmpset.addAll(worklistMoves);
        result.retainAll(tmpset);
        return result;
    }

    public boolean moveRelated(RiscvOperand reg) {
        return !nodeMoves(reg).isEmpty();
    }


    public LinkedHashSet<RiscvOperand> adjacent(RiscvOperand reg) {
        LinkedHashSet<RiscvOperand> result = new LinkedHashSet<>();
        if (adjList.containsKey(reg)) {
            result.addAll(adjList.get(reg));
        }
        selectStack.forEach(result::remove);
        result.removeAll(coalescedNodes);
        return result;
    }

    public void simplify() {
        var n = simplifyWorklist.iterator().next();
        simplifyWorklist.remove(n);
        selectStack.push(n);
        for (var m : adjacent((RiscvReg) n)) {
            decrementDegree(m);
        }
    }

    public void decrementDegree(RiscvOperand m) {
        var d = degree.get(m);
        degree.put(m, d - 1);
        if (d == K) {
            var tmp = adjacent(m);
            tmp.add(m);
            spillWorklist.remove(m);
            if (moveRelated(m)) {
                freezeWorklist.add(m);
            } else {
                simplifyWorklist.add(m);
            }
        }
    }

    public void enableMoves(RiscvOperand node) {
        enableMoves(new LinkedHashSet<>(Collections.singletonList(node)));
    }

    public void enableMoves(LinkedHashSet<RiscvOperand> nodes) {
        for (var n : nodes) {
            for (var m : nodeMoves(n)) {
                if (activeMoves.contains(m)) {
                    activeMoves.remove(m);
                    worklistMoves.add(m);
                }
            }
        }
    }

    public void coalesce() {
        var m = worklistMoves.iterator().next();
        var x = getAlias(m.getDefReg());
        var y = getAlias((RiscvReg) m.getOperands().get(0));
        Pair<RiscvReg, RiscvReg> uv;
        if (y.isPreColored()) {
            uv = new Pair<>(y, x);
        } else {
            uv = new Pair<>(x, y);
        }
        worklistMoves.remove(m);
        if (uv.a == uv.b) {
            coalescedMoves.add(m);
            addWorkList(uv.a);
        } else if (uv.b.isPreColored() || adjSet.contains(uv)) {
            constrainedMoves.add(m);
            addWorkList(uv.a);
            addWorkList(uv.b);
        } else {
            boolean flag = true;
            if (uv.a.isPreColored()) {
                for (var t : adjacent(uv.b)) {
                    flag = flag && OK((RiscvReg) t, uv.a);
                }
            } else if (!uv.a.isPreColored()) {// TODO: 2023/8/4
                LinkedHashSet<RiscvOperand> computedNodes = new LinkedHashSet<>();
                computedNodes.addAll(adjacent(uv.a));
                computedNodes.addAll(adjacent(uv.b));
                flag = conservative(computedNodes);
            }
            if (flag) {
                coalescedMoves.add(m);
                combine(uv.a, uv.b);
                addWorkList(uv.a);
            } else {
                activeMoves.add(m);
            }
        }
    }

    public void combine(RiscvReg u, RiscvReg v) {
        if (freezeWorklist.contains(v)) {
            freezeWorklist.remove(v);
        } else {
            spillWorklist.remove(v);
        }
        coalescedNodes.add(v);
        alias.put(v, u);
        moveList.get(u).addAll(moveList.get(v));
        enableMoves(v);
        for (var t : adjList.getOrDefault(v, new LinkedHashSet<>())) {
            addEdge((RiscvReg) t, u);
            decrementDegree(t);
        }
        if (degree.getOrDefault(u, 0) >= K && freezeWorklist.contains(u)) {
            freezeWorklist.remove(u);
            spillWorklist.add(u);
        }
    }

    public boolean conservative(LinkedHashSet<RiscvOperand> nodes) {
        int k = 0;
        for (var n : nodes) {
            if (degree.getOrDefault(n, 0) >= K) {
                k += 1;
            }
        }
        return k < K;
    }

    public boolean OK(RiscvReg t, RiscvReg r) {
        return degree.getOrDefault(t, 0) < K || t.isPreColored() ||
            adjSet.contains(new Pair<>(t, r));
    }


    public RiscvReg getAlias(RiscvReg n) {
        if (coalescedNodes.contains(n)) {
            return getAlias((RiscvReg) alias.get(n));
        } else {
            return n;
        }
    }

    public void addWorkList(RiscvReg u) {
        if (!u.isPreColored() && !moveRelated(u) && (degree.getOrDefault(u, 0) < K)) {
            freezeWorklist.remove(u);
            simplifyWorklist.add(u);
        }
    }

    public void freeze() {
        var u = freezeWorklist.iterator().next();
        freezeWorklist.remove(u);
        simplifyWorklist.add(u);
        freezeMoves((RiscvReg) u);
    }

    public void freezeMoves(RiscvReg u) {
        for (var m : nodeMoves(u)) {
            var x = m.getDefReg();
            var y = (RiscvReg) m.getOperands().get(0);
            RiscvReg v;
            if (getAlias(y) == getAlias(u)) {
                v = getAlias(x);
            } else {
                v = getAlias(y);
            }
            activeMoves.remove(m);
            frozenMoves.add(m);
            if (nodeMoves(v).isEmpty() && degree.getOrDefault(v, 0) < K) {
                freezeWorklist.remove(v);
                simplifyWorklist.add(v);
            }
        }
    }

    private double getprioForSpill(RiscvReg r) {
        return (double) r.loopFactor / (double) degree.getOrDefault(r, 0);
    }

    public void selectSpill() {
        RiscvReg v = null;//启发式算法，o（n）内得到选择的溢出寄存器
        boolean first = true;
        for (var waitSpill : spillWorklist) {
            RiscvReg vcmp = (RiscvReg) waitSpill;
            if (first) {
                v = vcmp;
                first = false;
            } else {
                if (getprioForSpill(vcmp) < getprioForSpill(v)) {
                    v = vcmp;
                }
            }
        }
        spillWorklist.remove(v);
        simplifyWorklist.add(v);
        freezeMoves(v);
    }

    public LinkedHashSet<Integer> initOkColors() {
        LinkedHashSet<Integer> okColors = new LinkedHashSet<>();
        if (currentType == RiscvVirReg.RegType.intType) {
            for (int i = 0; i < 27; i++) {
                okColors.add(5 + i);
            }
        } else {
            for (int i = 0; i < 32; i++) {
                okColors.add(i);
            }
        }

        return okColors;
    }

    public LinkedHashSet<Integer> initOkColorsStoF() {
        LinkedHashSet<Integer> okColors = new LinkedHashSet<>();
        for (int i = 0; i < 32; i++) {
            okColors.add(i);
        }

        return okColors;
    }

    public void assignColor(RiscvFunction function) {
        while (!selectStack.isEmpty()) {
            var n = selectStack.pop();
            LinkedHashSet<Integer> okColors = initOkColors();
            if (!adjList.containsKey(n)) {
                adjList.put(n, new LinkedHashSet<>());
            }
            for (var w : adjList.get(n)) {
                if (getAlias((RiscvReg) w).isPreColored() ||
                    coloredNodes.contains(w)) {
                    okColors.remove(color.get(getAlias((RiscvReg) w)));
                }
            }
            if (okColors.isEmpty()) {
                spilledNodes.add(n);
            } else {
                coloredNodes.add((RiscvReg) n);
                var c = okColors.iterator().next();
                color.put((RiscvReg) n, c);
            }
        }
        for (var n : coalescedNodes) {
            color.put((RiscvReg) n, color.get(getAlias((RiscvReg) n)));
        }
    }

    public void rewriteProgram(RiscvFunction function) {
        LinkedHashSet<Integer> spilledStoF = new LinkedHashSet<>();
        for (RiscvOperand riscvOperand : spilledNodes) {
            assert riscvOperand instanceof RiscvVirReg;
            if (currentType == RiscvVirReg.RegType.intType && false) {
                assert ((RiscvVirReg) riscvOperand).regType == RiscvVirReg.RegType.intType;
                var okColor = initOkColorsStoF();
                okColor.removeAll(spilledStoF);
                if(crossConflicts.containsKey(riscvOperand)){
                    crossConflicts.get(riscvOperand).forEach(c->{
                        assert c instanceof RiscvFPUReg;
                        okColor.remove(((RiscvFPUReg) c).getIndex());
                    });
                }
                if(!okColor.isEmpty()){
                    var select = okColor.iterator().next();
                    spilledStoF.add(select);
                    var fpureg = RiscvFPUReg.getRiscvFPUReg(select);
                    var users = new ArrayList<DoubleNode<RiscvInstr>>(riscvOperand.getUser());
                    for(var user : users){
                        var ins = user.getContent();
                        if(ins.getDefReg() != null && ins.getDefReg().equals(riscvOperand)){
                            RiscvVirReg newDefReg = new RiscvVirReg(function.getIrRegDispatcher(),
                                ((RiscvVirReg) riscvOperand).regType, function);
                            DoubleNode<RiscvInstr> node1 = new DoubleNode<>(
                                new RiscvDXMV(newDefReg, fpureg)
                            );
                            user.getParentList().insertAfterNode(user, node1);
                            newDefReg.beUsed(node1);
                            ins.replaceDefReg(newDefReg,user);
                            function.updateAlluse(newDefReg, (RiscvVirReg) riscvOperand);
                        }
                        for (RiscvOperand operand : ins.getOperands()) {
                            if (operand.equals(riscvOperand)) {
                                RiscvVirReg newDefReg = new RiscvVirReg(function.getIrRegDispatcher(),
                                    ((RiscvVirReg) riscvOperand).regType, function);
                                DoubleNode<RiscvInstr> node1 = new DoubleNode<>(
                                    new RiscvDXMV(fpureg, newDefReg)
                                );
                                user.getParentList().insertBeforeNode(user, node1);
                                newDefReg.beUsed(node1);
                                ins.replaceOperands((RiscvReg) riscvOperand, newDefReg,user);
                                function.updateAlluse(newDefReg, (RiscvVirReg) riscvOperand);
                                break;
                            }
                        }
                    }
                    riscvOperand.getUser().clear();
                    continue;
                }
            }

            function.alloc8((RiscvVirReg) riscvOperand);
            ArrayList<DoubleNode<RiscvInstr>> users =
                new ArrayList<>(riscvOperand.getUser());
            for (DoubleNode<RiscvInstr> insNode : users) {
                //TODO：需要检查这里是否维护好了User关系
                RiscvInstr ins = insNode.getContent();
                if (ins.getDefReg() != null && ins.getDefReg().equals(riscvOperand)) {
                    RiscvVirReg newDefReg = new RiscvVirReg(function.getIrRegDispatcher(),
                        ((RiscvVirReg) riscvOperand).regType, function);
                    RiscvVirReg assistReg = new RiscvVirReg(function.getIrRegDispatcher(),
                        RiscvVirReg.RegType.intType, function);
                    function.reMap(newDefReg, (RiscvVirReg) riscvOperand);
                    int offset = function.getOffset(newDefReg);
                    DoubleNode<RiscvInstr> node1;
                    DoubleNode<RiscvInstr> node3;
                    if (-1 * offset < 2048 && -1 * offset >= -2048) {
                        DoubleNode<RiscvInstr> node2;
                        if (((RiscvVirReg) riscvOperand).regType ==
                            RiscvVirReg.RegType.floatType) {
                            node2 = new DoubleNode<>(
                                new RiscvFsd(newDefReg, new RiscvImm(-1 * offset),
                                    RiscvCPUReg.getRiscvCPUReg(2)));
                        } else {
                            node2 = new DoubleNode<>(
                                new RiscvSd(newDefReg, new RiscvImm(-1 * offset),
                                    RiscvCPUReg.getRiscvCPUReg(2)));
                        }
                        newDefReg.beUsed(node2);
                        insNode.getParentList().insertAfterNode(insNode, node2);
                        ins.replaceDefReg(newDefReg, insNode);
                    } else {
                        node1 = new DoubleNode<>(
                            new RiscvLi(new RiscvImm(-1 * offset), assistReg));
                        node3 = new DoubleNode<>(
                            new RiscvBinary(new ArrayList<>(Arrays.asList(assistReg,
                                RiscvCPUReg.getRiscvCPUReg(2))), assistReg,
                                RiscvBinary.RiscvBinaryType.add));
                        assistReg.beUsed(node3);
                        DoubleNode<RiscvInstr> node2;
                        if (((RiscvVirReg) riscvOperand).regType ==
                            RiscvVirReg.RegType.floatType) {
                            node2 = new DoubleNode<>(
                                new RiscvFsd(newDefReg, new RiscvImm(0), assistReg));
                        } else {
                            node2 = new DoubleNode<>(
                                new RiscvSd(newDefReg, new RiscvImm(0), assistReg));
                        }
                        newDefReg.beUsed(node2);
                        insNode.getParentList().insertAfterNode(insNode, node2);
                        insNode.getParentList().insertAfterNode(insNode, node3);
                        insNode.getParentList().insertAfterNode(insNode, node1);
                        ins.replaceDefReg(newDefReg, insNode);
                    }
                }
                //用于处理连续用到两次
                boolean flag = false;
                RiscvVirReg newDefReg = null;
                for (RiscvOperand operand : ins.getOperands()) {
                    if (operand.equals(riscvOperand)) {
                        if (!flag) {
                            flag = true;
                            if (((RiscvVirReg) riscvOperand).regType ==
                                RiscvVirReg.RegType.intType) {
                                newDefReg = new RiscvVirReg(function.getIrRegDispatcher(),
                                    ((RiscvVirReg) riscvOperand).regType, function);
                                function.reMap(newDefReg, (RiscvVirReg) riscvOperand);
                                int offset = function.getOffset(newDefReg);
                                DoubleNode<RiscvInstr> node1 = null;
                                DoubleNode<RiscvInstr> node3 = null;
                                if (offset * -1 < 2048 && offset * -1 >= -2048) {
                                    DoubleNode<RiscvInstr> node2 = new DoubleNode<>(
                                        new RiscvLd(new RiscvImm(-1 * offset),
                                            RiscvCPUReg.getRiscvCPUReg(2), newDefReg));
                                    newDefReg.beUsed(node2);
                                    insNode.getParentList().insertBeforeNode(insNode, node2);
                                    ins.replaceOperands((RiscvReg) riscvOperand, newDefReg,
                                        insNode);
                                } else {
                                    node1 = new DoubleNode<>(
                                        new RiscvLi(new RiscvImm(-1 * offset), newDefReg));
                                    insNode.getParentList().insertBeforeNode(insNode, node1);
                                    node3 = new DoubleNode<>(
                                        new RiscvBinary(new ArrayList<>(Arrays.asList(newDefReg,
                                            RiscvCPUReg.getRiscvCPUReg(2))), newDefReg,
                                            RiscvBinary.RiscvBinaryType.add));
                                    newDefReg.beUsed(node3);
                                    insNode.getParentList().insertBeforeNode(insNode, node3);
                                    DoubleNode<RiscvInstr> node2 = null;
                                    node2 = new DoubleNode<>(
                                        new RiscvLd(new RiscvImm(0), newDefReg, newDefReg));
                                    newDefReg.beUsed(node2);
                                    insNode.getParentList().insertBeforeNode(insNode, node2);
                                    ins.replaceOperands((RiscvReg) riscvOperand, newDefReg,
                                        insNode);
                                }
                            } else {
                                newDefReg = new RiscvVirReg(function.getIrRegDispatcher(),
                                    ((RiscvVirReg) riscvOperand).regType, function);
                                RiscvVirReg assistReg =
                                    new RiscvVirReg(function.getIrRegDispatcher(),
                                        RiscvVirReg.RegType.intType, function);
                                function.reMap(newDefReg, (RiscvVirReg) riscvOperand);
                                int offset = function.getOffset(newDefReg);
                                DoubleNode<RiscvInstr> node1;
                                DoubleNode<RiscvInstr> node3;
                                if (offset * -1 < 2048 && offset * -1 >= -2048) {
                                    DoubleNode<RiscvInstr> node2 = new DoubleNode<>(
                                        new RiscvFld(newDefReg, new RiscvImm(-1 * offset),
                                            RiscvCPUReg.getRiscvCPUReg(2))
                                    );
                                    newDefReg.beUsed(node2);
                                    insNode.getParentList().insertBeforeNode(insNode, node2);
                                    ins.replaceOperands((RiscvReg) riscvOperand, newDefReg,
                                        insNode);
                                } else {
                                    node1 = new DoubleNode<>(
                                        new RiscvLi(new RiscvImm(-1 * offset), assistReg));
                                    insNode.getParentList().insertBeforeNode(insNode, node1);
                                    node3 = new DoubleNode<>(
                                        new RiscvBinary(new ArrayList<>(Arrays.asList(assistReg,
                                            RiscvCPUReg.getRiscvCPUReg(2))), assistReg,
                                            RiscvBinary.RiscvBinaryType.add));
                                    assistReg.beUsed(node3);
                                    DoubleNode<RiscvInstr> node2 = new DoubleNode<>(
                                        new RiscvFld(newDefReg, new RiscvImm(0), assistReg)
                                    );
                                    assistReg.beUsed(node2);
                                    newDefReg.beUsed(node2);
                                    insNode.getParentList().insertBeforeNode(insNode, node3);
                                    insNode.getParentList().insertBeforeNode(insNode, node2);
                                    ins.replaceOperands((RiscvReg) riscvOperand, newDefReg,
                                        insNode);
                                }
                            }
                        }
                        ins.replaceOperands((RiscvReg) riscvOperand, newDefReg, insNode);
                    }
                }
            }
            riscvOperand.getUser().clear();
        }
    }
}
