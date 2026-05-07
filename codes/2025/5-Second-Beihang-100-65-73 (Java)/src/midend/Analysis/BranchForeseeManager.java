package midend.Analysis;

import frontend.ir.Value;
import frontend.ir.constant.FloatConst;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.CFG;

import java.util.*;

/**
 * 前置: CFG + LoopAnalysis + ConstFold(只折分支就行）
 * 初始权重来自开源代码
 * <p>
 * 两步：
 * 1) 权重 -> 概率（一次性）
 * 2) 概率 -> 块频率（entry=1，Jacobi 迭代求解）
 */
public class BranchForeseeManager {
    // ======= 权重常量（保持你原来的习惯） =======
    private static final int MAX_WEIGHT = 2048;
    private static final int BACK_TAKEN_WEIGHT = 124;
    private static final int BACK_NOT_TAKEN_WEIGHT = 4;
    private static final int EXIT_HIGH_PRIORITY_WEIGHT = 25;
    private static final int EXIT_LOW_PRIORITY_WEIGHT = 7;
    private static final int BRANCH_TAKEN_WEIGHT = 20;
    private static final int BRANCH_NOT_TAKEN_WEIGHT = 12;

    // ======= 频率迭代参数 =======
    private static final double CONVERGENCE_THRESHOLD = 1e-4;
    private static final int MAX_ITER = 500;

    private boolean freqConverged = false;


    private final Function function;

    public static final class Edge {
        public final BasicBlock from;
        public final BasicBlock to;
        public int weight;        // 初始权重（由启发式给）
        public double probability; // 归一化后的边概率

        public Edge(BasicBlock from, BasicBlock to) {
            this.from = from;
            this.to = to;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (!(o instanceof Edge e)) return false;
            return from == e.from && to == e.to;
        }

        @Override
        public int hashCode() {
            return 31 * System.identityHashCode(from) + System.identityHashCode(to);
        }
    }

    public boolean isFrequencyConverged() { return freqConverged; }

    // 邻接：from -> (to -> Edge)
    public final Map<BasicBlock, Map<BasicBlock, Edge>> edges = new HashMap<>();
    // 块“相对频率”（entry=1 基准）
    public final Map<BasicBlock, Double> blockProbability = new HashMap<>(); // 保留你原来的名字

    public BranchForeseeManager(Function function) {
        this.function = function;
    }

    public void analyseOnFunc() {
        initWeight();                  // 生成 edges 并填充 weight（你的启发式）
        computeEdgeProbFromWeights();  // 一次性：权重 -> 概率
        computeBlockFrequency();       // Jacobi：概率 -> 块频率（entry=1）
        function.setBranchForeseeManager(this);
    }

    private void initWeight() {
        for (BasicBlock block : function.getBasicBlockList()) {
            blockProbability.put(block, 0.0); // 初始化频率表

            if (block.getLastInstr() instanceof JumpInstr j) {
                Edge e = new Edge(block, j.getTarget());
                e.weight = MAX_WEIGHT;
                edges.computeIfAbsent(block, _ -> new HashMap<>()).put(j.getTarget(), e);

            } else if (block.getLastInstr() instanceof BranchInstr b) {
                branchAnalyse(b, block);
            }
        }
    }

    private void branchHighLow(BasicBlock higher, BasicBlock lower, BasicBlock from,
                               int higherWeight, int lowerWeight) {
        edges.computeIfAbsent(from, _ -> new HashMap<>());
        Edge eh = new Edge(from, higher);
        eh.weight = higherWeight;
        edges.get(from).put(higher, eh);
        Edge el = new Edge(from, lower);
        el.weight = lowerWeight;
        edges.get(from).put(lower, el);
    }

