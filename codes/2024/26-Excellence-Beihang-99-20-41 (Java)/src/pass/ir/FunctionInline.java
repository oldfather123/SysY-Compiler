package pass.ir;

import driver.Config;
import ir.IrInstr;
import ir.IrModule;
import ir.instr.PhiInstr;
import ir.instr.TermInstr;
import ir.value.*;
import pass.Pass;
import utils.Pair;
import utils.Panic;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.stream.Collectors;

public class FunctionInline implements Pass.IrPass {
    private final HashMap<Function, List<Function>> caller2Callee = new HashMap<>();
    private final HashMap<Function, List<Function>> callee2Caller = new HashMap<>();
    private final HashMap<Pair<Function, Function>, List<IrInstr>> callMap = new HashMap<>();
    private IrModule module = null;

    @Override
    public void run(IrModule module) {
        this.module = module;
        for (Function function : module.getFunctions()) {
            // TODO: Maybe we can invalidate it when and only when we inline the function
            function.invalidateDomTree();
            function.invalidateLoopInfo();
        }

        run();
    }

    private void buildCallGraph() {
        callee2Caller.clear();
        caller2Callee.clear();
        callMap.clear();
        for (Function function : module.getFunctions()) {
            caller2Callee.put(function, new ArrayList<>());
            callee2Caller.put(function, new ArrayList<>());
        }
        for (Function function : module.getFunctions()) {
            for (BasicBlock block : function.getBasicBlocks()) {
                for (IrInstr instr : block.getInstrs()) {
                    if (instr instanceof TermInstr termInstr) {
                        if (termInstr.op.equals(IrInstr.OpCode.CALL)) {
                            Function callee = (Function) termInstr.getValue();
                            if (!caller2Callee.containsKey(callee)) {
                                continue;
                            }
                            caller2Callee.get(function).add(callee);
                            callee2Caller.get(callee).add(function);
                            callMap.computeIfAbsent(new Pair<>(function, callee), k -> new ArrayList<>()).add(instr);
                        }
                    }
                }
            }
        }
        for (var entry : callee2Caller.entrySet()) {
            if (!entry.getKey().getName().equals("@main") && entry.getValue().isEmpty()) {
                module.removeFunction(entry.getKey());
                if (Config.DebugSettings.displayFunctionInline) {
                    System.err.println("Remove unused function: " + entry.getKey().getName());
                }
            }
        }
    }

    private boolean changed = true;

    public void run() {
        while (changed) {
            changed = false;
            buildCallGraph();
            for (Function function : module.getFunctions().stream().filter(this::inlineable).toList()) {
                if (!callee2Caller.get(function).isEmpty()) {
                    inline(function);
                }
            }
        }
        buildCallGraph();  // Finally, rebuild the call graph to remove unused functions
    }

    private void inline(Function function) {
        changed = true;
        for (Function caller : callee2Caller.get(function)) {
            for (IrInstr call : callMap.get(new Pair<>(caller, function))) {
                for (BasicBlock block : caller.getBasicBlocks()) {
                    for (IrInstr instr : block.getInstrs()) {
                        if (instr.equals(call)) {
                            doInline(caller, function, (TermInstr) call, block);
                            break;
                        }
                    }
                }
            }
        }
    }

    private static int inlineID = 0;

    private void doInline(Function caller, Function callee, TermInstr call, BasicBlock callBlock) {
        inlineID++;
        if (Config.DebugSettings.displayFunctionInline) {
            System.err.println("Inline " + callee.getName() + " to " + caller.getName() + ":");
            System.err.println("  Call Instr: " + call + " in " + callBlock + " with inline id " + inlineID);
        }

        // 1. Split the caller block
        BasicBlock afterBlock = new BasicBlock(callBlock.getName() + "_i" + inlineID);
        // 1.1 Modify the phi node
        TermInstr termInstr = (TermInstr) callBlock.getTerminator();
        if (!termInstr.op.equals(IrInstr.OpCode.RET)) {
            for (BasicBlock block : termInstr.getJumpTargets()) {
                for (IrInstr instr : block.getInstrs()) {
                    if (instr instanceof PhiInstr phi) {
                        phi.replaceBasicBlock(callBlock.getName(), afterBlock);
                    } else {
                        break;
                    }
                }
            }
        }
        // 1.2 Move the instructions
        caller.insertBasicBlock(callBlock.getName(), afterBlock);
        var iter = callBlock.getInstrs().iterator();
        while (iter.hasNext()) {
            if (iter.next().equals(call)) {
                iter.remove();
                break;
            }
        }
        while (iter.hasNext()) {
            var instr = iter.next();
            afterBlock.insertTailInstr(instr);
            iter.remove();
        }

        // 2. Replace the arguments
        HashMap<String, Value> argMap = new HashMap<>();
        for (int i = 0; i < callee.getArguments().size(); i++) {
            Argument arg = callee.getArguments().get(i);
            Value value = call.getArguments().get(i);
            argMap.put(arg.getName(), value);
        }

        // 3. Copy the callee and insert it into the caller
        var shadow = shadowFunction(callee, argMap, afterBlock);
        List<BasicBlock> newFunction = shadow.getFirst();
        List<Pair<BasicBlock, Value>> retMap = shadow.getSecond();
        callBlock.insertTailInstr(new TermInstr(newFunction.get(0), IrInstr.OpCode.BR));
        caller.insertBasicBlocks(afterBlock.getName(), newFunction);

        // 4. Replace the return value
        if (call.getInitialValue() != null) {
            afterBlock.insertHeadInstr(new PhiInstr(
                    call.getInitialValue(), call.getInitialValue().getType(),
                    retMap.stream().map(Pair::getFirst).collect(Collectors.toCollection(ArrayList::new)),
                    retMap.stream().map(Pair::getSecond).collect(Collectors.toCollection(ArrayList::new))
            ));
        }
    }

