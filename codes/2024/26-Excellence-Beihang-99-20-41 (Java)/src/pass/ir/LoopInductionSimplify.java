package pass.ir;

import driver.Config;
import ir.IrInstr;
import ir.IrModule;
import ir.instr.BinaInstr;
import ir.instr.PhiInstr;
import ir.value.*;
import pass.Pass;
import pass.utils.ControlFlowGraph;
import pass.utils.LoopInfo;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Objects;
import java.util.stream.Stream;

class InductiveVariable {
    private final IrInstr updateInstr;
    private final Value initialValue;
    private final Value step;
    private final IrInstr.OpCode opCode;
    private final Intermediate inductionVar;
    private boolean touchedUpdate = false;

    public InductiveVariable(IrInstr updateInstr, Value initialValue, Value step, IrInstr.OpCode opCode, Intermediate inductionVar) {
        this.updateInstr = updateInstr;
        this.initialValue = initialValue;
        this.step = step;
        this.opCode = opCode;
        this.inductionVar = inductionVar;
    }

    public void testUpdateInstr(IrInstr instr) {
        if (updateInstr.equals(instr)) {
            touchedUpdate = true;
        }
    }

    public boolean match(Value value) {
        return inductionVar.equals(value);
    }

    public List<BinaInstr> spawnSimplifiedInstr(Literal times, Intermediate newBaseVar, Intermediate result) {
        IrInstr.OpCode mul;
        if (opCode.equals(IrInstr.OpCode.ADD) || opCode.equals(IrInstr.OpCode.FADD)) {
            mul = IrInstr.OpCode.FMUL;
        } else {
            mul = IrInstr.OpCode.MUL;
        }
        if (step instanceof Literal stepLiteral) {
            if (mul.equals(IrInstr.OpCode.MUL)) {
                return List.of(
                        new BinaInstr(newBaseVar, new Literal(stepLiteral.asInt() * times.asInt()), opCode, result)
                );
            } else {
                return List.of(
                        new BinaInstr(newBaseVar, new Literal(stepLiteral.asFloat() * times.asFloat()), opCode, result)
                );
            }
        } else {
            Intermediate tmp = new Intermediate(step.getType());
            return List.of(
                new BinaInstr(newBaseVar, tmp, opCode, result)
            );
        }
    }

    public List<BinaInstr> spawnInitialInstr(Literal times, Intermediate newBaseVar) {
        IrInstr.OpCode mul;
        if (opCode.equals(IrInstr.OpCode.ADD) || opCode.equals(IrInstr.OpCode.FADD)) {
            mul = IrInstr.OpCode.FMUL;
        } else {
            mul = IrInstr.OpCode.MUL;
        }
        if (!touchedUpdate) {
            Intermediate tmp = new Intermediate(initialValue.getType());
            IrInstr.OpCode oppositeOp;
            if (opCode.equals(IrInstr.OpCode.ADD)) {
                oppositeOp = IrInstr.OpCode.SUB;
            } else if (opCode.equals(IrInstr.OpCode.FADD)) {
                oppositeOp = IrInstr.OpCode.FSUB;
            } else if (opCode.equals(IrInstr.OpCode.SUB)) {
                oppositeOp = IrInstr.OpCode.ADD;
            } else {
                oppositeOp = IrInstr.OpCode.FADD;
            }
            return List.of(
                new BinaInstr(initialValue, step, oppositeOp, tmp),
                new BinaInstr(tmp, times, mul, newBaseVar)
            );
        }
        return List.of(
            new BinaInstr(initialValue, times, mul, newBaseVar)
        );
    }

    @Override
    public String toString() {
        return String.format(
                "InductiveVariable{initialValue=%s, step=%s, opCode=%s, inductionVar=%s}",
                initialValue, step, opCode, inductionVar
        );
    }
}

public class LoopInductionSimplify implements Pass.IrPass {
    private final List<InductiveVariable> inductiveVariables = new LinkedList<>();

    @Override
    public void run(IrModule module) {
        for (var function : module.getFunctions()) {
            run(function);
        }
    }

    private void run(Function function) {
        ControlFlowGraph cfg = ControlFlowGraph.fromFunction(function);
        for (var topLoop : function.getLoops()) {
            visitLoop(topLoop, cfg);
            run(topLoop, cfg);
        }
    }

    private void visitLoop(LoopInfo loop, ControlFlowGraph cfg) {
        for (var subLoop : loop.getSubLoops()) {
            visitLoop(subLoop, cfg);
            run(subLoop, cfg);
        }
    }

