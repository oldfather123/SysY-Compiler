package midend.range;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.*;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.Expr.*;
import midend.Analysis.SCEV.LoopCondition;
import midend.Analysis.SCEV.SCEVManager;

import java.util.*;

import static midend.range.LoopUtils.buildBlockToInnermostLoop;
import static midend.range.RangeMath.*;

/**
 * Worklist-based Integer Range Analysis with SCEV integration
 * -----------------------------------------------------------
 * - Forward dataflow: in[BB] -> out[BB]
 * - PHI at block heads with union + loop-latch widening; intersect SCEV(AddRec) range if available
 * - Path-sensitive refinement for icmp-based branches (var/const and const/var; coarse var/var)
 * - ctxAfter(inst, bb): replay transfer from in[bb] and cache the context after 'inst'
 * <p>
 * Notes:
 * 1) This implements TInt only; non-int results default to TOP.
 * 2) No IR mutation (icmp normalization is view-only).
 * 3) Widening on back-edges; optional one-pass narrowing using SCEV at headers.
 */
public class RangeAnalysisWorklistV3 {

    // fixme:开发期间推荐用 full union，避免分析信息丢失
    private static final boolean FULL_MERGE_MODE = true;

    private final Map<BasicBlock, Context> in = new HashMap<>();
    private final Map<BasicBlock, Context> out = new HashMap<>();
    private final Map<MapKey, Context> afterCache = new HashMap<>();
    private final Set<LoopInfo> runOnceLoop = new HashSet<>(); // 回边指向header的loop只能有一次，用于mergeSucc
    private final Map<BasicBlock, Map<Value, Range>> blk2refinedRange = new HashMap<>();
    private List<BasicBlock> topoSortExcludingLatch = new ArrayList<>();

    private record MapKey(Instruction inst, BasicBlock bb) {
    }

    private final Function function;
    private final Map<BasicBlock, LoopInfo> bb2Loop; // assumed available from Function
    private final SCEVManager scevManager;

    /**
     * 当处理到tailLoop的[除header外的body集合]的bound的时候，把得到的ctx赋值给loop的[除header外的body集合]
     */
    private Value bound;
    private LoopInfo main;
    private LoopInfo tailLoop;
    private Map<LoopInfo, LoopInfo> tail2main = new IdentityHashMap<>();
    private Map<LoopInfo, LoopInfo> main2tail = new IdentityHashMap<>();

    public RangeAnalysisWorklistV3(Function f) {
        this.function = f;
        this.topoSortExcludingLatch = f.getTopoSortExludingLatch();
        this.bb2Loop = buildBlockToInnermostLoop(f);
        this.scevManager = f.getSCEVManager();
    }

    public static RangeAnalysisWorklistV3 runOn(Function f) {
        RangeAnalysisWorklistV3 ra = new RangeAnalysisWorklistV3(f);
        ra.solve();
        return ra;
    }

    public Context ctxAfter(Instruction inst, BasicBlock bb) {
        MapKey k = new MapKey(inst, bb);
        Context cached = afterCache.get(k);
        if (cached != null) return cached;

        // === PERF: 一次 pass，沿途把每条指令的 after 都塞进 cache ===
        if (!in.containsKey(bb)) return null;
        Context c = in.get(bb).copy();
        for (Instruction i : bb.getInstrList()) {
            if (!(i instanceof PhiInstr || i instanceof Terminator)) {
                transfer(i, c);
            }
            afterCache.put(new MapKey(i, bb), c.copy()); // 存当前 after
            if (i == inst) break;
        }

        cached = afterCache.get(k);
        if (cached != null) return cached;
        if (inst instanceof PhiInstr phi) {
            return ctxAfter(phi, phi.getParentBB());
        }
        return afterCache.get(k);
    }

