package midend;

import frontend.ir.Value;
import frontend.ir.Visitor;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.convop.Bitcast;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.TChar;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.objecttype.derived.TArray;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.GlbObjPtr;

import java.util.*;

/**
 * 指针属性传播分析（别名分析）：
 * 为循环（？）优化阶段提供 isDistinct(v1, v2) 判别能力。
 */
public class AliasAnalysis {

    private static class PropagationEdge {
        final Value target;
        final Value source1;
        final Value source2;

        PropagationEdge(Value to, Value from1) {
            this.target = to;
            this.source1 = from1;
            this.source2 = null;
        }

        PropagationEdge(Value to, Value from1, Value from2) {
            this.target = to;
            this.source1 = from1;
            this.source2 = from2;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (!(o instanceof PropagationEdge e)) return false;
            return Objects.equals(target, e.target) &&
                    Objects.equals(source1, e.source1) &&
                    Objects.equals(source2, e.source2);
        }

        @Override
        public int hashCode() {
            return Objects.hash(target, source1, source2);
        }
    }

    private static Map<Function, AliasTracker> func2AliasTracker = new HashMap<>();

    public static Function curFunction;

    public static boolean notAlias(Value a, Value b) {
        if (curFunction == null) return false;
        if (!func2AliasTracker.containsKey(curFunction)) executeOnFunc(curFunction);
        return func2AliasTracker.get(curFunction).notAlias(a, b);
    }

    public static boolean mustAlias(Value a, Value b) {
        if (curFunction == null) return false;
        if (!func2AliasTracker.containsKey(curFunction)) executeOnFunc(curFunction);
        return func2AliasTracker.get(curFunction).mustAlias(a, b);
    }

    public static void execute(List<Function> functions) {
        func2AliasTracker.clear();
        for (Function function : functions) {
            executeOnFunc(function);
        }
    }

