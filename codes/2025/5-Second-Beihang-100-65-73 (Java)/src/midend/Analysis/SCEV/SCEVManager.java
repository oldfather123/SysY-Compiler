package midend.Analysis.SCEV;

import frontend.ir.Value;
import frontend.ir.constant.FloatConst;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.*;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.otherop.PhiInstr;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.Expr.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

/**
 * SCEV依赖于循环信息
 */
public class SCEVManager {

    public static final SCEVCouldNotCompute CouldNotCompute = SCEVCouldNotCompute.get();

    private final HashMap<Value, SCEVComputing> computingMap = new HashMap<>();
    private final HashMap<Value, SCEVExpr> cache = new HashMap<>();

    public static final HashSet<SCEVExpr> simplifying = new HashSet<>();
    private boolean isCollecting = false;

    public SCEVExpr getSCEVForceRecompute(Value val) {
        cache.remove(val);
        return getSCEV(val);
    }

    public SCEVExpr getSCEV(Value val) {

        if (computingMap.containsKey(val)) {
            return computingMap.get(val);
        }

        if (cache.containsKey(val)) return cache.get(val);
        SCEVExpr result;

        SCEVComputing computing = new SCEVComputing(val);
        computingMap.put(val, computing);

        switch (val) {
            case IntConst intConst -> result = new SCEVConstant(intConst.getConstInt().intValue());
            case FloatConst floatConst -> result = new SCEVFConstant(floatConst.getConstVal().floatValue());
            case AddInstr addInstr -> {
                SCEVExpr lhs = getSCEV(addInstr.getOperand1());
                SCEVExpr rhs = getSCEV(addInstr.getOperand2());
                result = new SCEVAddExpr(new ArrayList<>(List.of(lhs, rhs)), addInstr);
                if (lhs instanceof SCEVComputing computingOp) {
                    computingOp.addUser(result);
                }
                if (rhs instanceof SCEVComputing computingOp) {
                    computingOp.addUser(result);
                }
            }
            case SubInstr subInstr -> {
                SCEVExpr lhs = getSCEV(subInstr.getOperand1());
                SCEVExpr rhs = getSCEV(subInstr.getOperand2());
                result = new SCEVMinusExpr(new ArrayList<>(List.of(lhs, rhs)), subInstr);
                if (lhs instanceof SCEVComputing computingOp) {
                    computingOp.addUser(result);
                }
                if (rhs instanceof SCEVComputing computingOp) {
                    computingOp.addUser(result);
                }
            }

//        else if (val instanceof MulInstr) {
//            SCEVExpr lhs = getSCEV(((MulInstr) val).getOperand1());
//            SCEVExpr rhs = getSCEV(((MulInstr) val).getOperand2());
//            result = new SCEVMulExpr(List.of(lhs, rhs));
//        }

            case SDivInstr sDivInstr -> {
                SCEVExpr lhs = getSCEV(sDivInstr.getOperand1());
                SCEVExpr rhs = getSCEV(sDivInstr.getOperand2());
                result = new SCEVSdivExpr(new ArrayList<>(List.of(lhs, rhs)), sDivInstr);
                if (lhs instanceof SCEVComputing computingOp) {
                    computingOp.addUser(result);
                }
                if (rhs instanceof SCEVComputing computingOp) {
                    computingOp.addUser(result);
                }
            }

//        else if (val instanceof CastInst) {
//            CastInst cast = (CastInst) val;
//            result = new SCEVCastExpr(
//                    getSCEV(cast.operand),
//                    cast.kind,
//                    cast.fromBits,
//                    cast.toBits
//            );
//        }
            case PhiInstr phi -> {
                ArrayList<AddInstr> add = isVecAddRecPhi(phi);
//                ArrayList<FAddInstr> fAdd = isFVecAddRecPhi(phi);
//                ArrayList<SubInstr> sub = isVecMinusRecPhi(phi);
//                ArrayList<FSubInstr> fSub = isVecFMinusRecPhi(phi);
                if (add != null) {
                    result = buildVecAddRecFromPhi(phi, add);
//                } else if (fAdd != null) {
//                    result = buildFVecAddRecFromPhi(phi, fAdd);
//                } else if (sub != null) {
//                    result = buildVecMinusRecFromPhi(phi, sub);
//                } else if (fSub != null) {
//                    result = buildVecFMinusRecFromPhi(phi, fSub);
                } else if (isAddRecPhi(phi)) {
                    result = buildAddRecFromPhi(phi);
                } else if (isSRemRecPhi(phi)) {
                    result = buildSRemRecFromPhi(phi);
                } else {
                    result = new SCEVUnknown(val);
                }
            }
            case null, default -> result = new SCEVUnknown(val);
        }

        computing.clearUse();
        computingMap.remove(val);

        result = result.simplify();
        cache.put(val, result);
        return result;
    }

