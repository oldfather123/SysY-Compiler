package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.instr.AllocaInstr;
import ir.instr.LoadInstr;
import ir.instr.PhiInstr;
import ir.instr.StoreInstr;
import ir.type.IrType;
import ir.type.PointerType;
import ir.type.Ty;
import ir.value.*;
import pass.Pass;
import pass.utils.DominatorTree;

import java.util.*;

public class Mem2Reg implements Pass.IrPass {
    private final List<Value> allocas = new LinkedList<>();
    private final Map<Value, Set<BasicBlock>> allocaDefs = new HashMap<>();
    private final Map<PhiInstr, Value> newPhis = new HashMap<>();
    private final Map<BasicBlock, List<BasicBlock>> dominatorFrontiers = new HashMap<>();

    private void initDominators(Function function) {
        dominatorFrontiers.clear();
        DominatorTree domTree = function.getDomTree();
        // System.err.println("Dom Tree:\n" + domTree);

        Deque<BasicBlock> worklist = new LinkedList<>();
        HashSet<BasicBlock> visited = new HashSet<>();
        worklist.push(function.getBasicBlocks().getFirst());
        while (!worklist.isEmpty()) {
            BasicBlock bb = worklist.pop();
            if (visited.contains(bb)) {
                continue;
            }
            dominatorFrontiers.put(bb, new LinkedList<>());
            visited.add(bb);
            for (var successor : bb.getSuccessors()) {
                // In the algo <https://buaa-se-compiling.github.io/miniSysY-tutorial/challenge/mem2reg/help.html>
                // a == bb, b == successor
                BasicBlock x = bb;
                while (x != null && !domTree.isAncestor(x, successor)) {
                    dominatorFrontiers.get(x).add(successor);
                    x = domTree.getParent(x);
                }
                worklist.push(successor);
            }
        }
    }

    private void initAllocas(Function function) {
        allocas.clear();
        allocaDefs.clear();

        for (var bb : function.getBasicBlocks()) {
            for (var instr : bb.getInstrs()) {
                if (instr instanceof AllocaInstr alloca) {
                    if (alloca.getBaseType().is("i32") || alloca.getBaseType().is("float")) {
                        allocas.add(alloca.getInitialValue());
                    }
                }
                if (instr instanceof StoreInstr store) {
                    if (allocas.contains(store.getPointer())) {
                        allocaDefs.computeIfAbsent(store.getPointer(), k -> new LinkedHashSet<>()).add(bb);
                    }
                }
            }
        }
    }

    static IrType getBaseTypeOfAlloca(Value alloca) {
        return ((PointerType) alloca.getType()).getBaseType();
    }