    private void branchAnalyse(BranchInstr b, BasicBlock from) {
        BasicBlock tb = b.getThenBlk();
        BasicBlock fb = b.getElseBlk();

        if (from.getLoopBelonged() != null) {
            LoopInfo loop = from.getLoopBelonged();
            if (loop.getLatchBlocks().contains(from)) {
                if (tb == loop.getLoopHeader()) {
                    branchHighLow(tb, fb, from, BACK_TAKEN_WEIGHT, BACK_NOT_TAKEN_WEIGHT);
                } else {
                    branchHighLow(fb, tb, from, BACK_TAKEN_WEIGHT, BACK_NOT_TAKEN_WEIGHT);
                }
                return;
            } else if (loop.getExiting().contains(from)) {
                if (tb.isContainsInLoop(loop)) {
                    branchHighLow(tb, fb, from, BACK_TAKEN_WEIGHT, BACK_NOT_TAKEN_WEIGHT);
                    return;
                } else if (fb.isContainsInLoop(loop)) {
                    branchHighLow(fb, tb, from, BACK_TAKEN_WEIGHT, BACK_NOT_TAKEN_WEIGHT);
                    return;
                } else {
                    if (tb.getDomSet().size() >= fb.getDomSet().size()) {
                        branchHighLow(tb, fb, from, EXIT_HIGH_PRIORITY_WEIGHT, EXIT_LOW_PRIORITY_WEIGHT);
                    } else {
                        branchHighLow(fb, tb, from, EXIT_HIGH_PRIORITY_WEIGHT, EXIT_LOW_PRIORITY_WEIGHT);
                    }
                    return;
                }
            }
        }

        Value cond = b.getCondition();
        if (cond instanceof CmpInstr cmp) {
            if (cmp.getOperand2() instanceof IRConst con) {
                if (con instanceof IntConst ic) {
                    if (ic.getConstInt() == 0) {
                        switch (cmp.getCond()) {
                            case ILT:
                            case ILE:
                            case IEQ:
                                branchHighLow(fb, tb, from, BRANCH_TAKEN_WEIGHT, BRANCH_NOT_TAKEN_WEIGHT);
                                return;
                            case IGT:
                            case IGE:
                            case INE:
                                branchHighLow(tb, fb, from, BRANCH_TAKEN_WEIGHT, BRANCH_NOT_TAKEN_WEIGHT);
                                return;
                            default: /* fallthrough */
                        }
                    } else if (ic.getConstInt() == -1) {
                        switch (cmp.getCond()) {
                            case IEQ:
                                branchHighLow(fb, tb, from, BRANCH_TAKEN_WEIGHT, BRANCH_NOT_TAKEN_WEIGHT);
                                return;
                            case INE:
                                branchHighLow(tb, fb, from, BRANCH_TAKEN_WEIGHT, BRANCH_NOT_TAKEN_WEIGHT);
                                return;
                            default: /* fallthrough */
                        }
                    }
                } else if (con instanceof FloatConst) {
                    if (cmp.getCond() == CmpInstr.CmpCond.FEQ) {
                        branchHighLow(fb, tb, from, BRANCH_TAKEN_WEIGHT, BRANCH_NOT_TAKEN_WEIGHT);
                        return;
                    } else if (cmp.getCond() == CmpInstr.CmpCond.FNE) {
                        branchHighLow(tb, fb, from, BRANCH_TAKEN_WEIGHT, BRANCH_NOT_TAKEN_WEIGHT);
                        return;
                    }
                }
            }
            if (tb.getDomSet().size() >= fb.getDomSet().size()) {
                branchHighLow(tb, fb, from, BRANCH_TAKEN_WEIGHT, BRANCH_NOT_TAKEN_WEIGHT);
            } else {
                branchHighLow(fb, tb, from, BRANCH_TAKEN_WEIGHT, BRANCH_NOT_TAKEN_WEIGHT);
            }
        } else {
            throw new RuntimeException("你这 br 条件哪来的: " + cond);
        }
    }

    private void computeEdgeProbFromWeights() {
        for (BasicBlock from : function.getBasicBlockList()) {
            Map<BasicBlock, Edge> out = edges.get(from);
            if (out == null || out.isEmpty()) continue;

            long sum = 0;
            for (Edge e : out.values()) sum += e.weight;

            if (sum <= 0) {
                double p = 1.0 / out.size();
                for (Edge e : out.values()) e.probability = p;
            } else {
                for (Edge e : out.values()) e.probability = ((double) e.weight) / (double) sum;
            }
        }
    }

