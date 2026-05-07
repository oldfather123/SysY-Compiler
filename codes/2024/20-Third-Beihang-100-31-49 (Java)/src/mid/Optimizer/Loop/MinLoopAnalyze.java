package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.ControllFlow.ControlFlowGraph;
import mid.Optimizer.Optimizer;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class MinLoopAnalyze {
    private HashMap<Value, ScEvValue> basicInds;

    public void analyze() {
        HashSet<Loop> loops = new HashSet<>(Optimizer.instance().getLoopAnalyze().getLoops());
        loops.forEach(this::analyzeFor);
    }

    public void analyzeFor(Loop loop) {
        /*
            Min Loop满足：
                exiting单一，保证不存在break/continue
                能够求出循环次数
         */
        if (loop.getExitings().size() > 1) {
            return;
        }
        BasicBlock exiting = (BasicBlock) loop.getExitings().toArray()[0];
        BasicBlock latch = loop.getLatch();

        ControlFlowGraph cfg = Optimizer.instance().getCFG();
        BasicBlock header = loop.getHeader();
        HashSet<BasicBlock> exit = loop.getExits();
        if (exit.size() > 1) {
            return;
        }
        for (BasicBlock b : loop.getBlocksInLoop()) {
            if (!b.equals(latch)) {
                HashSet<BasicBlock> children = cfg.getChildren(b);
                if (children != null && children.contains(header)) {
                    return;
                }
            }
        }

        // 尝试求出其循环次数，这里要求两个比较值要么是ind要么是constValue(循环外定义量或常值)
        MinLoop minLoop = new MinLoop(loop);
        Value cond = ((Br) exiting.getLastInstruction()).getCond();
        if (!(cond instanceof Cmp cmpCond)) {
            return;
        }
        Value operand1 = cmpCond.getOperandList().get(0);
        Value operand2 = cmpCond.getOperandList().get(1);
        String op = cmpCond.getOperator();
        if (operand1.isFloat()) {
            // 关于浮点数的考虑，如果有浮点数参与，可能有点过于复杂了
            return;
        }

        basicInds = loop.getInds();

        if (!(loop.isConstant(operand1) || basicInds.containsKey(operand1))) {
            return;
        } else if (!(loop.isConstant(operand2) || basicInds.containsKey(operand2))) {
            return;
        }

        if (basicInds.containsKey(operand1) && basicInds.containsKey(operand2)) {
            // 退出条件：两个均为归纳变量
            // TODO:再说吧，想不明白了
            return;
        }

        if (loop.isConstant(operand1) && loop.isConstant(operand2)) {
            // 退出条件：两个均为常量 这个只有死循环和不进入两种可能，没啥好优化的
            return;
        }

        Value biv = null;
        Value constant = null;
        if (loop.isConstant(operand1) && basicInds.containsKey(operand2)) {
            biv = operand2;
            constant = operand1;
            op = Cmp.getOppsiteOperator(op);
        } else if (loop.isConstant(operand2) && basicInds.containsKey(operand1)) {
            biv = operand1;
            constant = operand2;
        }

        assert biv != null && constant != null;
        // 退出条件：归纳变量和常量 （最主要的情况）
        /*
            退出条件，总是形如biv op constant，biv = {init，+，step}
            由于是jud-do-while形式，因此初始值一定满足条件，只需要考虑循环的次数；并且，已经确定一定是整型变量
            假设a//b表示上取整除法，则a//b=(a-1+b)/b，注意这里不能改成(a-1)/b+1，因为a可能为负
            > ，step一定是负数，值是 ((init-constant-1+absstep)/absstep)+1，
                即上取整除法，之后加一（do-while循环下已经执行了一次）
            < ，step一定是正数，值是 ((constant-init-1+step)/step)+1
            >= 相当于>constant-1，值是 ((init-constant+absstep)/absstep)+1
            <= 相当于<constant+1，值是 ((constant-init+step)/step)+1
            == 整型条件下似乎不会出现这种情况吧，也没有优化的意义
            != 可以转化为>或<，但是可能需要引入额外的跳转判断指令，意义存疑，但可以仅对biv与constant均为常值的情况进行估计
         */
        ScEvValue scEvValue = basicInds.get(biv);
//        if (!(biv instanceof Phi)) {
//            // 为统一次数规范，都需要是phi
//            scEvValue = basicInds.get(loop.getIndWithPhi().get(biv));
//        }
        Value init = scEvValue.getInitVal();
        Value step = scEvValue.getStepVal();
        BasicBlock preHeader = loop.getPreheader();

        Instruction repeatNumber = null;
        switch (op) {
            case ">=" -> {
                Instruction diffVal = new ALU(init, "-", constant, true);
                preHeader.addInstructionBeforeBranch(diffVal);
                Instruction absStep = new ALU((new ConstNumber(0)).withType(true),
                        "-", step, true);
                preHeader.addInstructionBeforeBranch(absStep);
                Instruction toCeilDiv = new ALU(diffVal, "+", absStep, true);
                preHeader.addInstructionBeforeBranch(toCeilDiv);
                repeatNumber = new ALU(toCeilDiv, "/", absStep, true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
                repeatNumber = new ALU(repeatNumber, "+",
                        (new ConstNumber(1)).withType(true), true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
            }
            case "<=" -> {
                Instruction diffVal = new ALU(constant, "-", init, true);
                preHeader.addInstructionBeforeBranch(diffVal);
                Instruction toCeilDiv = new ALU(diffVal, "+", step, true);
                preHeader.addInstructionBeforeBranch(toCeilDiv);
                repeatNumber = new ALU(toCeilDiv, "/", step, true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
                repeatNumber = new ALU(repeatNumber, "+",
                        (new ConstNumber(1)).withType(true), true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
            }
            case ">" -> {
                Instruction diffVal = new ALU(init, "-", constant, true);
                preHeader.addInstructionBeforeBranch(diffVal);
                Instruction absStep = new ALU((new ConstNumber(0)).withType(true),
                        "-", step, true);
                preHeader.addInstructionBeforeBranch(absStep);
                Instruction toCeilDiv = new ALU(diffVal, "+", absStep, true);
                preHeader.addInstructionBeforeBranch(toCeilDiv);
                toCeilDiv = new ALU(toCeilDiv, "-", new ConstNumber(1), true);
                preHeader.addInstructionBeforeBranch(toCeilDiv);
                repeatNumber = new ALU(toCeilDiv, "/", absStep, true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
                repeatNumber = new ALU(repeatNumber, "+",
                        (new ConstNumber(1)).withType(true), true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
            }
            case "<" -> {
                Instruction diffVal = new ALU(constant, "-", init, true);
                preHeader.addInstructionBeforeBranch(diffVal);
                Instruction toCeilDiv = new ALU(diffVal, "+", step, true);
                preHeader.addInstructionBeforeBranch(toCeilDiv);
                toCeilDiv = new ALU(toCeilDiv, "-", new ConstNumber(1), true);
                preHeader.addInstructionBeforeBranch(toCeilDiv);
                repeatNumber = new ALU(toCeilDiv, "/", step, true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
                repeatNumber = new ALU(repeatNumber, "+",
                        (new ConstNumber(1)).withType(true), true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
            }
            case "==" -> {
                // 假如能够进入，由于const是循环无关的，因此只可能进行一次
                repeatNumber = new ALU(new ConstNumber(0), "+", new ConstNumber(1), true);
                preHeader.addInstructionBeforeBranch(repeatNumber);
            }
            default -> {
                return;
            }
        }
        minLoop.setRepeatNumber(repeatNumber);
        Optimizer.instance().getLoopAnalyze().loopMinimize(loop, minLoop);
    }
}