    private void run(Function function) {
        /// - 阶段一：放置 PHI
        ///   - 初始化一个 map，由 phi 映射到它对应的 alloca（newPhis）
        newPhis.clear();
        ///   - 遍历每一个 alloca：
        for (var alloca : allocas) {
            ///     - 初始化全部 bb 为“未访问”
            Set<BasicBlock> visited = new HashSet<>();
            ///     - worklist <- allocaDef[alloca]
            Deque<BasicBlock> worklist = new LinkedList<>(allocaDefs.getOrDefault(alloca, new LinkedHashSet<>()));
            ///     - while worklist 不为空：
            while (!worklist.isEmpty()) {
                ///       - bb <- worklist.pop()
                BasicBlock bb = worklist.pop();
                ///       - for each 'dominator frontiers' of bb as df：
                for (var df : dominatorFrontiers.get(bb)) {
                    //////// Following algorithm is supplied by Copilot
                    ///         - if df 未访问：
                    ///           - df.visited <- true
                    ///           - if df 中有 store 到 alloca 的指令：
                    ///             - df.phi <- phi
                    ///             - newPhis[phi] <- alloca
                    ///           - else:
                    ///             - worklist.push(df)
                    //////// End of Copilot's algorithm
                    ///         - if df 未访问：
                    if (!visited.contains(df)){
                        visited.add(df);
                        ///           - phi <- new phi (in bb)
                        PhiInstr phi = new PhiInstr(new Intermediate(getBaseTypeOfAlloca(alloca)), getBaseTypeOfAlloca(alloca), new ArrayList<>(), new ArrayList<>());
                        df.insertHeadInstr(phi);
                        ///           - newPhis[phi] <- alloca
                        newPhis.put(phi, alloca);
                        ///           - worklist.push(df)
                        worklist.push(df);
                    }
                    //////// Blog's algorithm ends here, he did not set the visited flag of df or bb
                }
            }
        }
        /// - 阶段二：重命名
        ///   - 将每一个 bb 标记为“未访问”
        HashSet<BasicBlock> visited = new HashSet<>();
        ///   - 初始化一个 dict，由 bb 映射到它对应的 incoming values（worklist）
        Deque<Map<BasicBlock, Map<Value, Value>>> worklist = new LinkedList<>();
        ///   - worklist <- {(entry bb of function, [???])}
        worklist.push(Map.of(function.getBasicBlocks().getFirst(), new HashMap<>()));
        ///   - while worklist 不为空：
        Map<Value, Value> replaceMap = new HashMap<>();
        while (!worklist.isEmpty()){
            ///     - bb, incoming <- worklist.pop()
            var entry = worklist.pop();
            BasicBlock bb = entry.keySet().iterator().next();
            Map<Value, Value> incoming = entry.get(bb);
            ///     - 若 bb 已访问：continue;
            if (visited.contains(bb)) {
                continue;
            }
            ///     - bb.visited <- true
            visited.add(bb);
            ///     - for each instr in bb:
            var iter = bb.getInstrs().iterator();
            while (iter.hasNext()) {
                var instr = iter.next();
                ///       - match type of instr:
                for (var replaceEntry : replaceMap.entrySet()) {
                    // TODO: Record uses to optimal it!
                    instr.replaceOperand(replaceEntry.getKey(), replaceEntry.getValue());
                }
                if (instr instanceof AllocaInstr alloca) {
                    ///         - alloca: remove the instr from bb
                    if (!allocas.contains(alloca.getInitialValue())) {
                        continue;
                    }
                    iter.remove();
                } else if (instr instanceof LoadInstr load) {
                    ///         - load where allocas.contains(instr.addr):
                    if (!allocas.contains(load.getPointer())) {
                        continue;
                    }
                    ///             - replace all use of instr with incoming[instr.addr]
                    replaceMap.put(load.getInitialValue(), incoming.get(load.getPointer()));
                    ///             - remove instr from bb
                    iter.remove();
                } else if (instr instanceof StoreInstr store) {
                    ///         - store where allocas.contains(instr.addr):
                    if (!allocas.contains(store.getPointer())) {
                        continue;
                    }
                    ///             - incoming[instr.addr] <- instr.data
                    incoming.put(store.getPointer(), store.getValue());
                    ///             - remove instr from bb
                    iter.remove();
                } else if (instr instanceof PhiInstr phi) {
                    ///         - phi where newPhis.contains(instr):
                    if (!newPhis.containsKey(phi)) {
                        continue;
                    }
                    ///             - alloca <- newPhis[instr]
                    Value alloca = newPhis.get(phi);
                    ///             - incoming[alloca] <- instr
                    incoming.put(alloca, phi.getInitialValue());
                }
            }
            ///       - for each successor of bb:
            for (var successor : bb.getSuccessors()) {
                ///         - worklist.push((successor, incoming))
                worklist.push(Map.of((BasicBlock) successor, new HashMap<>(incoming)));
                ///         - for each phi in successor:
                for (var innerInstr : ((BasicBlock) successor).getInstrs()) {
                    if (innerInstr instanceof PhiInstr phi) {
                        ///           - if phi in newPhis:
                        if (newPhis.containsKey(phi)) {
                            ///             - alloca <- newPhis[phi]
                            Value alloca = newPhis.get(phi);
                            ///             - add (incoming[alloca], bb) into phi
                            if (incoming.containsKey(alloca)) {
                                phi.addIncoming(bb, incoming.get(alloca));
                            } else {
                                phi.addIncoming(bb, Literal.zero(((PointerType) alloca.getType()).getBaseType()));
                            }
                        }
                    }
                }
            }
        }
        for (var bb : function.getBasicBlocks()) {
            if (!visited.contains(bb)) {
                for (var instr : bb.getInstrs()) {
                    for (var replaceEntry : replaceMap.entrySet()) {
                        // TODO: Record uses to optimal it!
                        instr.replaceOperand(replaceEntry.getKey(), replaceEntry.getValue());
                    }
                }
            }
        }
    }

    @Override
    public void run(IrModule module) {
        for (var function : module.getFunctions()) {
            /// Mem2Reg 算法实现，Reference: <Roife Blog, https://roife.github.io/posts/mem2reg-pass/>
            /// - 初始化内容：
            initAllocas(function);
            ///   - 一个需要被 promote 的 alloca's 的集合 (allocas)
            // System.err.println("Mem2Reg: " + allocas.size() + " allocas to promote");
            // System.err.println(allocas);
            ///   - 一个 map，由 alloca 映射到 stores 的 bb（allocaDefs）
            // System.err.println("Mem2Reg: " + allocaDefs.size() + " allocaDefs");
            // System.err.println(allocaDefs.entrySet());

            initDominators(function);
            // System.err.println(dominatorFrontiers.entrySet());

            run(function);
        }
    }

    @Override
    public String getName() {
        return "Mem2Reg";
    }
}