    // todo: 在 solve() 的 transfer 循环中也顺手填缓存，避免重复扫描
    public void cacheAfter(BasicBlock bb, Instruction inst, Context ctx) {
        afterCache.put(new MapKey(inst, bb), ctx.copy());
        // fixme bound可能是const int
        if (this.tailLoop != null && this.main != null && this.bound != null &&
                inst == this.bound && LoopUtils.inBodyBlocksButNotHeader(bb, tailLoop)) {
            BasicBlock mainHeader = this.main.getLoopHeader();
            for (BasicBlock latch : this.main.getBodyBlocks()) {
                if (latch == mainHeader) continue; // 不能是header
                afterCache.put(new MapKey(inst, latch), ctx.copy());
            }
        }

    }

    /* ===================== Core solve ===================== */
    private void solve() {
        // init in/out
        for (BasicBlock bb : topoSortExcludingLatch) {
            in.put(bb, new Context());
            out.put(bb, new Context());
        }
        // Seed loop-header PHI with SCEV(AddRec) ranges if available
        seedLoopHeaderPhiWithSCEV(); // 在这一步当中完成了tail2main的收集

        Deque<BasicBlock> work = new ArrayDeque<>();
        IdentityHashMap<BasicBlock, Boolean> inQ = new IdentityHashMap<>();

        // 初始化，必须将所有块加入工作序列
        for (BasicBlock bb : topoSortExcludingLatch) {
            work.add(bb);
            inQ.put(bb, Boolean.TRUE);
            LoopInfo loop = bb.getLoopBelonged();
            if (loop != null && this.main2tail.containsKey(loop)) {
                loop.addBlk2TopoSort(bb);
            }
        }

        while (!work.isEmpty()) {

            BasicBlock b = work.pollFirst();
            // inQ.remove(b);在循环最后执行，防止自己跳自己造成死循环

            // === PERF: 少一次 copy；transfer 时直接在 outCtx 上写 ===
            Context baseIn = in.get(b);
            Context outCtx = baseIn.copy();
            for (Instruction inst : b.getInstrList()) {
                if (inst instanceof PhiInstr phi) {
                    for (Map.Entry<BasicBlock, Value> opEntry : phi.getOperandMap().entrySet()) {
                        Value val = opEntry.getValue();
                        BasicBlock valBlk = opEntry.getKey();
                        if (val instanceof Instruction usedInstr &&
                                outCtx.getOrDefaultNull(usedInstr) != null) {
                            Context opCtx = ctxAfter(usedInstr, valBlk);
                            if (opCtx == null) continue; // 说明还没算到
                            cacheAfter(b, usedInstr, opCtx);
                        }
                    }
                    continue;
                }
                if (inst instanceof Terminator) continue;
                transfer(inst, outCtx);
                // === PERF: 顺手把 afterCache 也填了（见 ctxAfter 改动） ===
                cacheAfter(b, inst, outCtx);
                // 把用到的instr也填进去
                for (Value val : inst.getOperands()) {
                    if (val instanceof Instruction usedInstr &&
                            outCtx.getOrDefaultNull(usedInstr) != null) {
                        cacheAfter(b, usedInstr, outCtx);
                    }
                }
            }
            out.put(b, outCtx);

            // propagate to successors
            Terminator term = (Terminator) b.getLastInstr();
            if (term instanceof JumpInstr jmp) {

                BasicBlock succ = jmp.getTarget();
                if (mergeToSuccessor(b, succ, outCtx)) {
                    if (!inQ.containsKey(succ)) {
                        work.addLast(succ);
                        inQ.put(succ, Boolean.TRUE);
                    }
                }
            } else if (term instanceof BranchInstr br) {
                BasicBlock t = br.getThenBlk();
                BasicBlock e = br.getElseBlk();
                if (mergeToSuccessor(b, t, outCtx, br, true)) {
                    if (!inQ.containsKey(t)) {
                        work.addLast(t);
                        inQ.put(t, Boolean.TRUE);
                    }
                }
                if (mergeToSuccessor(b, e, outCtx, br, false)) {
                    if (!inQ.containsKey(e)) {
                        work.addLast(e);
                        inQ.put(e, Boolean.TRUE);
                    }
                }
            }

            // 在循环最后执行，防止自己跳自己造成死循环
            inQ.remove(b);
        }

        // Optional single-pass narrowing using SCEV on headers (cheap refinement)
        //narrowHeadersOnceWithSCEV();
    }

