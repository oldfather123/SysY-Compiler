package midend.range;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;

import java.util.IdentityHashMap;

/* ===================== Context: Value -> Range ===================== */
public class Context {
    private final Context parent; // 可为空；链式共享历史
    private final IdentityHashMap<Value, Range> local;
    private final int depth; // 仅用于控制是否压缩

    // 构造：根上下文
    public Context() {
        this.parent = null;
        this.local = new IdentityHashMap<>();
        this.depth = 0;
    }

    // 构造：overlay
    private Context(Context parent) {
        this.parent = parent;
        this.local = new IdentityHashMap<>();
        this.depth = parent.depth + 1;
    }

    /**
     * 轻量“复制”：创建一个空的 overlay，O(1)，不搬数据
     */
    public Context copy() {
        return new Context(this);
    }

    /**
     * 查找：先看本地，再走父链；常见命中在本地（热路径）
     */
    public Range getOrDefaultTop(Value v) {
        if (v instanceof IntConst ic) return Range.constRange(ic.getConstInt().intValue());
        Range r = local.get(v);
        if (r != null) return r;
        return parent == null ? Range.top() : parent.getOrDefaultTop(v);
    }

    public Range getOrDefaultNull(Value v) {
        if (v instanceof IntConst ic) return Range.constRange(ic.getConstInt().intValue());
        Range r = local.get(v);
        if (r != null) return r;
        return parent == null ? null : parent.getOrDefaultNull(v);
    }

    /**
     * 只写本地增量
     */
    public void put(Value v, Range r) {
        local.put(v, r);
    }

    /**
     * 将另一个上下文的“本地增量”合并进来（热路径只需这一个）
     */
    public boolean unionLocalsFrom(Context o) {
        boolean changed = false;
        for (var e : o.local.entrySet()) {
            Value k = e.getKey();
            Range inc = e.getValue();
            Range old = getOrDefaultNull(k);             // 注意：get() 看见的可能来自父链
            Range merged = (old == null) ? inc : old.union(inc);
            if (merged != old && !merged.equals(old)) {
                local.put(k, merged);       // 只在变更时写
                // 若要用 cacheafter必须加上
                changed = true;
            }
        }
        return changed;
    }

    /**
     * 兜底：需要把“完整可见集”都迭代时使用（较少走到）
     */
    public boolean unionAllFrom(Context o, RangeAnalysisWorklistV3 ra, BasicBlock bb) {
        //System.out.println("unionAllFrom: debug");

        boolean changed = false;
        // 先把 o 的父链全部扁平到一个临时视图，再迭代（避免重复键）
        IdentityHashMap<Value, Range> temp = new IdentityHashMap<>();
        o.collectAllInto(temp);
        for (var e : temp.entrySet()) {
            Value k = e.getKey();
            Range inc = e.getValue();
            Range old = getOrDefaultNull(k);
            Range merged = (old == null) ? inc : old.union(inc);
            if (merged != old && !merged.equals(old)) {
                local.put(k, merged);
                if (k instanceof Instruction instr)
                    ra.cacheAfter(bb, instr, this);
                changed = true;
            }
        }
        return changed;
    }

    // 将自身“可见的全部绑定”收集到 map 中
    private void collectAllInto(IdentityHashMap<Value, Range> dst) {
        if (parent != null) parent.collectAllInto(dst);
        dst.putAll(local);
    }

    /**
     * 压缩：当链太深或本地太大时，把可见集一次性扁平化到根 Context
     */
    public Context compactIfNeeded() {
        final int MAX_DEPTH = 8;
        final int MAX_LOCAL = 64;
        if (depth <= MAX_DEPTH && local.size() <= MAX_LOCAL) return this;

        IdentityHashMap<Value, Range> flat = new IdentityHashMap<>();
        collectAllInto(flat);
        Context root = new Context();
        root.local.putAll(flat);
        return root;
    }

    /**
     * 仅用于测试/调试
     */
    public boolean equalsTo(Context o) {
        IdentityHashMap<Value, Range> a = new IdentityHashMap<>(), b = new IdentityHashMap<>();
        this.collectAllInto(a);
        o.collectAllInto(b);
        return a.equals(b);
    }
}