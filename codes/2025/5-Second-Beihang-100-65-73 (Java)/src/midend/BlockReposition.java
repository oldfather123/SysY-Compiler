package midend;

import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.BranchForeseeManager;

import java.util.*;

/** EF = F(u)*P(u->v) 的 trace 重排；未收敛时回退到只用边概率的保守布局 */
public class BlockReposition {

    // EF 布局参数
    private static final double FORWARD_EF_FRAC = 0.15;
    private static final double BACKWARD_EF_FRAC = 0.15;
    private static final double MIN_ABS_EDGE_EF = 1e-6;

    // 回退布局参数（只用 P 的阈值）
    private static final double PROB_THRESH = 0.65; // 概率至少 50% 才继续延长

    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            BranchForeseeManager bfm = new BranchForeseeManager(function);
            bfm.analyseOnFunc();
            // bfm.debugPrint(); // 需要时打开
            executeOnFunc(function);
        }
    }

    public static void executeOnFunc(Function function) {
        final List<BasicBlock> blocks = function.getBasicBlockList();
        final int n = blocks.size();
        if (n <= 1) return;

        final Map<BasicBlock, Integer> origIdx = new IdentityHashMap<>();
        for (int i = 0; i < n; ++i) origIdx.put(blocks.get(i), i);

        final BranchForeseeManager prophet = function.getBranchForeseeManager();

        if (prophet.isFrequencyConverged()) {
            // === 正常路径：用 EF 的 trace 布局 ===
            executeWithEF(function, blocks, origIdx, prophet);
        } else {
            // === 回退路径：只按 P 的保守布局 ===
            executeWithProbFallback(function, blocks, origIdx, prophet);
        }
    }

    // =================== 已收敛：EF trace ===================

    private static void executeWithEF(Function function,
                                      List<BasicBlock> blocks,
                                      Map<BasicBlock, Integer> origIdx,
                                      BranchForeseeManager prophet) {
        final int n = blocks.size();
        final Set<BasicBlock> placed = new HashSet<>(n);
        final List<BasicBlock> order = new ArrayList<>(n);

        final BasicBlock entry = blocks.get(0);
        order.addAll(buildHotTraceEF(entry, placed, prophet, origIdx));

        while (placed.size() < n) {
            BasicBlock seed = pickHottestUnplacedSeed(blocks, placed, prophet, origIdx);
            if (seed == null) { // 全 0 等情况：按原序补齐
                for (BasicBlock b : blocks) if (!placed.contains(b)) {
                    order.add(b); placed.add(b);
                }
                break;
            }
            order.addAll(buildHotTraceEF(seed, placed, prophet, origIdx));
        }

        function.ReallocateBlocks(order);
    }

    private static BasicBlock pickHottestUnplacedSeed(List<BasicBlock> blocks,
                                                      Set<BasicBlock> placed,
                                                      BranchForeseeManager prophet,
                                                      Map<BasicBlock, Integer> origIdx) {
        BasicBlock best = null;
        double bestF = -1.0;
        for (BasicBlock b : blocks) {
            if (placed.contains(b)) continue;
            double f = prophet.blockProbability.getOrDefault(b, 0.0);
            if (f > bestF || (f == bestF && best != null &&
                    origIdx.get(b) < origIdx.get(best))) {
                best = b; bestF = f;
            }
        }
        return best;
    }

    private static final class Pick {
        BasicBlock bb; double ef; double prob; double bf;
        Pick(BasicBlock bb, double ef, double prob, double bf) {
            this.bb = bb; this.ef = ef; this.prob = prob; this.bf = bf;
        }
    }

    private static List<BasicBlock> buildHotTraceEF(BasicBlock seed,
                                                    Set<BasicBlock> placed,
                                                    BranchForeseeManager prophet,
                                                    Map<BasicBlock, Integer> origIdx) {
        List<BasicBlock> trace = new ArrayList<>();
        if (placed.contains(seed)) return trace;
        trace.add(seed); placed.add(seed);

        // 前向扩展（EF）
        BasicBlock tail = seed;
        while (true) {
            Pick p = bestForwardSuccEF(tail, placed, prophet, origIdx);
            if (p == null) break;
            double fTail = prophet.blockProbability.getOrDefault(tail, 0.0);
            if (p.ef < Math.max(MIN_ABS_EDGE_EF, FORWARD_EF_FRAC * fTail)) break;
            trace.add(p.bb); placed.add(p.bb); tail = p.bb;
        }

        // 后向扩展（EF）
        BasicBlock head = seed;
        while (true) {
            Pick p = bestBackwardPredEF(head, placed, prophet, origIdx);
            if (p == null) break;
            double fPred = prophet.blockProbability.getOrDefault(p.bb, 0.0);
            if (p.ef < Math.max(MIN_ABS_EDGE_EF, BACKWARD_EF_FRAC * fPred)) break;
            trace.add(0, p.bb); placed.add(p.bb); head = p.bb;
        }

        return trace;
    }

    private static Pick bestForwardSuccEF(BasicBlock from,
                                          Set<BasicBlock> placed,
                                          BranchForeseeManager prophet,
                                          Map<BasicBlock, Integer> origIdx) {
        BasicBlock best = null;
        double bestEF = -1.0, bestProb = -1.0, bestBF = -1.0;
        for (BasicBlock s : from.getSucs()) {
            if (placed.contains(s)) continue;
            double prob = prophet.getEdgeProbability(from, s);
            if (prob <= 0.0) continue;
            double ef = prophet.getEdgeFrequency(from, s);
            double bf = prophet.blockProbability.getOrDefault(s, 0.0);

            if (ef > bestEF
                    || (ef == bestEF && (prob > bestProb
                    || (prob == bestProb && (bf > bestBF
                    || (bf == bestBF && origIdx.get(s) < origIdx.get(best))))))) {
                best = s; bestEF = ef; bestProb = prob; bestBF = bf;
            }
        }
        return (best == null) ? null : new Pick(best, bestEF, bestProb, bestBF);
    }

    private static Pick bestBackwardPredEF(BasicBlock to,
                                           Set<BasicBlock> placed,
                                           BranchForeseeManager prophet,
                                           Map<BasicBlock, Integer> origIdx) {
        BasicBlock best = null;
        double bestEF = -1.0, bestProb = -1.0, bestBF = -1.0;
        for (BasicBlock p : to.getPres()) {
            if (placed.contains(p)) continue;
            double prob = prophet.getEdgeProbability(p, to);
            if (prob <= 0.0) continue;
            double ef = prophet.getEdgeFrequency(p, to);
            double bf = prophet.blockProbability.getOrDefault(p, 0.0);

            if (ef > bestEF
                    || (ef == bestEF && (prob > bestProb
                    || (prob == bestProb && (bf > bestBF
                    || (bf == bestBF && origIdx.get(p) < origIdx.get(best))))))) {
                best = p; bestEF = ef; bestProb = prob; bestBF = bf;
            }
        }
        return (best == null) ? null : new Pick(best, bestEF, bestProb, bestBF);
    }

    // =================== 未收敛：只用 P 的保守回退 ===================

    private static void executeWithProbFallback(Function function,
                                                List<BasicBlock> blocks,
                                                Map<BasicBlock, Integer> origIdx,
                                                BranchForeseeManager prophet) {
        final int n = blocks.size();
        final Set<BasicBlock> placed = new HashSet<>(n);
        final List<BasicBlock> order = new ArrayList<>(n);

        final BasicBlock entry = blocks.get(0);
        order.addAll(buildTraceProbOnly(entry, placed, prophet));

        // 其余按原始顺序补齐（每个未放置块各自起一条局部链）
        for (BasicBlock b : blocks) {
            if (!placed.contains(b)) {
                order.addAll(buildTraceProbOnly(b, placed, prophet));
            }
        }

        function.ReallocateBlocks(order);
    }

    /** 只按边概率前向扩展；不依赖 F，不做后向扩展，概率阈值默认 0.5 */
    private static List<BasicBlock> buildTraceProbOnly(BasicBlock seed,
                                                       Set<BasicBlock> placed,
                                                       BranchForeseeManager prophet) {
        List<BasicBlock> trace = new ArrayList<>();
        if (placed.contains(seed)) return trace;
        trace.add(seed); placed.add(seed);

        BasicBlock cur = seed;
        while (true) {
            BasicBlock best = null;
            double bestP = -1.0;
            for (BasicBlock s : cur.getSucs()) {
                if (placed.contains(s)) continue;
                double p = prophet.getEdgeProbability(cur, s);
                if (p > bestP) { bestP = p; best = s; }
            }
            if (best == null || bestP < PROB_THRESH) break; // 保守：<0.5 就停止
            trace.add(best); placed.add(best); cur = best;
        }
        return trace;
    }
}