    /**
     * Merge outCtx of pred into succ's in-context, handling PHI and optional branch refinement.
     */
    private boolean mergeToSuccessor(BasicBlock pred, BasicBlock succ, Context outCtx) {
        return mergeToSuccessor(pred, succ, outCtx, null, false);
    }

    /**
     * Range merge的规则
     * 若 `oldPhi == null`：这是对该 `phi` 的第一次合并，直接用本前驱贡献的 `incR` 作为 `merged`。
     * 否则，分两种边：
     * - 回边→头块：用 `widen`（经典拓宽）。
     * - 语义：若新值把下界再压低/上界再抬高，就把该边界直接放到 `-∞/+∞`（用 `INT_MIN/INT_MAX` 近似），以强制收敛。
     * - 代价：精度会变松；你随后会用 SCEV `intersect` 拉回一部分。
     * - 普通边：用 `union`。
     * - 语义：取两边界的最小下界/最大上界，是标准的并（“可能集变大”）。
     */
    private boolean mergeToSuccessor(BasicBlock pred, BasicBlock succ, Context outCtx,
                                     BranchInstr br, boolean trueBranch) {

        LoopInfo loop = succ.getLoopBelonged();
        if (loop != null && loop.getLatchBlocks().contains(pred) && loop.getLoopHeader() == succ) {
            if (runOnceLoop.contains(loop))
                // 如果是回边指向开头就不merge了
                return false;
            else runOnceLoop.add(loop);
        }

        Context base = in.get(succ);
        Context next = base.copy();

        boolean changed = false;

        // 1) 只合并 outCtx 的“本地增量”
        if (FULL_MERGE_MODE) {
            changed |= next.unionAllFrom(outCtx, this, succ);
        } else {
            changed |= next.unionLocalsFrom(outCtx);
        }

        // 2) 分支路径细化（变化才写）
        if (br != null) {
            // 旧：用 sizeBefore 判断是否变化，可能漏判
            // boolean before = changed;
            // int sizeBefore = next.local.size();
            // refineWithBranch(br, trueBranch, next);
            // if (next.local.size() != sizeBefore) changed = true;

            // 新：直接用返回值汇总变化
            changed |= refineWithBranch(br, trueBranch, next, succ);

        }

        // 3) PHI 只看头部连续的 PHI；遇到首个非 PHI 立即停止
        for (Instruction inst : succ.getInstrList()) {
            if (!(inst instanceof PhiInstr phi)) break;
            if (!(phi.getType() instanceof TInt)) continue;
            Value incoming = phi.getOperandMap().get(pred);
            if (incoming == null) continue;

            // 先查当前in中有无incR（可能从refineBr中得到新值）
            Range incR = rangeOfOrDefaultNull(incoming, next);
            if (incR == null) {
                // 再查pred的OutCtx中的incR
                incR = rangeOfOrDefaultTop(incoming, outCtx);
            }

            Range oldPhi = next.getOrDefaultNull(phi);

            Range merged = oldPhi == null ? incR :
                    (isBackEdgeToHeader(pred, succ) ? oldPhi.widen(incR) : oldPhi.union(incR));
            Range scevR = scevPhiRangeIfAny(phi);
            if (scevR != null) merged = merged.intersect(scevR);
            if (merged != oldPhi && !merged.equals(oldPhi)) {
                next.put(phi, merged);
                cacheAfter(succ, phi, next);
                changed = true;
            }
        }

        if (changed) {
            in.put(succ, next.compactIfNeeded()); // 仅在确有变化时，可能压缩
        }
        return changed;
    }

