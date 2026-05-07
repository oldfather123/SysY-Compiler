package midend.Analysis.SCEV;

import frontend.ir.Value;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.terminator.BranchInstr;
import midend.Analysis.SCEV.Expr.SCEVAddRecExpr;
import midend.Analysis.SCEV.Expr.SCEVExpr;
import midend.Analysis.SCEV.Expr.SCEVVecAddRecExpr;

public class LoopCondition {
    public BranchInstr branch;             // 使用这个比较的跳转指令
    public CmpInstr icmp;                  // 比较指令
    public Value loopVar;
    public SCEVAddRecExpr loopVarExpr;
    public SCEVVecAddRecExpr loopVarExprSpecial;
    public SCEVExpr tripCountExpr;
    public Value bound;
    public SCEVExpr boundExpr;
    public CmpInstr.CmpCond cond;
    public boolean status;
    public boolean isVecRec;

    public LoopCondition(BranchInstr branch, CmpInstr icmp) {
        this.branch = branch;
        this.icmp = icmp;

        this.cond = icmp.getCond();
        this.status = true;
        this.isVecRec = false;
        this.loopVarExprSpecial = null;
        SCEVManager manager = branch.getParentBB().getParentFunc().getSCEVManager();
        if (manager.getSCEV(icmp.getOperand1()) instanceof SCEVAddRecExpr addRec) {
            this.loopVar = icmp.getOperand1();
            this.loopVarExpr = addRec;
            this.bound = icmp.getOperand2();
            this.boundExpr = manager.getSCEV(icmp.getOperand2());
        } else if (manager.getSCEV(icmp.getOperand2()) instanceof SCEVAddRecExpr addRec) {
            this.loopVar = icmp.getOperand2();
            this.loopVarExpr = addRec;
            this.bound = icmp.getOperand1();
            this.boundExpr = manager.getSCEV(icmp.getOperand1());
        } else if (manager.getSCEV(icmp.getOperand1()) instanceof SCEVVecAddRecExpr addRec) {
            this.loopVar = icmp.getOperand1();
            this.loopVarExprSpecial = addRec;
            this.bound = icmp.getOperand2();
            this.boundExpr = manager.getSCEV(icmp.getOperand2());
            isVecRec = true;
        } else if (manager.getSCEV(icmp.getOperand2()) instanceof SCEVVecAddRecExpr addRec) {
            this.loopVar = icmp.getOperand2();
            this.loopVarExprSpecial = addRec;
            this.bound = icmp.getOperand1();
            this.boundExpr = manager.getSCEV(icmp.getOperand1());
            isVecRec = true;
        } else {
            this.loopVar = null;
            this.loopVarExpr = null;
            this.status = false;
        }
    }

    public SCEVExpr getTripCountExpr() {
//        if (tripCountExpr != null) {
//            throw new RuntimeException("TripCount需要再算一遍");
//        }
        return tripCountExpr;
    }

    @Override
    public String toString() {
        return "LoopCondition{" +
                "branch=" + branch + '\n' +
                ", icmp=" + icmp + '\n' +
                ", loopVar=" + loopVar + '\n' +
                ", loopVarExprSpecial=" + loopVarExprSpecial + '\n' +
                ", loopVarExpr=" + loopVarExpr + '\n' +
                ", tripCountExpr=" + tripCountExpr + '\n' +
                ", bound=" + bound + '\n' +
                ", boundExpr=" + boundExpr + '\n' +
                ", cond=" + cond + '\n' +
                ", status=" + status + '\n' +
                '}';
    }


}