package midend.loop;

import frontend.ir.Value;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.*;
import frontend.ir.instr.convop.Si2Fp;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.otherop.cmp.Cmp;
import frontend.ir.instr.otherop.cmp.CmpCond;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.ArrayList;
import java.util.HashMap;


public class LoopSimplify {
    public static void execute(ArrayList<Function> functions) {
        AnalysisLoop.execute(functions);
        for (Function function : functions) {
            for (Loop out : function.getOuterLoop()) {
                dfs4loopSimplify(out);
            }
        }
    }

    private static void dfs4loopSimplify(Loop loop) {
        for (Loop inner : loop.getInnerLoops()) {
            dfs4loopSimplify(inner);
        }
        AnalysisLoop.singleLiftAnalysis(loop);
        if (!loop.hasIndVar()) {
            return;
        }
//        loop.LoopPrint();
        BasicBlock loopExit = loop.getExits().get(0);
        BasicBlock exit = null;
        for (BasicBlock blk : loopExit.getSucs()) {
            exit = blk;
        }
        if (exit == null) {
            return;
        }
        Instruction phi = (Instruction) exit.getInstructions().getHead();
        HashMap<Instruction, PhiInstr> liveOut = new HashMap<>();
        while (phi instanceof PhiInstr) {
            int index = ((PhiInstr) phi).getPrtBlks().indexOf(loopExit);
            if (index == -1) {
                throw new RuntimeException(phi.print() + " prtBlk: " + loopExit);
            }
            Value value = ((PhiInstr) phi).getValues().get(index);
            if (canLift(value)) {
                liveOut.put((Instruction) value, (PhiInstr) phi);
            }
            phi = (Instruction) phi.getNext();
        }
        if (liveOut.isEmpty()) {
            return;
        }
        //计算times
        /* (init - end - 1) / step + 1 */
        Cmp cmp = loop.getCond();
        if (!(cmp.getCond().equals(CmpCond.LT)) && !(cmp.getCond().equals(CmpCond.GT))) {
            return;
        }
        Value init = loop.getBegin();
        Value end = loop.getEnd();
        Value step = loop.getStep();
        Value alu = loop.getAlu();
        Procedure procedure = (Procedure) loopExit.getParent().getOwner();
        SubInstr sub1;
        if (alu instanceof AddInstr) {
            sub1 = new SubInstr(procedure.getAndAddRegIndex(), end, init);
        } else if (alu instanceof SubInstr) {
            sub1 = new SubInstr(procedure.getAndAddRegIndex(), init, end);
        } else {
            return;
        }
        SubInstr sub2 = new SubInstr(procedure.getAndAddRegIndex(), sub1, ConstInt.One);
        SDivInstr div = new SDivInstr(procedure.getAndAddRegIndex(), sub2, step);
        AddInstr times  = new AddInstr(procedure.getAndAddRegIndex(), div, ConstInt.One);
        loopExit.addInstrToHead(times);
        loopExit.addInstrToHead(div);
        loopExit.addInstrToHead(sub2);
        loopExit.addInstrToHead(sub1);
        Instruction last = times;
        HashMap<Instruction, Instruction> old2new = new HashMap<>();
        BasicBlock preHeader = loop.getPreHeader();
        for (Instruction instr : liveOut.keySet()) {
            if (!(instr instanceof BinaryOperation)) {
                throw new RuntimeException("bush gemer?");
            }
            Value op1 = ((BinaryOperation) instr).getOp1();
            Value op2 = ((BinaryOperation) instr).getOp2();
            /*
            * TODO:
            *  reg1 = phi1 + 1
            *  reg2 = reg1 + 1
            *
            * */
            if (instr instanceof SRemInstr) {
                if (!(((SRemInstr) instr).getOp1() instanceof BinaryOperation)) {
                    continue;
                }
                BinaryOperation remOp1 = (BinaryOperation) ((SRemInstr) instr).getOp1();
                Value remOp1Op1 = remOp1.getOp1();
                Value remOp1Op2 = remOp1.getOp2();
                if (!(remOp1Op2 instanceof ConstInt)) {
                    continue;
                }
                if (!(remOp1Op1 instanceof PhiInstr)) {
                    continue;
                }
                int index = ((PhiInstr) remOp1Op1).getPrtBlks().indexOf(preHeader);
                if (index == -1) {
                    return;
                }
                Value phiStartValue = ((PhiInstr) remOp1Op1).getValues().get(index);


                SRemInstr timeRem = new SRemInstr(procedure.getAndAddRegIndex(), times, op2);//TODO: 前提？？
                timeRem.insertAfter(last);
                last = timeRem;
                SRemInstr addRem = new SRemInstr(procedure.getAndAddRegIndex(), remOp1Op2, op2);
                addRem.insertAfter(last);
                last = addRem;
                MulInstr mul = new MulInstr(procedure.getAndAddRegIndex(), timeRem, addRem);
                mul.insertAfter(last);
                last = mul;
                SRemInstr mulRem = new SRemInstr(procedure.getAndAddRegIndex(), mul, op2);
                mulRem.insertAfter(last);
                last = mulRem;
                if (op1 instanceof AddInstr) {
                    AddInstr resAdd = new AddInstr(procedure.getAndAddRegIndex(), phiStartValue, mulRem);
                    resAdd.insertAfter(last);
                    last = resAdd;
                    old2new.put(instr, resAdd);
                } else if (op1 instanceof SubInstr) {
                    SubInstr resSub = new SubInstr(procedure.getAndAddRegIndex(), phiStartValue, mulRem);
                    resSub.insertAfter(last);
                    last = resSub;
                    old2new.put(instr, resSub);
                } else {
                    continue;
                }
            } else {
                /*
                * instr : reg = phi + const
                *   => phiStartValue + const * times
                * mul:  const * times
                * */
                if (!(op1 instanceof PhiInstr)) {
                    continue;
                }
                int index = ((PhiInstr) op1).getPrtBlks().indexOf(preHeader);
                if (index == -1) {
                    return;
                }
                Value phiStartValue = ((PhiInstr) op1).getValues().get(index);

                if (instr instanceof AddInstr) {
                    MulInstr mul = new MulInstr(procedure.getAndAddRegIndex(), times, op2);
                    mul.insertAfter(last);
                    last = mul;
                    AddInstr resAdd = new AddInstr(procedure.getAndAddRegIndex(), phiStartValue, mul);
                    resAdd.insertAfter(last);
                    last = resAdd;
                    old2new.put(instr, resAdd);
                } else if (instr instanceof SubInstr) {
                    MulInstr mul = new MulInstr(procedure.getAndAddRegIndex(), times, op2);
                    mul.insertAfter(last);
                    last = mul;
                    SubInstr resSub = new SubInstr(procedure.getAndAddRegIndex(), phiStartValue, mul);
                    resSub.insertAfter(last);
                    last = resSub;
                    old2new.put(instr, resSub);
                } else if (instr instanceof FAddInstr) {
                    Si2Fp si2Fp = new Si2Fp(procedure.getAndAddRegIndex(), times);
                    si2Fp.insertAfter(last);
                    last = si2Fp;
                    FAddInstr resAdd = new FAddInstr(procedure.getAndAddRegIndex(), phiStartValue, si2Fp);
                    resAdd.insertAfter(last);
                    last = resAdd;
                } else {
                    continue;
                }
            }

        }

        for (Instruction instr : old2new.keySet()) {
            PhiInstr phiInstr = liveOut.get(instr);
            phiInstr.modifyUse(instr, old2new.get(instr));
//            instr.removeFromList();
        }
    }
    private static boolean canLift(Value value) {
        if (!(value instanceof Instruction)) {
            return false;
        }
        if (value instanceof AddInstr && ((AddInstr) value).getOp2() instanceof ConstInt) {
            return true;
        }
        if (value instanceof SubInstr && ((SubInstr) value).getOp2() instanceof ConstInt) {
            return true;
        }
        if (value instanceof SRemInstr && ((SRemInstr) value).getOp2() instanceof ConstInt) {//TODO: 可以进一步优化
            return true;
        }
//        if (value instanceof FAddInstr && ((FAddInstr) value).getOp2() instanceof ConstFloat) {
//            return true;
//        }
        return false;
    }
}