    private void transfer(Instruction inst, Context ctx) {
        if (!(inst.getType() instanceof TInt)) {
            // non-int results: ignore
            return;
        }
        Range res;
        switch (inst) {
            case BinaryOperation bin -> {
                Range a = rangeOfOrDefaultTop(bin.getOperand1(), ctx);
                Range b = rangeOfOrDefaultTop(bin.getOperand2(), ctx);
                res = evalBinary(bin, a, b);
            }
            case CallInstr call -> res = evalCall(call);
            case PhiInstr _ ->
                // PHI handled in merge; keep whatever is present
                    res = ctx.getOrDefaultTop(inst);
            default ->
                // default conservative
                    res = Range.top();
        }
        ctx.put(inst, res);
    }

    private Range evalBinary(BinaryOperation inst, Range r1, Range r2) {
        if (r1.isBottom() || r2.isBottom()) return Range.bottom();
        int a1 = r1.min, a2 = r1.max, b1 = r2.min, b2 = r2.max;

        if (inst instanceof AddInstr) {
            return new Range(safeAdd(a1, b1, true), safeAdd(a2, b2, false));
        } else if (inst instanceof SubInstr) {
            return new Range(safeSub(a1, b2, true), safeSub(a2, b1, false));
        } else if (inst instanceof MulInstr) {
            int m1 = safeMul(a1, b1, true), M1 = safeMul(a1, b1, false);
            int m2 = safeMul(a1, b2, true), M2 = safeMul(a1, b2, false);
            int m3 = safeMul(a2, b1, true), M3 = safeMul(a2, b1, false);
            int m4 = safeMul(a2, b2, true), M4 = safeMul(a2, b2, false);
            int mn = Math.min(Math.min(m1, m2), Math.min(m3, m4));
            int mx = Math.max(Math.max(M1, M2), Math.max(M3, M4));
            return new Range(mn, mx);
        } else if (inst instanceof SDivInstr) {
            return divRange(r1, r2);
        } else if (inst instanceof SRemInstr) {
            return remRange(r1, r2);
        }
        return Range.top();
    }


    private Range evalCall(CallInstr call) {
        if (!(call.getCallee() instanceof Function.LibFunc lib)) return Range.top();
        return switch (lib.getName()) {
            case "getch" -> new Range(-128, 127);
            case "getarray", "getfarray" -> new Range(0, Integer.MAX_VALUE);
            default -> Range.top();
        };
    }

    private Range rangeOfOrDefaultTop(Value v, Context ctx) {
        if (v instanceof IntConst ic) return Range.constRange(ic.getConstInt().intValue());
        if (!(v.getType() instanceof TInt)) return Range.top();

        Range r = ctx.getOrDefaultTop(v);
        /*if (r.isTop()) {
            if (v instanceof Instruction instr) {
                Context c = ctxAfter(instr, blk);
                return c.get(v);
            }
        }*/
        return r;
    }

    private Range rangeOfOrDefaultNull(Value v, Context ctx) {
        if (v instanceof IntConst ic) return Range.constRange(ic.getConstInt().intValue());
        if (!(v.getType() instanceof TInt)) return Range.top();

        Range r = ctx.getOrDefaultNull(v);
        /*if (r.isTop()) {
            if (v instanceof Instruction instr) {
                Context c = ctxAfter(instr, blk);
                return c.get(v);
            }
        }*/
        return r;
    }

    /**
     * 新：返回是否有收紧变化
     */