    public static SCEVCouldNotCompute getSCEVCouldNotCompute() {
        return CouldNotCompute;
    }

    private boolean isAddRecPhi(PhiInstr phi) {
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        if (loop == null) return false;
        if (loop.getLoopHeader() != phi.getParentBB()) return false;

        Value pre = phi.getOperandMap().get(loop.getPreHeader());
        Value recur = phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());
        if (pre == null || recur == null) return false;

        if (!(recur instanceof AddInstr add)) {
            return false;
        }

        return (add.getOperand1() == phi && add.getOperand2() != phi) ||
                (add.getOperand2() == phi && add.getOperand1() != phi);
    }

    private boolean isSRemRecPhi(PhiInstr phi) {
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        if (loop == null) return false;
        if (loop.getLoopHeader() != phi.getParentBB()) return false;

        Value pre = phi.getOperandMap().get(loop.getPreHeader());
        Value recur = phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());
        if (pre == null || recur == null) return false;

        if (!(recur instanceof SRemInstr sRemInstr && sRemInstr.getOperand1() instanceof AddInstr add)) {
            return false;
        }

        if (sRemInstr.getOperand2() instanceof Instruction ins && ins.getParentBB().isContainsInLoop(loop)) {
            return false;
        }

        return (add.getOperand1() == phi && add.getOperand2() instanceof IRConst) ||
                (add.getOperand2() == phi && add.getOperand1() instanceof IRConst);
    }

    private ArrayList<AddInstr> digFindAddInstr(AddInstr val, PhiInstr phi, int depth, ArrayList<AddInstr> path) {
        if (depth > 4) {
            return null;
        }
        if (val.getOperand1() == phi || val.getOperand2() == phi) {
            if (depth == 4) return path;
            return null;
        }
        if (val.getOperand1() instanceof AddInstr addInstr) {
            path.add(addInstr);
            return digFindAddInstr(addInstr, phi, depth + 1, path);
        } else if (val.getOperand2() instanceof AddInstr addInstr) {
            path.add(addInstr);
            return digFindAddInstr(addInstr, phi, depth + 1, path);
        }
        return null;
    }

    private ArrayList<AddInstr> isVecAddRecPhi(PhiInstr phi) {
        if (isCollecting) return null;
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        if (loop == null) return null;
        if (loop.getLoopHeader() != phi.getParentBB()) return null;

        Value pre = phi.getOperandMap().get(loop.getPreHeader());
        Value recur = phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());
        if (pre == null || recur == null) return null;

        if (!(recur instanceof AddInstr add)) {
            return null;
        }
        ArrayList<AddInstr> path = new ArrayList<>();
        path.add(add);
        return digFindAddInstr(add, phi, 1, path);
    }


    private SCEVAddRecExpr buildAddRecFromPhi(PhiInstr phi) {
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        Value init = phi.getOperandMap().get(loop.getPreHeader());
        Value recur = phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());

        AddInstr add = (AddInstr) recur;

        Value step = (add.getOperand1() == phi) ? add.getOperand2() : add.getOperand1();

        SCEVAddRecExpr result = new SCEVAddRecExpr(
                new ArrayList<>(List.of(getSCEV(init), getSCEV(step))),
                phi.getParentBB().getLoopBelonged(),
                phi
        );
        for (SCEVExpr op : result.operands) {
            if (op instanceof SCEVComputing computingOp) {
                computingOp.addUser(result);
            }
        }
        return result;
    }

    private SCEVSRemRecExpr buildSRemRecFromPhi(PhiInstr phi) {
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        Value init = phi.getOperandMap().get(loop.getPreHeader());
        SRemInstr recur = (SRemInstr) phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());
        AddInstr add = (AddInstr) recur.getOperand1();

        Value step = (recur).getOperand2();

        return new SCEVSRemRecExpr(
                new ArrayList<>(List.of(getSCEV(init), getSCEV(step))),
                phi.getParentBB().getLoopBelonged(),
                phi,
                add
        );
    }

    private SCEVVecAddRecExpr buildVecAddRecFromPhi(PhiInstr phi, ArrayList<AddInstr> addInstrs) {
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        Value init = phi.getOperandMap().get(loop.getPreHeader());
//        Value recur = phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());

        List<SCEVExpr> operands = new ArrayList<>();
        Value last = phi;
        operands.add(getSCEV(init));
        for (int i = addInstrs.size() - 1; i >= 0; i--) {
            if (addInstrs.get(i).getOperand1() == last) {
                operands.add(getSCEV(addInstrs.get(i).getOperand2()));
                last = addInstrs.get(i);
            } else {
                operands.add(getSCEV(addInstrs.get(i).getOperand1()));
                last = addInstrs.get(i);
            }
        }
        SCEVVecAddRecExpr result = new SCEVVecAddRecExpr(operands, addInstrs.reversed(), loop, phi);
        for (SCEVExpr op : operands) {
            if (op instanceof SCEVComputing computingOp) {
                computingOp.addUser(result);
            }
        }
        return result;
    }


    private ArrayList<FAddInstr> digFindFAddInstr(FAddInstr val, PhiInstr phi, int depth, ArrayList<FAddInstr> path) {
        if (depth > 4) {
            return null;
        }
        if (val.getOperand1() == phi || val.getOperand2() == phi) {
            if (depth == 4) return path;
            return null;
        }
        if (val.getOperand1() instanceof FAddInstr addInstr) {
            path.add(addInstr);
            return digFindFAddInstr(addInstr, phi, depth + 1, path);
        } else if (val.getOperand2() instanceof FAddInstr addInstr) {
            path.add(addInstr);
            return digFindFAddInstr(addInstr, phi, depth + 1, path);
        }
        return null;
    }

    private ArrayList<FAddInstr> isFVecAddRecPhi(PhiInstr phi) {
        if (isCollecting) return null;
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        if (loop == null) return null;
        if (loop.getLoopHeader() != phi.getParentBB()) return null;

        Value pre = phi.getOperandMap().get(loop.getPreHeader());
        Value recur = phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());
        if (pre == null || recur == null) return null;

        if (!(recur instanceof FAddInstr add)) {
            return null;
        }
        ArrayList<FAddInstr> path = new ArrayList<>();
        path.add(add);
        return digFindFAddInstr(add, phi, 1, path);
    }

    private SCEVFVecAddRecExpr buildFVecAddRecFromPhi(PhiInstr phi, ArrayList<FAddInstr> addInstrs) {
        isCollecting = true;
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        Value init = phi.getOperandMap().get(loop.getPreHeader());

        List<SCEVExpr> operands = new ArrayList<>();
        Value last = phi;
        operands.add(getSCEV(init));
        for (int i = addInstrs.size() - 1; i >= 0; i--) {
            if (addInstrs.get(i).getOperand1() == last) {
                operands.add(getSCEV(addInstrs.get(i).getOperand2()));
                last = addInstrs.get(i);
            } else {
                operands.add(getSCEV(addInstrs.get(i).getOperand1()));
                last = addInstrs.get(i);
            }
        }

        isCollecting = false;
        return new SCEVFVecAddRecExpr(operands, addInstrs.reversed(), loop, phi);
    }


    private ArrayList<SubInstr> digFindSubInstr(SubInstr val, PhiInstr phi, int depth, ArrayList<SubInstr> path) {
        if (depth > 4) {
            return null;
        }
        if (val.getOperand1() == phi || val.getOperand2() == phi) {
            if (depth == 4) return path;
            return null;
        }
        if (val.getOperand1() instanceof SubInstr subInstr) {
            path.add(subInstr);
            return digFindSubInstr(subInstr, phi, depth + 1, path);
        } else if (val.getOperand2() instanceof SubInstr subInstr) {
            path.add(subInstr);
            return digFindSubInstr(subInstr, phi, depth + 1, path);
        }
        return null;
    }

    private ArrayList<SubInstr> isVecMinusRecPhi(PhiInstr phi) {
        if (isCollecting) return null;
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        if (loop == null) return null;
        if (loop.getLoopHeader() != phi.getParentBB()) return null;

        Value pre = phi.getOperandMap().get(loop.getPreHeader());
        Value recur = phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());
        if (pre == null || recur == null) return null;

        if (!(recur instanceof SubInstr sub)) {
            return null;
        }
        ArrayList<SubInstr> path = new ArrayList<>();
        path.add(sub);
        return digFindSubInstr(sub, phi, 1, path);
    }

    private SCEVVecMinusRecExpr buildVecMinusRecFromPhi(PhiInstr phi, ArrayList<SubInstr> subInstrs) {
        isCollecting = true;
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        Value init = phi.getOperandMap().get(loop.getPreHeader());

        List<SCEVExpr> operands = new ArrayList<>();
        Value last = phi;
        operands.add(getSCEV(init));
        for (int i = subInstrs.size() - 1; i >= 0; i--) {
            if (subInstrs.get(i).getOperand1() == last) {
                operands.add(getSCEV(subInstrs.get(i).getOperand2()));
                last = subInstrs.get(i);
            } else {
                operands.add(getSCEV(subInstrs.get(i).getOperand1()));
                last = subInstrs.get(i);
            }
        }

        isCollecting = false;
        return new SCEVVecMinusRecExpr(operands, subInstrs.reversed(), loop, phi);
    }


    private ArrayList<FSubInstr> digFindFSubInstr(FSubInstr val, PhiInstr phi, int depth, ArrayList<FSubInstr> path) {
        if (depth > 4) {
            return null;
        }
        if (val.getOperand1() == phi || val.getOperand2() == phi) {
            if (depth == 4) return path;
            return null;
        }
        if (val.getOperand1() instanceof FSubInstr subInstr) {
            path.add(subInstr);
            return digFindFSubInstr(subInstr, phi, depth + 1, path);
        } else if (val.getOperand2() instanceof FSubInstr subInstr) {
            path.add(subInstr);
            return digFindFSubInstr(subInstr, phi, depth + 1, path);
        }
        return null;
    }

    private ArrayList<FSubInstr> isVecFMinusRecPhi(PhiInstr phi) {
        if (isCollecting) return null;
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        if (loop == null) return null;
        if (loop.getLoopHeader() != phi.getParentBB()) return null;

        Value pre = phi.getOperandMap().get(loop.getPreHeader());
        Value recur = phi.getOperandMap().get(loop.getLatchBlocks().iterator().next());
        if (pre == null || recur == null) return null;

        if (!(recur instanceof FSubInstr sub)) {
            return null;
        }
        ArrayList<FSubInstr> path = new ArrayList<>();
        path.add(sub);
        return digFindFSubInstr(sub, phi, 1, path);
    }

    private SCEVFVecMinusRecExpr buildVecFMinusRecFromPhi(PhiInstr phi, ArrayList<FSubInstr> fsubInstrs) {
        isCollecting = true;
        LoopInfo loop = phi.getParentBB().getLoopBelonged();
        Value init = phi.getOperandMap().get(loop.getPreHeader());

        List<SCEVExpr> operands = new ArrayList<>();
        Value last = phi;
        operands.add(getSCEV(init));
        for (int i = fsubInstrs.size() - 1; i >= 0; i--) {
            if (fsubInstrs.get(i).getOperand1() == last) {
                operands.add(getSCEV(fsubInstrs.get(i).getOperand2()));
                last = fsubInstrs.get(i);
            } else {
                operands.add(getSCEV(fsubInstrs.get(i).getOperand1()));
                last = fsubInstrs.get(i);
            }
        }

        isCollecting = false;
        return new SCEVFVecMinusRecExpr(operands, fsubInstrs.reversed(), loop, phi);
    }

    public void getTripCount(LoopCondition loopCond, LoopInfo loopInfo) {
        if (!loopCond.status) {
            return;
        }
        SCEVExpr step;
        SCEVExpr init;
        if (!loopCond.isVecRec) {
            if (loopCond.loopVarExpr.operands.size() != 2) {
                loopCond.tripCountExpr = CouldNotCompute;
                return;
            }
            step = loopCond.loopVarExpr.operands.get(1).simplify();
            init = loopCond.loopVarExpr.operands.get(0).simplify();
        } else {
            init = loopCond.loopVarExprSpecial.getOperands().getFirst().simplify();
            SCEVExpr step01 = new SCEVAddExpr(new ArrayList<>(){{
                add(loopCond.loopVarExprSpecial.getOperands().get(1).simplify());
                add(loopCond.loopVarExprSpecial.getOperands().get(2).simplify());
            }}, null).simplify();
            SCEVExpr step12 = new SCEVAddExpr(new ArrayList<>(){{
                add(step01);
                add(loopCond.loopVarExprSpecial.getOperands().get(3).simplify());
            }}, null).simplify();
            step = new SCEVAddExpr(new ArrayList<>(){{
                add(step12);
                add(loopCond.loopVarExprSpecial.getOperands().get(4).simplify());
            }}, null).simplify();
        }
        if (loopCond.cond == CmpInstr.CmpCond.ILT) {
            SCEVExpr count_base = new SCEVMinusExpr(new ArrayList<>(List.of(loopCond.boundExpr, init)), null).simplify();
            SCEVExpr step_below = new SCEVMinusExpr(new ArrayList<>(List.of(step, new SCEVConstant(1))), null).simplify();
            SCEVExpr count_up = new SCEVAddExpr(new ArrayList<>(List.of(count_base, step_below)), null).simplify();
            loopCond.tripCountExpr = new SCEVSdivExpr(new ArrayList<>(List.of(
                    count_up,
                    step
            )), null).simplify();
        } else if (loopCond.cond == CmpInstr.CmpCond.ILE) {
            SCEVExpr count_base = new SCEVMinusExpr(new ArrayList<>(List.of(loopCond.boundExpr, init)), null).simplify();
            SCEVExpr count_up = new SCEVAddExpr(new ArrayList<>(List.of(count_base, step)), null).simplify();
            loopCond.tripCountExpr = new SCEVSdivExpr(new ArrayList<>(List.of(
                    count_up,
                    step
            )), null).simplify();
        } else if (loopCond.cond == CmpInstr.CmpCond.IGT) {
            SCEVExpr count_base = new SCEVMinusExpr(new ArrayList<>(List.of(init, loopCond.boundExpr)), null);
            SCEVExpr step_rev = new SCEVMinusExpr(new ArrayList<>(List.of(new SCEVConstant(0), step)), null).simplify();
            SCEVExpr step_below = new SCEVMinusExpr(new ArrayList<>(List.of(step_rev, new SCEVConstant(1))), null).simplify();
            SCEVExpr count_up = new SCEVAddExpr(new ArrayList<>(List.of(count_base, step_below)), null).simplify();
            loopCond.tripCountExpr = new SCEVSdivExpr(new ArrayList<>(List.of(
                    count_up,
                    step_rev
            )), null).simplify();
        } else if (loopCond.cond == CmpInstr.CmpCond.IGE) {
            SCEVExpr count_base = new SCEVMinusExpr(new ArrayList<>(List.of(init, loopCond.boundExpr)), null).simplify();
            SCEVExpr step_rev = new SCEVMinusExpr(new ArrayList<>(List.of(new SCEVConstant(0), step)), null).simplify();
            SCEVExpr count_up = new SCEVAddExpr(new ArrayList<>(List.of(count_base, step_rev)), null).simplify();
            loopCond.tripCountExpr = new SCEVSdivExpr(new ArrayList<>(List.of(
                    count_up,
                    step_rev
            )), null).simplify();
        } else {
            loopCond.tripCountExpr = CouldNotCompute;
        }
    }

    public HashMap<Value, SCEVExpr> getSCEVInfo(){
        return cache;
    }
}