    private Pair<List<BasicBlock>, List<Pair<BasicBlock, Value>>> shadowFunction(
            Function function, HashMap<String, Value> argMap, BasicBlock afterBlock
    ) {
        List<BasicBlock> shadowBlocks = new ArrayList<>();
        List<Pair<BasicBlock, Value>> retMap = new ArrayList<>();
        for (BasicBlock block : function.getBasicBlocks()) {
            BasicBlock shadowBlock = new BasicBlock(block.getName() + "_s" + inlineID);
            shadowBlocks.add(shadowBlock);
        }
        for (int i = 0; i < function.getBasicBlocks().size(); i++) {
            BasicBlock block = function.getBasicBlocks().get(i);
            BasicBlock shadowBlock = shadowBlocks.get(i);
            for (IrInstr instr : block.getInstrs()) {
                IrInstr shadowInstr;
                try {
                    shadowInstr = instr.copy();
                } catch (CloneNotSupportedException e) {
                    shadowInstr = null;
                    Panic.panic("Failed to clone instruction");
                }
                for (Value operand : shadowInstr.getOperands()) {
                    if (operand != null && operand.getName().startsWith("%p") && argMap.containsKey(operand.getName())) {
                        shadowInstr.replaceOperand(operand, argMap.get(operand.getName()));
                    } else if (operand instanceof Intermediate && !(operand instanceof Literal) && !(operand.getName().startsWith("@"))) {
                        shadowInstr.replaceOperand(operand, new Intermediate("%s" + inlineID + "_" + operand.getName().substring(1), operand.getType()));
                    } else if (operand instanceof BasicBlock) {
                        shadowInstr.replaceOperand(operand, shadowBlocks.get(function.getBasicBlocks().indexOf(operand)));
                    } else if (operand != null && !(operand instanceof Literal) && !(operand instanceof Function) && !(operand.getName().startsWith("@"))) {
                        Panic.panic("Unexpected operand type in shadow: " + operand + " in `" + shadowInstr + "`");
                    }
                }
                if (shadowInstr instanceof PhiInstr phi) {
                    shadowInstr = new PhiInstr(
                            phi.getInitialValue(), phi.getInitialValue().getType(),
                            phi.getIncoming().keySet().stream().map(b -> shadowBlocks.get(function.getBasicBlocks().indexOf(b))).collect(Collectors.toCollection(ArrayList::new)),
                            new ArrayList<>(phi.getIncoming().values())
                    );
                }
                if (shadowInstr instanceof TermInstr termInstr && termInstr.op.equals(IrInstr.OpCode.RET)) {
                    if (termInstr.isVoid) {
                        shadowBlock.insertTailInstr(new TermInstr(afterBlock, IrInstr.OpCode.BR));
                    } else {
                        shadowBlock.insertTailInstr(new TermInstr(afterBlock, IrInstr.OpCode.BR));
                        retMap.add(new Pair<>(shadowBlock, termInstr.getValue()));
                    }
                } else {
                    shadowBlock.insertTailInstr(shadowInstr);
                }
            }
        }
        return new Pair<>(shadowBlocks, retMap);
    }

    private boolean inlineable(Function callee) {
        return !callee.getName().equals("@main") && caller2Callee.get(callee).isEmpty();
    }

    @Override
    public String getName() {
        return "function-inline";
    }
}