    private boolean refineWithBranch(BranchInstr br, boolean trueBranch, Context ctx, BasicBlock succ) {
        Value cond = br.getCondition();
        if (!(cond instanceof CmpInstr icmp) || !icmp.isI32()) return false;

        boolean isBrTailBranch = false;
        LoopInfo tail = br.getParentBB().getLoopBelonged();
        if (tail != null && tail2main.containsKey(tail)) {
            LoopCondition tailCond = tail.getLoopCondition();
            if (tailCond != null && br == tailCond.branch) {
                isBrTailBranch = true;
            }
        }

        Value L = icmp.getOperand1();
        Value R = icmp.getOperand2();

        boolean swapped = false;
        if (L instanceof IntConst && !(R instanceof IntConst)) {
            // 统一成 (var, const) 形态
            Value tmp = L;
            L = R;
            R = tmp;
            swapped = true;
        }

        Range baseL = rangeOfOrDefaultTop(L, ctx);
        Range baseR = rangeOfOrDefaultTop(R, ctx);
        if (!(L.getType() instanceof TInt)) return false;

        CmpInstr.CmpCond c = icmp.getCond();
        if (swapped) c = swapCond(c);

        boolean changed = false;

        // (1) R 是常量：只需要收紧 L（你原有逻辑，保留）
        if (R instanceof IntConst rc) {
            int k = rc.getConstInt().intValue();
            Range newL = getRangeWhenRightIsConst(k, baseL, trueBranch, c);
            // 如果br是tail的branch，那么这个newL应该传播给main的loopCondition
            // fixme
            /*if (isBrTailBranch && tail.getBodyBlocks().contains(succ)) {
                transferFromTail2Main(tail, getRangeWhenRightIsConst(k, Range.top(), trueBranch, c));
            }*/
            if (!newL.equals(baseL)) {
                ctx.put(L, mergeRange(L, newL, succ));
                if (L instanceof Instruction IL) {
                    cacheAfter(succ, IL, ctx);
                }
                changed = true;
            }
            return changed;
        }

        // (2) L、R 都是变量：对称收紧两边
        // 取消原来的 TOP 早退：即便一边是 TOP，也能从另一边推得有用的界
        int lmin = baseL.min, lmax = baseL.max;
        int rmin = baseR.min, rmax = baseR.max;

        Range newL = baseL, newR = baseR;

        switch (c) {
            case IEQ:
                if (trueBranch) {
                    newL = baseL.intersect(baseR);
                    newR = baseR.intersect(baseL);
                } // false 分支是 “!=” ，单区间域难以表达 -> 保守不变
                break;

            case INE:
                if (!trueBranch) { // false 分支即 “==”
                    newL = baseL.intersect(baseR);
                    newR = baseR.intersect(baseL);
                }
                break;

            case IGE: // L >= R
                if (trueBranch) {
                    newL = new Range(Math.max(lmin, rmin), lmax);            // L ≥ R ≥ rmin ⇒ L ≥ rmin
                    newR = new Range(rmin, Math.min(rmax, lmax));            // R ≤ L ≤ lmax ⇒ R ≤ lmax
                } else { // L < R
                    newL = new Range(lmin, Math.min(lmax, decCap(rmax)));    // L ≤ R-1 ≤ rmax-1
                    newR = new Range(Math.max(rmin, incCap(lmin)), rmax);    // R ≥ L+1 ≥ lmin+1
                }
                break;

            case IGT: // L > R
                if (trueBranch) {
                    newL = new Range(Math.max(lmin, incCap(rmin)), lmax);    // L ≥ R+1 ≥ rmin+1
                    newR = new Range(rmin, Math.min(rmax, lmax));            // R ≤ L ≤ lmax
                } else { // L ≤ R
                    newL = new Range(lmin, Math.min(lmax, rmax));            // L ≤ R ≤ rmax
                    newR = new Range(Math.max(rmin, lmin), rmax);            // R ≥ L ≥ lmin
                }
                break;

            case ILE: // L ≤ R
                if (trueBranch) {
                    newL = new Range(lmin, Math.min(lmax, rmax));            // L ≤ R ≤ rmax
                    newR = new Range(Math.max(rmin, lmin), rmax);            // R ≥ L ≥ lmin
                } else { // L > R
                    newL = new Range(Math.max(lmin, incCap(rmin)), lmax);    // L ≥ R+1 ≥ rmin+1
                    newR = new Range(rmin, Math.min(rmax, lmax));            // R ≤ L ≤ lmax
                }
                break;

            case ILT: // L < R
                if (trueBranch) {
                    newL = new Range(lmin, Math.min(lmax, decCap(rmax)));    // L ≤ R-1 ≤ rmax-1
                    newR = new Range(Math.max(rmin, incCap(lmin)), rmax);    // R ≥ L+1 ≥ lmin+1
                } else { // L ≥ R
                    newL = new Range(Math.max(lmin, rmin), lmax);            // L ≥ R ≥ rmin
                    newR = new Range(rmin, Math.min(rmax, lmax));            // R ≤ L ≤ lmax
                }
                break;

            default: // 保守不变
                break;
        }

        if (!newL.equals(baseL)) {
            ctx.put(L, mergeRange(L, newL, succ));
            if (L instanceof Instruction IL) {
                cacheAfter(succ, IL, ctx);
            }
            changed = true;
        }
        if (!newR.equals(baseR)) {
            Range putR = mergeRange(R, newR, succ);
            ctx.put(R, putR);
            if (R instanceof Instruction IR) {
                cacheAfter(succ, IR, ctx);
            }
            changed = true;
        }
        return changed;
    }

