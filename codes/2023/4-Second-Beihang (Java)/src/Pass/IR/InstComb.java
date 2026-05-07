package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.IntegerType;
import IR.Use;
import IR.Value.*;
import IR.Value.Instructions.BinaryInst;
import IR.Value.Instructions.Instruction;
import IR.Value.Instructions.OP;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;
import java.util.HashMap;

public class InstComb implements Pass.IRPass {
    private final IRBuildFactory f = IRBuildFactory.getInstance();

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            constInstCombForFunc(function);
        }
        for(Function function : module.getFunctions()){
            instCombForFunc(function);
        }
    }

    private void instCombForFunc(Function function){
        ArrayList<BinaryInst> twoTimesInst = new ArrayList<>();

        for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
            BasicBlock bb = bbNode.getValue();
            IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();
            while (itInstNode != null) {
                Instruction inst = itInstNode.getValue();
                itInstNode = itInstNode.getNext();
                //  TODO: 目前只考虑整数
                if(inst instanceof BinaryInst binInst
                        && binInst.getLeftVal().equals(binInst.getRightVal())
                        && binInst.getOp().equals(OP.Add)
                        && binInst.getType().isIntegerTy()
                        && !(binInst.getLeftVal() instanceof Const)){
                    twoTimesInst.add(binInst);
                }
            }
        }

        //  取出形如 %b = add %a %a的指令
        for(BinaryInst binInst : twoTimesInst){
            int nowFactor  = 2;
            Value baseValue = binInst.getLeftVal();
            //  change标识是否可以接着沿着def-use合并
            boolean change = true;
            Value nowBinInst = binInst;
            while (change){
                change = false;
                for(User user : nowBinInst.getUserList()){
                    Instruction userInst = (Instruction) user;
                    if(!(userInst instanceof BinaryInst userBinInst)){
                        continue;
                    }
                    if(!userBinInst.getLeftVal().equals(baseValue)
                            && !userBinInst.getRightVal().equals(baseValue)){
                        continue;
                    }

                    //  开始修改
                    nowFactor++;
                    setCombValueFactor(userBinInst, nowFactor, baseValue);
                    change = true;
                    //  迭代
                    nowBinInst = userBinInst;
                }
            }
        }
    }

    private void setCombValueFactor(BinaryInst binInst, int factor, Value baseValue){
        Value leftValue = binInst.getLeftVal();
        Value rightValue = binInst.getRightVal();
        if(leftValue.equals(baseValue)){
            binInst.replaceOperand(1, f.buildNumber(factor));
            binInst.setOp(OP.Mul);
        }
        else if(rightValue.equals(baseValue)){
            binInst.replaceOperand(0, f.buildNumber(factor));
            binInst.setOp(OP.Mul);
        }
    }

    private void constInstCombForFunc(Function function){
        for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
            BasicBlock bb = bbNode.getValue();
            IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();
            while (itInstNode != null){
                Instruction inst = itInstNode.getValue();
                itInstNode = itInstNode.getNext();

                if(canCombAdd(inst)){
                    ArrayList<Use> tmpUseList = new ArrayList<>(inst.getUseList());
                    for(Use use : tmpUseList) {
                        BinaryInst userInst = (BinaryInst) use.getUser();
                        combAddToUser((BinaryInst) inst, userInst);
                    }
                    inst.removeSelf();
                }

                else if(canCombMul(inst)){
                    ArrayList<Use> tmpUseList = new ArrayList<>(inst.getUseList());
                    for(Use use : tmpUseList) {
                        BinaryInst userInst = (BinaryInst) use.getUser();
                        combMulToUser((BinaryInst) inst, userInst);
                    }
                    inst.removeSelf();
                }

                //  除法需要向下取整，不确定可不可以直接comb
//                else if(canCombDiv(inst)){
//                    for(Use use : inst.getUseList()) {
//                        BinaryInst userInst = (BinaryInst) use.getUser();
//                        combDivToUser((BinaryInst) inst, userInst);
//                    }
//                    inst.removeSelf();
//                }
            }
        }
    }

    private void combMulToUser(BinaryInst inst, BinaryInst userInst){
        Value leftA = inst.getLeftVal();
        Value rightA = inst.getRightVal();
        Value leftB = userInst.getLeftVal();
        Value rightB = userInst.getRightVal();

        int ConstInA = 0, ConstInB = 0;
        float ConstFloatA = 0, ConstFloatB = 0;
        boolean ConstInAIs0 = false, ConstInBIs0 = false;
        boolean isInt = inst.getType().isIntegerTy();

        if (leftA instanceof Const) {
            if(isInt) ConstInA = ((ConstInteger) leftA).getValue();
            else ConstFloatA = ((ConstFloat) leftA).getValue();
            ConstInAIs0 = true;
        }
        else {
            if(isInt) ConstInA = ((ConstInteger) rightA).getValue();
            else ConstFloatA = ((ConstFloat) rightA).getValue();
        }

        if (leftB instanceof Const) {
            if(isInt) ConstInB = ((ConstInteger) leftB).getValue();
            else ConstFloatB = ((ConstFloat) leftB).getValue();
            ConstInBIs0 = true;
        }
        else {
            if(isInt) ConstInB = ((ConstInteger) rightB).getValue();
            else ConstFloatB = ((ConstFloat) rightB).getValue();
        }

        ConstInteger constInteger = new ConstInteger(ConstInA * ConstInB, IntegerType.I32);
        ConstFloat constFloat = new ConstFloat(ConstFloatA * ConstFloatB);
        //  b = 3 * a; c = 2 * b;
        if(ConstInAIs0 && ConstInBIs0){
            if(isInt) userInst.replaceOperand(0, constInteger);
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, rightA);
        }
        else if(ConstInAIs0 && !ConstInBIs0){
            if(isInt) userInst.replaceOperand(1, constInteger);
            else userInst.replaceOperand(1, constFloat);
            userInst.replaceOperand(0, rightA);
        }
        else if(!ConstInAIs0 && ConstInBIs0){
            if(isInt) userInst.replaceOperand(0, constInteger);
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
        }
        else if(!ConstInAIs0 && !ConstInBIs0){
            if(isInt) userInst.replaceOperand(1, constInteger);
            else userInst.replaceOperand(1, constFloat);
            userInst.replaceOperand(0, leftA);
        }
    }


    //  b = a + 1; c = b + 2;
    //  c = a + 3;
    private void combAddToUser(BinaryInst inst, BinaryInst userInst){
        Value leftA = inst.getLeftVal();
        Value rightA = inst.getRightVal();
        Value leftB = userInst.getLeftVal();
        Value rightB = userInst.getRightVal();

        int ConstInA = 0, ConstInB = 0;
        float ConstFloatA = 0, ConstFloatB = 0;
        boolean ConstInAIs0 = false, ConstInBIs0 = false;
        boolean isInt = inst.getType().isIntegerTy();
        boolean isAAdd = (inst.getOp() == OP.Add) || (inst.getOp() == OP.Fadd);
        boolean isBAdd = (userInst.getOp() == OP.Add) || (inst.getOp() == OP.Fadd);

        if (leftA instanceof Const) {
            if(isInt) ConstInA = ((ConstInteger) leftA).getValue();
            else ConstFloatA = ((ConstFloat) leftA).getValue();
            ConstInAIs0 = true;
        }
        else {
            if(isInt) ConstInA = ((ConstInteger) rightA).getValue();
            else ConstFloatA = ((ConstFloat) rightA).getValue();
        }

        if (leftB instanceof Const) {
            if(isInt) ConstInB = ((ConstInteger) leftB).getValue();
            else ConstFloatB = ((ConstFloat) leftB).getValue();
            ConstInBIs0 = true;
        }
        else {
            if(isInt) ConstInB = ((ConstInteger) rightB).getValue();
            else ConstFloatB = ((ConstFloat) rightB).getValue();
        }

        if (ConstInAIs0 && ConstInBIs0 && isAAdd && isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInA + ConstInB, IntegerType.I32);
            ConstFloat constfloat = new ConstFloat(ConstFloatA + ConstFloatB);
            if(isInt) userInst.replaceOperand(0, constantInt);
            else userInst.replaceOperand(0, constfloat);
            userInst.replaceOperand(1, rightA);
        }
        else if (ConstInAIs0 && ConstInBIs0 && isAAdd && !isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInB - ConstInA, IntegerType.I32);
            ConstFloat constfloat = new ConstFloat(ConstFloatB - ConstFloatA);

            if(isInt) userInst.replaceOperand(0, constantInt);
            else userInst.replaceOperand(0, constfloat);
            userInst.replaceOperand(1, rightA);
        }
        else if (ConstInAIs0 && ConstInBIs0 && !isAAdd && isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInB + ConstInA, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatB + ConstFloatA);
            if(isInt) userInst.replaceOperand(0, constantInt );
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, rightA);
            if(isInt) userInst.setOp(OP.Sub);
            else userInst.setOp(OP.Fsub);
        }
        else if (ConstInAIs0 && ConstInBIs0 && !isAAdd && !isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInB - ConstInA, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatB - ConstFloatA);
            if(isInt) userInst.replaceOperand(0, constantInt);
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, rightA);
            if(isInt) userInst.setOp(OP.Add);
            else userInst.setOp(OP.Fadd);
        }


        else if (ConstInAIs0 && !ConstInBIs0 && isAAdd && isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInA + ConstInB, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatA + ConstFloatB);
            if(isInt) userInst.replaceOperand(1, constantInt);
            else userInst.replaceOperand(1, constFloat);
            userInst.replaceOperand(0, rightA);
        }
        else if (ConstInAIs0 && !ConstInBIs0 && isAAdd && !isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInA - ConstInB, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatA - ConstFloatB);
            if(isInt) userInst.replaceOperand(1, constantInt);
            else userInst.replaceOperand(1, constFloat);
            userInst.replaceOperand(0, rightA);
            if(isInt) userInst.setOp(OP.Add);
            else userInst.setOp(OP.Fadd);
        }
        //b = x - a; c = b + y; c = (x+y) - a
        else if (ConstInAIs0 && !ConstInBIs0 && !isAAdd && isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInA + ConstInB, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatA + ConstFloatB);
            if(isInt) userInst.replaceOperand(0, constantInt );
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, rightA);
            if(isInt) userInst.setOp(OP.Sub);
            else userInst.setOp(OP.Fsub);
            //A.remove();
        }
        //b = x - a; c = b - y ==> c = (x - y) - a
        else if (ConstInAIs0 && !ConstInBIs0 && !isAAdd && !isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInA - ConstInB, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatA - ConstFloatB);
            if(isInt) userInst.replaceOperand(0, constantInt );
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, rightA);
        }
        else if (!ConstInAIs0 && ConstInBIs0 && isAAdd && isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInA + ConstInB, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatA + ConstFloatB);
            if(isInt) userInst.replaceOperand(0, constantInt);
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
        }
        else if (!ConstInAIs0 && ConstInBIs0 && isAAdd && !isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInB - ConstInA, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatB - ConstFloatA);
            if(isInt) userInst.replaceOperand(0, constantInt);
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
        }
        else if (!ConstInAIs0 && ConstInBIs0 && !isAAdd && isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInB - ConstInA, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatB - ConstFloatA);
            if(isInt) userInst.replaceOperand(0, constantInt );
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
        }
        else if (!ConstInAIs0 && ConstInBIs0 && !isAAdd && !isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInB + ConstInA, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatB + ConstFloatA);
            if(isInt) userInst.replaceOperand(0, constantInt);
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
        }

        else if (!ConstInAIs0 && !ConstInBIs0 && isAAdd && isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInA + ConstInB, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatA + ConstFloatB);
            if(isInt) userInst.replaceOperand(0, constantInt );
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
        }
        else if (!ConstInAIs0 && !ConstInBIs0 && isAAdd && !isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInA - ConstInB, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatA - ConstFloatB);
            if(isInt) userInst.replaceOperand(0, constantInt );
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
            if(isInt) userInst.setOp(OP.Add);
            else userInst.setOp(OP.Fadd);
        }
        else if (!ConstInAIs0 && !ConstInBIs0 && !isAAdd && isBAdd) {
            ConstInteger constantInt = new ConstInteger(ConstInB - ConstInA, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(ConstFloatB - ConstFloatA);
            if(isInt) userInst.replaceOperand(0, constantInt);
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
        }

        //b = a - x; c = b - y; c = a - (x + y)
        else if (!ConstInAIs0 && !ConstInBIs0 && !isAAdd && !isBAdd) {
            ConstInteger constantInt = new ConstInteger(-ConstInB - ConstInA, IntegerType.I32);
            ConstFloat constFloat = new ConstFloat(-ConstFloatB - ConstFloatA);
            if(isInt) userInst.replaceOperand(0, constantInt );
            else userInst.replaceOperand(0, constFloat);
            userInst.replaceOperand(1, leftA);
            if(isInt) userInst.setOp(OP.Add);
            else userInst.setOp(OP.Fadd);
        }
    }

    private boolean canCombAdd(Instruction inst){
        if(!isCombAddAlu(inst)){
            return false;
        }
        for(Use use : inst.getUseList()){
            User user = use.getUser();
            if(!(user instanceof Instruction userInst && isCombAddAlu(userInst))){
                return false;
            }
        }
        return true;
    }

    private boolean canCombMul(Instruction inst){
        if(!isCombMulAlu(inst)){
            return false;
        }
        for(Use use : inst.getUseList()){
            User user = use.getUser();
            if(!(user instanceof Instruction userInst && isCombMulAlu(userInst))){
                return false;
            }
        }
        return true;
    }

    private boolean canCombDiv(Instruction inst){
        if(!isCombDivAlu(inst)){
            return false;
        }
        for(Use use : inst.getUseList()){
            User user = use.getUser();
            if(!(user instanceof Instruction userInst && isCombDivAlu(userInst))){
                return false;
            }
        }
        return true;
    }

    private boolean isCombAddAlu(Instruction inst){
        if(!(inst instanceof BinaryInst binaryInst)){
            return false;
        }
        OP op = binaryInst.getOp();
        if(!(op == OP.Add || op == OP.Sub || op == OP.Fadd || op == OP.Fsub)){
            return false;
        }
        return binaryInst.hasOneConst();
    }

    private boolean isCombMulAlu(Instruction inst){
        if(!(inst instanceof BinaryInst binaryInst)){
            return false;
        }
        OP op = binaryInst.getOp();
        if(!(op == OP.Mul || op == OP.Fmul)){
            return false;
        }
        return binaryInst.hasOneConst();
    }

    private boolean isCombDivAlu(Instruction inst){
        if(!(inst instanceof BinaryInst binaryInst)){
            return false;
        }
        OP op = binaryInst.getOp();
        if(!(op == OP.Div || op == OP.Fdiv)){
            return false;
        }
        return binaryInst.hasOneConst();
    }

    @Override
    public String getName() {
        return "InstComb";
    }
}
