package midend;

import frontend.ir.Value;
import frontend.ir.constant.IRConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.binop.MulInstr;
import frontend.ir.instr.binop.SRemInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.Expr.SCEVAddRecExpr;
import midend.Analysis.SCEV.Expr.SCEVSRemRecExpr;
import midend.Analysis.SCEV.SCEVManager;

import java.util.HashSet;
import java.util.List;

public class SCEVSimplify {
    private static final HashSet<AddInstr> mayOverflow = new HashSet<>();
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            simplifyFunc(function);
        }
    }

    public static void simplifyFunc(Function function) {
        SCEVManager manager = function.getSCEVManager();
        for (LoopInfo loopInfo : function.getAncientLoopInfo()) {
            simplifyRecLauncher(loopInfo, manager);
        }
    }

    public static void simplifyRecLauncher(LoopInfo loop, SCEVManager manager) {
        for (LoopInfo childLoop : loop.getChildLoop()) {
            simplifyRecLauncher(childLoop, manager);
        }
        mayOverflow.clear();
        for (Instruction instr : loop.getLoopHeader().getInstrList()) {
            if (instr instanceof PhiInstr phi) {
                if (manager.getSCEV(phi) instanceof SCEVSRemRecExpr scevsRemRecExpr) {
                    sremRecSimplify(loop, scevsRemRecExpr, manager);
                }
            } else {
                break;
            }
        }
        for (Instruction instr : loop.getLoopHeader().getInstrList()) {
            if (instr instanceof PhiInstr phi) {
                if (manager.getSCEV(phi) instanceof SCEVAddRecExpr scevAddRecExpr) {
                    addRecSimplify(loop, scevAddRecExpr, manager);
                }
            } else {
                break;
            }
        }
    }

    public static void sremRecSimplify(LoopInfo loopInfo, SCEVSRemRecExpr sremRecExpr, SCEVManager manager) {
        AddInstr addInstr = sremRecExpr.addExpr;
        if (addInstr.getUserList().size() > 1 || !loopInfo.isSimpleLoop()) {
            return;
        }
        PhiInstr sremPhi = (PhiInstr) sremRecExpr.getValueProjected();
        SRemInstr inner = (SRemInstr) sremPhi.getOperandMap().get(loopInfo.getLatchBlocks().iterator().next());

        if (inner.getUserList().size() > 1) {
            return;
        }

        for (Value user : sremPhi.getUserList()) {
            if (user instanceof Instruction ins && ins.getParentBB().isContainsInLoop(loopInfo)) {
                if (user != addInstr) {
                    return;
                }
            }
        }

        // 找到srem的LCSSA
        PhiInstr lcssa = null;
        for (Instruction instruction : loopInfo.getExitTargets().iterator().next().getInstrList()) {
            if (instruction instanceof PhiInstr phi && phi.isLCSSA()) {
                if (phi.getOperandMap().containsValue(sremPhi)) {
                    lcssa = phi;
                    break;
                }
            }
        }
        if (lcssa == null) {
            return;
        }
        // 在LCSSA后加一条srem，并做replace
        SRemInstr outer = new SRemInstr(
                loopInfo.getFunction().getAndAddRegIdx(), lcssa, inner.getOperand2(),
                lcssa.getParentBB());
        lcssa.replaceUseWith(outer);

        outer.setUse(lcssa);
        outer.setUse(inner.getOperand2());
        outer.insertBefore(lcssa.getParentBB().getLastInstr());

        sremPhi.modifyUse(inner, addInstr);
        inner.getUserList().remove(sremPhi);
        addInstr.getUserList().add(sremPhi);
        inner.removeFromList();

        manager.getSCEVForceRecompute(sremPhi);
        mayOverflow.add(addInstr);
    }

    public static void addRecSimplify(LoopInfo loopInfo, SCEVAddRecExpr addRecExpr, SCEVManager manager) {
        if (!(loopInfo.isSimpleLoop() && loopInfo.isConditionSimple())) {
            return;
        }
        if (loopInfo.loopCondition.tripCountExpr.getValueProjected() == null ||
        addRecExpr.operands.getFirst().getValueProjected() == null) {
            return;
        }
        PhiInstr addPhi = (PhiInstr) addRecExpr.getValueProjected();
        AddInstr addInstr = (AddInstr) addPhi.getOperandMap().get(loopInfo.getLatchBlocks().iterator().next());
        if (addInstr.getUserList().size() > 1) {
            return;
        }
        Value step;
        if (addInstr.getOperand1() == addPhi) {
            if (addInstr.getOperand2() instanceof Instruction ins && ins.getParentBB().isContainsInLoop(loopInfo)) {
                return;
            } else {
                step = addInstr.getOperand2();
            }
        } else {
            if (addInstr.getOperand1() instanceof Instruction ins && ins.getParentBB().isContainsInLoop(loopInfo)) {
                return;
            } else {
                step = addInstr.getOperand1();
            }
        }

        if (!(step instanceof IRConst)) {
            return;
        }

        for (Value user : addPhi.getUserList()) {
            if (user instanceof Instruction ins && ins.getParentBB().isContainsInLoop(loopInfo)) {
                if (user != addInstr) {
                    return;
                }
            }
        }
        // 找到addPhi的LCSSA
        PhiInstr lcssa = null;
        for (Instruction instruction : loopInfo.getExitTargets().iterator().next().getInstrList()) {
            if (instruction instanceof PhiInstr phi && phi.isLCSSA()) {
                if (phi.getOperandMap().containsValue(addPhi)) {
                    lcssa = phi;
                    break;
                }
            }
        }
        if (lcssa == null) {
            return;
        }
        Value count = loopInfo.loopCondition.tripCountExpr.getValueProjected();
        if (mayOverflow.contains(addInstr)) {
            if (lcssa.getUserList().size() > 1) {
                throw new RuntimeException("这是不允许的");
            } else {
                SRemInstr sRemInstr = (SRemInstr) lcssa.getUserList().getFirst();
                SRemInstr sremA = new SRemInstr(
                        loopInfo.getFunction().getAndAddRegIdx(), step, sRemInstr.getOperand2(),
                        lcssa.getParentBB());
                sremA.setUse(step);
                sremA.setUse(sRemInstr.getOperand2());
                sremA.insertBefore(lcssa.getParentBB().getLastInstr());
                SRemInstr sremB = new SRemInstr(
                        loopInfo.getFunction().getAndAddRegIdx(), count, sRemInstr.getOperand2(),
                        lcssa.getParentBB());
                sremB.setUse(count);
                sremB.setUse(sRemInstr.getOperand2());
                sremB.insertBefore(lcssa.getParentBB().getLastInstr());

                step = sremA;
                count = sremB;
            }
        }

        // 在LCSSA后加一条乘法
        MulInstr mulInstr = new MulInstr(
                loopInfo.getFunction().getAndAddRegIdx(), step,
                count,
                lcssa.getParentBB());

        mulInstr.setUse(step);
        mulInstr.setUse(count);
        mulInstr.insertBefore(lcssa.getParentBB().getLastInstr());

        Value init = addRecExpr.operands.getFirst().getValueProjected();

        AddInstr outerAdd = new AddInstr(
                loopInfo.getFunction().getAndAddRegIdx(), init, mulInstr,
                lcssa.getParentBB());

        outerAdd.setUse(init);
        outerAdd.setUse(mulInstr);
        outerAdd.insertBefore(lcssa.getParentBB().getLastInstr());
        lcssa.replaceUseWith(outerAdd);
        lcssa.removeFromList();

        addPhi.forceRemoveFromList();
        addInstr.forceRemoveFromList();


    }
}