    private Range getRangeWhenRightIsConst(int rc, Range baseL, boolean trueBranch, CmpInstr.CmpCond c) {
        int k = rc;
        Range newL = switch (c) {
            case IEQ -> trueBranch ? Range.constRange(k) : baseL;
            case INE -> baseL; // 单区间域下保持保守
            case IGE -> trueBranch ? new Range(Math.max(baseL.min, k), baseL.max)
                    : new Range(baseL.min, Math.min(baseL.max, k - 1));
            case IGT -> trueBranch ? new Range(Math.max(baseL.min, incCap(k)), baseL.max)
                    : new Range(baseL.min, Math.min(baseL.max, k));
            case ILE -> trueBranch ? new Range(baseL.min, Math.min(baseL.max, k))
                    : new Range(Math.max(baseL.min, incCap(k)), baseL.max);
            case ILT -> trueBranch ? new Range(baseL.min, Math.min(baseL.max, decCap(k)))
                    : new Range(Math.max(baseL.min, k), baseL.max);
            default -> baseL;
        };
        return newL;
    }

    private boolean isControlSimple(Set<BasicBlock> bodyExcludeHeader) {
        for (BasicBlock bb : bodyExcludeHeader) {
            int succNum = 0;
            for (BasicBlock succ : bb.getSucs()) {
                if (bodyExcludeHeader.contains(succ)) succNum++;
            }
            if (succNum != 1 && succNum != 0) return false;
        }
        return true;
    }

    //todo
    int cnt = 0;

    private void transferFromTail2Main(LoopInfo tail, Range LR) {
//        cnt++; System.out.println(cnt);
        LoopInfo main = this.tail2main.get(tail);
        LoopCondition mainCond = main.getLoopCondition();
        if (mainCond == null) return;

        BasicBlock header = main.getLoopHeader();
        Set<BasicBlock> bodyExcludeHeader = new HashSet<>(main.getBodyBlocks());
        bodyExcludeHeader.remove(header);

        // 先判断控制流简单，要求内部为单一线条
        if (!isControlSimple(bodyExcludeHeader)) return;

        Value mainL = mainCond.icmp.getOperand1();
        if (mainL instanceof IntConst) {
            // 交换这步
            mainL = mainCond.icmp.getOperand2();
        }


        // 处理header
        Map<Value, Range> newRanges = new HashMap<>();

        // 特判
        if (LR.max == Integer.MAX_VALUE) {
            LR = new Range(3 + LR.min, LR.max);
        }
        newRanges.put(mainL, LR);

        //todo
        boolean prev = true;

        for (Instruction inst : header.getInstrList()) {
            if (!prev)
                if (!(inst.getType() instanceof TInt)) continue;
            if (inst instanceof BinaryOperation bin) {
                Value op1 = bin.getOperand1();
                Value op2 = bin.getOperand2();

                Range a = newRanges.getOrDefault(op1, RangeAnalysisV3.getRange(op1, header));
                Range b = newRanges.getOrDefault(op2, RangeAnalysisV3.getRange(op2, header));
                Range res = evalBinary(bin, a, b);
                newRanges.put(inst, res);
            }
        }

        Deque<BasicBlock> work = new ArrayDeque<>();

        for (BasicBlock succ : header.getSucs()) {
            if (bodyExcludeHeader.contains(succ)) work.add(succ);
        }

        while (!work.isEmpty()) {
            BasicBlock bb = work.pollFirst();

            for (Instruction inst : bb.getInstrList()) {
                if (!prev)
                    if (!(inst.getType() instanceof TInt)) continue;
                if (inst instanceof BinaryOperation bin) {
                    Value op1 = bin.getOperand1();
                    Value op2 = bin.getOperand2();

                    Range a = newRanges.getOrDefault(op1, RangeAnalysisV3.getRange(op1, header));
                    Range b = newRanges.getOrDefault(op2, RangeAnalysisV3.getRange(op2, header));
                    Range res = evalBinary(bin, a, b);
                    newRanges.put(inst, res);
                }
            }

            for (Map.Entry<Value, Range> entry : newRanges.entrySet()) {
                if (entry.getKey() instanceof Instruction inst) {
                    Context c = ctxAfter(inst, bb); // 不copy
                    c.put(inst, entry.getValue());
                    cacheAfter(bb, inst, c);
                }
            }

            for (BasicBlock succ : bb.getSucs()) {
                if (bodyExcludeHeader.contains(succ)) {
                    work.add(succ);
                }
            }
        }
    }