    public static void executeOnFunc(Function func) {
        /**
         * opt: false，只传播【全局变量的gep】 not alias 属性
         * opt: true，传播【函数内所有gep】 not alias 属性
         */
        boolean opt = true;

        AliasTracker aliasTracker = new AliasTracker();
        func2AliasTracker.put(func, aliasTracker);
        int currentId = 0;

        int globalRoot = currentId++;
        int stackRoot = currentId++;
        aliasTracker.markTagsDisjoint(globalRoot, stackRoot);

        Set<Integer> globalGroup = new HashSet<>();
        Set<Integer> stackGroup = new HashSet<>();

        Map<Value, List<PropagationEdge>> dependencyGraph = new HashMap<>();
        Queue<Value> workQueue = new ArrayDeque<>();
        Set<Value> queued = new HashSet<>();

        Map<GEPInstr, Value> gep2Base = new HashMap<>();

        // Global variable tagging
        for (GlbObjPtr g : Visitor.getGlobalVariables()) {
            int id = currentId++;
            aliasTracker.bindTags(g, Set.of(globalRoot, id));
            globalGroup.add(id);
        }

        int argGroupId = currentId++;
        for (Function.FParam param : func.getFParams()) {
            if (param.getType() instanceof TPointer) {
                aliasTracker.bindTags(param, Set.of(argGroupId));
            }
        }

        for (BasicBlock block : FunctionInfoCollector.getFuncInfo(func).getDomBfsOrder()) {
            for (Instruction instr : block.getInstrList()) {
                if (!(instr.getType() instanceof TPointer)) continue;

                if (instr instanceof AllocaInstr) {
                    int id = currentId++;
                    aliasTracker.bindTags(instr, Set.of(stackRoot, id));
                    aliasTracker.markTagsDisjoint(id, argGroupId);
                    stackGroup.add(id);

                } else if (instr instanceof GEPInstr gep) {
                    Set<Integer> localAttrs = new HashSet<>();
                    GEPInstr walker = gep;
                    Value base = walker.getPtrVal();

                    while (true) {
                        Value offset = walker.getIndexList().getLast();

                        if (offset instanceof IntConst c) {
                            // 相当于无偏移，gep 和 base 等价（alias）
                            if (c.getConstInt() == 0) {
                                dependencyGraph.computeIfAbsent(gep, k -> new ArrayList<>()).add(
                                        new PropagationEdge(gep, base)
                                );
                                break;
                            } else {
                                // 偏移非 0，意味着不同地址
                                int tag1 = currentId++;
                                int tag2 = currentId++;
                                aliasTracker.markTagsDisjoint(tag1, tag2);
                                localAttrs.add(tag1);
                                aliasTracker.appendTag(base, tag2);
                            }
                        }

                        if (base instanceof GEPInstr next) {
                            walker = next;
                        } else break;
                    }

                    if (opt)
                        gep2Base.put(gep, base);
                    else if (base instanceof GlbObjPtr && !opt) {
                        gep2Base.put(gep, base);
                    }

                    // 在 offset 不可解析时，为该 gep 添加一个唯一的 tag
                    if (localAttrs.isEmpty()) {
                        int symbolicTag = currentId++;
                        localAttrs.add(symbolicTag);
                    }
                    aliasTracker.bindTags(gep, localAttrs);

                } else if (instr instanceof Bitcast) {
                    aliasTracker.bindTags(instr, Set.of());

                } else if (instr instanceof LoadInstr || instr instanceof CallInstr) {
                    aliasTracker.bindTags(instr, Set.of());

                } else if (instr instanceof PhiInstr phi) {
                    aliasTracker.bindTags(phi, Set.of());
                    Set<Value> inherited = new HashSet<>();
                    for (Value v : phi.getOperandMap().values()) {
                        if (!(v instanceof IRConst)) inherited.add(v);
                        if (inherited.size() > 2) break;
                    }

                    List<Value> list = new ArrayList<>(inherited);
                    if (list.size() == 1) {
                        dependencyGraph.computeIfAbsent(phi, k -> new ArrayList<>()).add(
                                new PropagationEdge(phi, list.get(0))
                        );
                    } else if (list.size() >= 2) {
                        dependencyGraph.computeIfAbsent(phi, k -> new ArrayList<>()).add(
                                new PropagationEdge(phi, list.get(0), list.get(1))
                        );
                    }
                }
            }
        }

        // Type-Based Alias Analysis (TBAA)
        Map<TPointer, Integer> typeTagMap = new HashMap<>();
        for (var value : aliasTracker.valueTagMap.keySet()) {
            TPointer ptrType = (TPointer) value.getType();
            if (!typeTagMap.containsKey(ptrType)) {
                typeTagMap.put(ptrType, currentId++);
            }
        }

        for (var typeA : typeTagMap.keySet()) {
            Type innerA = typeA.getReferencedType();
            for (var typeB : typeTagMap.keySet()) {
                if (typesAreDistinct(innerA, typeB.getReferencedType())) {
                    aliasTracker.markTagsDisjoint(typeTagMap.get(typeA), typeTagMap.get(typeB));
                }
            }
        }

        for (var value : aliasTracker.valueTagMap.keySet()) {
            int tag = typeTagMap.get((TPointer) value.getType());
            aliasTracker.appendTag(value, tag);
        }

        aliasTracker.addDisjointGroup(globalGroup);
        aliasTracker.addDisjointGroup(stackGroup);
        // 必须在标记完前两组内 disjoint 后进行这一步gep传播
        // 简化实现，由于global一定not alias, 只传播【全局变量的gep】 not alias 属性
        if (!opt) {
            for (Map.Entry<GEPInstr, Value> entry1 : gep2Base.entrySet()) {
                for (Map.Entry<GEPInstr, Value> entry2 : gep2Base.entrySet()) {
                    if (aliasTracker.notAlias(entry1.getValue(), entry2.getValue())) {
                        aliasTracker.markTagsDisjoint(aliasTracker.getTags(entry1.getKey()), aliasTracker.getTags(entry2.getKey()));
                    }
                }
            }
        }
        // 完整实现, 传播【函数内所有gep】 not alias 属性
        else {
            for (Value v1 : gep2Base.keySet()) {
                for (Value v2 : gep2Base.keySet()) {
                    if (aliasTracker.notAlias(gep2Base.get(v1), gep2Base.get(v2))) {
                        aliasTracker.markTagsDisjoint(aliasTracker.getTags(v1), aliasTracker.getTags(v2));
                    }
                }
            }
        }

        // Attribute propagation
        workQueue.addAll(dependencyGraph.keySet());

        while (!workQueue.isEmpty()) {
            Value target = workQueue.poll();
            for (PropagationEdge edge : dependencyGraph.getOrDefault(target, List.of())) {
                boolean modified;
                if (edge.source2 != null) {
                    Set<Integer> left = aliasTracker.getTags(edge.source1);
                    Set<Integer> right = aliasTracker.getTags(edge.source2);
                    Set<Integer> common = new HashSet<>(left);
                    common.retainAll(right);
                    modified = aliasTracker.appendTags(target, common);
                } else {
                    modified = aliasTracker.appendTags(target, aliasTracker.getTags(edge.source1));
                }

                if (modified && queued.add(target)) {
                    workQueue.add(target);
                }
            }
        }
    }

    private static boolean typesMayContain(Type superType, Type subType) {
        if (superType.equals(subType)) return true;
        if (superType instanceof TInt || superType instanceof TFloat) return false;
        if (superType instanceof TArray arr) {
            Type inner = subType instanceof TArray ? arr.getElementType() : arr.getBaseElementType();
            return typesMayContain(inner, subType);
        }
        if (superType instanceof TChar || subType instanceof TChar)
            return false; // 保守处理，认为可能alias任何类型
        return false;   // 反正处理不了了
    }

    private static boolean typesAreDistinct(Type a, Type b) {
        return !typesMayContain(a, b) && !typesMayContain(b, a);
    }
}