    private void computeBlockFrequency() {
        final List<BasicBlock> all = function.getBasicBlockList();
        final BasicBlock entry = function.getFirstBlk();

        // 频率（累计访问次数）与“前沿”
        Map<BasicBlock, Double> freq = new HashMap<>(all.size() * 2);
        Map<BasicBlock, Double> frontier = new HashMap<>(all.size() * 2);
        Map<BasicBlock, Double> next = new HashMap<>(all.size() * 2);

        for (BasicBlock b : all) {
            freq.put(b, 0.0);
            frontier.put(b, 0.0);
            next.put(b, 0.0);
        }
        // 一次性注入入口 1 单位“质量”
        frontier.put(entry, 1.0);

        double residual = Double.POSITIVE_INFINITY;

        for (int it = 0; it < MAX_ITER; ++it) {
            // 1) 把当前前沿累计到频率
            for (BasicBlock b : all) {
                double add = frontier.get(b);
                if (add != 0.0) freq.put(b, freq.get(b) + add);
            }

            // 2) 按边概率把前沿“推进一跳”
            for (BasicBlock b : all) next.put(b, 0.0);
            for (BasicBlock from : all) {
                double mass = frontier.get(from);
                if (mass == 0.0) continue;
                Map<BasicBlock, Edge> out = edges.get(from);
                if (out == null || out.isEmpty()) continue; // sink: 质量在此终止
                for (Edge e : out.values()) {
                    double add = mass * e.probability;
                    if (add == 0.0) continue;
                    next.put(e.to, next.get(e.to) + add);
                }
            }

            // 3) 收敛：下一轮要传播的总量很小
            double l1 = 0.0;
            for (double v : next.values()) l1 += Math.abs(v);
            residual = l1;
            if (l1 < CONVERGENCE_THRESHOLD) {
                freqConverged = true;
                break;
            }

            // 4) 进入下一轮
            for (BasicBlock b : all) frontier.put(b, next.get(b));
        }

        if (!freqConverged) {
            if (residual < 1e-2) {
                freqConverged = true;
            }
            System.out.println("BranchForeseeManager: 未在上限内衰减到阈值, residual=" + residual);
            // 如仍担心慢，可把 MAX_ITER 提到 300~500；或把阈值放宽到 1e-6~1e-5
        }

        blockProbability.clear();
        blockProbability.putAll(freq);
    }

    public double getEdgeProbability(BasicBlock from, BasicBlock to) {
        Map<BasicBlock, Edge> out = edges.get(from);
        if (out == null) return 0.0;
        Edge e = out.get(to);
        return e == null ? 0.0 : e.probability;
    }

    public double getEdgeFrequency(BasicBlock from, BasicBlock to) {
        Map<BasicBlock, Edge> out = edges.get(from);
        if (out == null) return 0.0;
        Edge e = out.get(to);
        if (e == null) return 0.0;
        Double fu = blockProbability.get(from);
        return (fu == null ? 0.0 : fu) * e.probability;
    }

    public void debugPrint() {
        List<BasicBlock> rpo = CFG.getReversePostOrder(function);
        for (BasicBlock bb : rpo) {
            Map<BasicBlock, Edge> out = edges.get(bb);
            if (out == null) continue;
            for (Map.Entry<BasicBlock, Edge> en : out.entrySet()) {
                Edge e = en.getValue();
                System.out.printf("%s -> %s  weight=%d  prob=%.6f  ef=%.6f%n",
                        bb, en.getKey(), e.weight, e.probability, getEdgeFrequency(bb, en.getKey()));
            }
        }
        double mx = 0.0;
        for (double v : blockProbability.values()) mx = Math.max(mx, v);
        for (BasicBlock bb : rpo) {
            double f = blockProbability.getOrDefault(bb, 0.0);
            double vis = (mx > 0 ? f / mx : 0.0);
            System.out.printf("%s  F=%.6f  vis=%.6f%n", bb, f, vis);
        }
    }
}