    private Range mergeRange(Value val, Range newR, BasicBlock bb) {
        if (!blk2refinedRange.containsKey(bb)) {
            blk2refinedRange.put(bb, new HashMap<>());
        }

        /*if (!(val instanceof Instruction instr)) return Range.top();
        Context succContext = ctxAfter(instr, bb);
        if (succContext == null) return newR;
        Range r = succContext.getOrDefaultNull(val);
        if (r == null) return newR;
        else return r.union(newR);*/
        if (blk2refinedRange.get(bb).containsKey(val)) {
            Range uR = blk2refinedRange.get(bb).get(val).union(newR);
            blk2refinedRange.get(bb).put(val, uR);
            return uR;
        } else {
            blk2refinedRange.get(bb).put(val, newR);
            return newR;
        }
    }

    private CmpInstr.CmpCond swapCond(CmpInstr.CmpCond c) {
        return switch (c) {
            case IEQ -> CmpInstr.CmpCond.IEQ;
            case INE -> CmpInstr.CmpCond.INE;
            case IGE -> CmpInstr.CmpCond.ILE; // a>=b  ==  b<=a
            case IGT -> CmpInstr.CmpCond.ILT;
            case ILE -> CmpInstr.CmpCond.IGE;
            case ILT -> CmpInstr.CmpCond.IGT;
            default -> c;
        };
    }


    /* ===================== Loop/back-edge helper & SCEV hooks ===================== */
    private boolean isBackEdgeToHeader(BasicBlock pred, BasicBlock succ) {
        LoopInfo L = bb2Loop.get(succ);
        if (L == null) return false;
        if (!succ.equals(L.getLoopHeader())) return false;
        return L.getLatchBlocks().contains(pred);
    }

    /**
     * If phi has SCEV AddRec, return its modeled range; else null.
     */
    private Range scevPhiRangeIfAny(PhiInstr phi) {
        SCEVExpr expr = scevManager.getSCEV(phi);
        if (expr instanceof SCEVAddRecExpr addrec) {
            Range oriR = addrec.getRange(); // reuse your existing Range type if needed
            if (addrec.operands.getFirst() instanceof SCEVUnknown unknown) {
                Value init = unknown.val;
                BasicBlock pred = null;
                for (Map.Entry<BasicBlock, Value> entry : phi.getOperandMap().entrySet()) {
                    if (entry.getValue() == init) {
                        pred = entry.getKey();
                        break;
                    }
                }
                if (pred == null) return oriR;
                Range initR = rangeOfOrDefaultTop(init, in.get(pred));

                // 保守处理：只判断addrec的步长皆为SCEVConstant的情况
                int sum = 0;
                for (int i = 1; i < addrec.operands.size(); i++) {
                    if (addrec.operands.get(i) instanceof SCEVConstant c) {
                        sum += c.getValue();
                    } else {
                        return oriR;
                    }
                }

                if (sum > 0) {
                    return new Range(initR.min, Integer.MAX_VALUE);
                } else if (sum < 0) {
                    return new Range(Integer.MIN_VALUE, initR.max);
                }

            }
            // 其他复杂情况：保守返回top
            return oriR;

        } else if (expr instanceof SCEVVecAddRecExpr vecAddRecExpr) {
            LoopInfo loop = vecAddRecExpr.getLoop();
            if (loop != null) dealLoopWithSCEVVecAdd(loop);
            return vecAddRecExpr.getRange();
        }
        return null;
    }