    private void run(LoopInfo loop, ControlFlowGraph cfg) {
        if (!loop.isSimple(cfg)) {
            return;
        }
        inductiveVariables.clear();
        List<PhiInstr> headerPhis = new LinkedList<>();
        for (var instr : loop.getHeaderBlock().getInstrs()) {
            if (instr instanceof PhiInstr phi && phi.getIncoming().size() == 2) {
                headerPhis.add(phi);
            } else {
                break;
            }
        }
        for (var headerPhi : headerPhis) {
            IrInstr inLoopVarInstr = null;
            Value outLoopVar = null;
            for (var incoming : headerPhi.getIncoming().entrySet()) {
                if (loop.getBlocks().contains(incoming.getKey())) {
                    for (var instr : incoming.getKey().getInstrs()) {
                        if (Objects.equals(instr.getInitialValue(), incoming.getValue())) {
                            inLoopVarInstr = instr;
                            break;
                        }
                    }
                } else {
                    outLoopVar = incoming.getValue();
                }
            }
            if (inLoopVarInstr == null || outLoopVar == null) {
                continue;
            }
            if (!(inLoopVarInstr instanceof BinaInstr inLoopVarBina)) {
                continue;
            }
            if (Stream.of(
                    IrInstr.OpCode.ADD, IrInstr.OpCode.SUB,
                    IrInstr.OpCode.FADD, IrInstr.OpCode.FSUB
            ).noneMatch(op -> op.equals(inLoopVarBina.getOpCode()))) {
                continue;
            }
            Value step;
            if (inLoopVarInstr.getOperands()[1].equals(headerPhi.getInitialValue())) {
                step = inLoopVarInstr.getOperands()[2];
            } else if (inLoopVarInstr.getOperands()[2].equals(headerPhi.getInitialValue())) {
                step = inLoopVarInstr.getOperands()[1];
            } else {
                continue;
            }
            if (step instanceof Intermediate stepIntermediate && loop.getInvariants().contains(stepIntermediate)) {
                continue;
            }
            inductiveVariables.add(new InductiveVariable(
                    inLoopVarBina, outLoopVar, step, inLoopVarBina.getOpCode(),
                    (Intermediate) inLoopVarInstr.getInitialValue()
            ));
        }
        if (Config.DebugSettings.displayInductionVar) {
            System.err.println("Inductive Variables:");
            System.err.println("  " + inductiveVariables);
        }
        if (inductiveVariables.isEmpty()) {
            return;
        }
        simplify(inductiveVariables, loop.getLevelOrder(), loop.getHoistedBlock(), loop.getLatchBlocks().get(0));
    }

    private void simplify(List<InductiveVariable> indVars, List<BasicBlock> blocks, BasicBlock hoistedBlock, BasicBlock latchBlock) {
        for (var block : blocks) {
            var iter = block.getInstrs().listIterator();
            while (iter.hasNext()) {
                var instr = iter.next();
                if (!(instr instanceof BinaInstr binaInstr)) {
                    continue;
                }
                if (binaInstr.getOpCode().equals(IrInstr.OpCode.MUL)) {
                    Literal times = null;
                    Value baseVar = null;
                    InductiveVariable inductiveVariable = null;
                    if (binaInstr.val1 instanceof Literal literal) {
                        for (var indVar : indVars) {
                            if (indVar.match(binaInstr.val2)) {
                                times = literal;
                                baseVar = binaInstr.val2;
                                inductiveVariable = indVar;
                                break;
                            }
                        }
                    } else if (binaInstr.val2 instanceof Literal literal) {
                        for (var indVar : indVars) {
                            if (indVar.match(binaInstr.val1)) {
                                times = literal;
                                baseVar = binaInstr.val1;
                                inductiveVariable = indVar;
                                break;
                            }
                        }
                    }
                    if (times == null || baseVar == null) {
                        continue;
                    }
                    if (Config.DebugSettings.displayInductionVar) {
                        System.err.println("Simplified: " + instr + " with " + inductiveVariable);
                    }
                    // 1. Init Instruction in Hoisted Block
                    Intermediate initialVar = new Intermediate(
                            "%reduced.init." + instr.getInitialValue().getName().substring(1), instr.getInitialValue().getType()
                    );
                    for (var i : inductiveVariable.spawnInitialInstr(times, initialVar)) {
                        hoistedBlock.insertInstrBeforeLast(i);
                    }
                    // 2. Phi switch in Header Block
                    Intermediate reduceVar = new Intermediate(
                            "%reduced." + instr.getInitialValue().getName().substring(1), instr.getInitialValue().getType()
                    );
                    PhiInstr phiInstr = new PhiInstr(
                        reduceVar, reduceVar.type,
                        new ArrayList<>(List.of(hoistedBlock, latchBlock)),
                        new ArrayList<>(List.of(initialVar, instr.getInitialValue()))
                    );
                    hoistedBlock.getSuccessors()[0].insertHeadInstr(phiInstr);
                    // 3. Update Instruction in Hoisted Block and Original Block
                    List<BinaInstr> updateInstrs = inductiveVariable.spawnSimplifiedInstr(times, reduceVar, (Intermediate) instr.getInitialValue());
                    if (updateInstrs.size() == 1) {
                        iter.set(updateInstrs.get(0));
                    } else {
                        hoistedBlock.insertInstrBeforeLast(updateInstrs.get(0));
                        iter.set(updateInstrs.get(1));
                    }
                } else {
                    for (var indVar : indVars) {
                        indVar.testUpdateInstr(instr);
                    }
                }
            }
        }
    }

    @Override
    public String getName() {
        return "loop-inductive-variable-simplify";
    }
}