    /**
     * 专门解决unroll拆解形成的两个循环，已保证loop不为null
     */
    private boolean dealLoopWithSCEVVecAdd(LoopInfo loop) {
        LoopInfo tail = loop.getTailLoop();
        if (tail == null) return false;

        LoopCondition tailCondition = tail.getLoopCondition();
        if (tailCondition == null) return false;

        this.bound = tailCondition.bound;
        this.main = loop;
        this.tailLoop = tail;
        this.tail2main.put(tail, loop);
        this.main2tail.put(loop, tail);
        return true;
    }

    /**
     * Pre-seed header PHIs with SCEV range to speed convergence and keep tight.
     */
    private void seedLoopHeaderPhiWithSCEV() {
        for (BasicBlock bb : topoSortExcludingLatch) {
            LoopInfo L = bb2Loop.get(bb);
            if (L == null || !bb.equals(L.getLoopHeader())) continue;
            Context c = in.get(bb);
            for (Instruction inst : bb.getInstrList()) {
                if (!(inst instanceof PhiInstr phi)) break;
                if (!(phi.getType() instanceof TInt)) continue;
                Range r = scevPhiRangeIfAny(phi);
                if (r != null) c.put(phi, r);
            }
        }
    }

    /**
     * One cheap post-pass: intersect header PHIs with SCEV once, and do a single push.
     */
    private void narrowHeadersOnceWithSCEV() {
        Deque<BasicBlock> work = new ArrayDeque<>();
        Set<BasicBlock> inQ = new HashSet<>();
        for (BasicBlock bb : topoSortExcludingLatch) {
            LoopInfo L = bb2Loop.get(bb);
            if (L == null || !bb.equals(L.getLoopHeader())) continue;
            boolean touched = false;
            Context c = in.get(bb).copy();
            for (Instruction inst : bb.getInstrList()) {
                if (!(inst instanceof PhiInstr phi)) break;
                if (!(phi.getType() instanceof TInt)) continue;
                Range scev = scevPhiRangeIfAny(phi);
                if (scev == null) continue;
                Range old = c.getOrDefaultTop(phi);
                Range inter = old.intersect(scev);
                if (!inter.equals(old)) {
                    c.put(phi, inter);
                    cacheAfter(bb, phi, c);
                    touched = true;
                }
            }
            if (touched) {
                in.put(bb, c);
                work.add(bb);
                inQ.add(bb);
            }
        }
        // single forward push
        while (!work.isEmpty()) {
            BasicBlock b = work.pollFirst();
            inQ.remove(b);
            Context outCtx = in.get(b).copy();
            for (Instruction inst : b.getInstrList()) {
                if (inst instanceof PhiInstr || inst instanceof Terminator) continue;
                transfer(inst, outCtx);
                cacheAfter(b, inst, outCtx);
            }
            out.put(b, outCtx);
            Terminator term = (Terminator) b.getLastInstr();
            if (term instanceof JumpInstr jmp) {
                BasicBlock succ = jmp.getTarget();
                if (mergeToSuccessor(b, succ, outCtx)) if (inQ.add(succ)) work.addLast(succ);
            } else if (term instanceof BranchInstr br) {
                BasicBlock t = br.getThenBlk();
                BasicBlock e = br.getElseBlk();
                if (mergeToSuccessor(b, t, outCtx, br, true)) if (inQ.add(t)) work.addLast(t);
                if (mergeToSuccessor(b, e, outCtx, br, false)) if (inQ.add(e)) work.addLast(e);
            }
        }
    }
}
