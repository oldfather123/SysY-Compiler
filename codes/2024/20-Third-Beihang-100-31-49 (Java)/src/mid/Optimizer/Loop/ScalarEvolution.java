package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class ScalarEvolution {
    private final HashMap<Value, ScEvValue> basicInds = new HashMap<>();

    public void optimize() {
        // 从外层循环开始遍历，避免出现内层循环的归纳变量scev指令在外层循环内部导致use早于def位置
        Optimizer.instance().getLoopAnalyze().bfsLoops().forEach(this::analyze);
    }

    public void analyze(Loop loop) {
        basicIndsAnalyze(loop);
        givToBiv(loop);
        // 归纳变量的scev是不能跨循环使用的
        basicInds.clear();
    }

    private void basicIndsAnalyze(Loop loop) {
        /*
            识别基础归纳变量BIV，形如 X = phi [initVal, preheader] [val, latch]
            其中，val = X + stepval，除X外均为循环不变量
            其SCEV表达式为 { initVal, op, stepVal }
         */

        BasicBlock header = loop.getHeader();
        BasicBlock preheader = loop.getPreheader();
        BasicBlock latch = loop.getLatch();
        for (Instruction i : header.getInstructionList()) {
            if (!(i instanceof Phi phi)) {
                break;
            }
            if (loop.isConstant(phi.valueFromBlock(preheader))) {
                Value val = phi.valueFromBlock(latch);
                Value stepVal = null;
                String op = null;
                if (val instanceof ALU alu) {
                    if ((alu.getOperator().equals("+") || alu.getOperator().equals("-"))
                            && alu.getOperandList().contains(i)) {
                        for (Value v : alu.getOperandList()) {
                        /*
                            如果是减法，则只可能是 evolve = phi - stepval {phiInit - stepval, -stepval}
                            如果是 evolve = stepval - phi ，应该排除
                         */
                            if (!v.equals(i) && loop.isConstant(v)) {
                                stepVal = v;
                                op = alu.getOperator();
                                if (op.equals("-") && alu.getOperandList().get(0).equals(stepVal)) {
                                    stepVal = null;
                                }
                            }
                        }
                    }
                }

                if (stepVal != null && !stepVal.isFloat()) {
                    Value phiInit = phi.valueFromBlock(preheader);
                    Value evolve = phi.valueFromBlock(latch);
                    if (op.equals("-")) {
                        stepVal = new ALU((new ConstNumber(0)).withType(!stepVal.isFloat()), "-",
                                stepVal, !stepVal.isFloat());
                        preheader.addInstructionBeforeBranch((Instruction) stepVal);
                    }
                    ScEvValue phiScEv = new ScEvValue(phiInit, stepVal);
                    // 这里已经将所有的biv转为了加法形式
                    Instruction evolveInit = new ALU(phiInit, "+", stepVal, !stepVal.isFloat());
                    loop.getPreheader().addInstructionBeforeBranch(evolveInit);
                    ScEvValue evolveScEv = new ScEvValue(evolveInit, stepVal);
                    basicInds.put(phi, phiScEv);
                    basicInds.put(evolve, evolveScEv);
                    loop.addInd(phi, evolve, phiScEv, evolveScEv);
                }
            }
        }
    }

    public void givToBiv(Loop loop) {
        /*
            识别依赖归纳变量GIV，形如 giv = biv op const；更进一步，giv2 = giv1 op const也算作GIV
            将其转为 giv = phiInit + evovle
            giv1 = {a, op, b}, op是+,-,*，另外两个有点危险 TODO:/和%可以作为GIV吗
            其SCEV表达式为： phi {a op const - evovle, op, evovle} giv {a op const, op, evovle}
            其中，+ -: evovle = b ; * : evovle = b * const
            此外，由于-不符合交换律，所以需要额外识别：giv = const - biv应该是 phi {const - a, -b} giv {const -a + b, -b}
         */
        boolean hasChanged = true;
        HashSet<BasicBlock> blocks = loop.getBlocksInLoop();

        /*
            所有giv存储到givList，其中givList的内容：
            0. giv
            1. givOperand
            2. constant
         */
        HashSet<ArrayList<Value>> givLists = new HashSet<>();
        while (hasChanged) {
            hasChanged = false;
            for (BasicBlock block : blocks) {
                for (Instruction instr : block.getInstructionList()) {
                    if (basicInds.containsKey(instr)) {
                        continue;
                    }
                    if (!(instr instanceof ALU alu && (
//                            alu.getOperator().equals("+") || alu.getOperator().equals("-") ||
                            alu.getOperator().equals("*")
                    ))) {
                        continue;
                    }
                    Value constant;
                    Value givOperand;
                    if (loop.isConstant(alu.getOperandList().get(0)) &&
                            basicInds.containsKey(alu.getOperandList().get(1))) {
                        constant = alu.getOperandList().get(0);
                        givOperand = alu.getOperandList().get(1);
                    } else if (loop.isConstant(alu.getOperandList().get(1)) &&
                            basicInds.containsKey(alu.getOperandList().get(0))) {
                        constant = alu.getOperandList().get(1);
                        givOperand = alu.getOperandList().get(0);
                    } else {
                        continue;
                    }
                    hasChanged = true;

                    ArrayList<Value> givList = new ArrayList<>();
                    givList.add(alu);
                    givList.add(givOperand);
                    givList.add(constant);
                    givLists.add(givList);
                }
            }

            // 将识别的giv都转为biv并求出其scev表达式
            BasicBlock preheader = loop.getPreheader();
            BasicBlock latch = loop.getLatch();
            BasicBlock header = loop.getHeader();
            for (ArrayList<Value> givList : givLists) {
                ALU giv = (ALU) givList.get(0);
                boolean isMult = giv.getOperator().equals("*");
                ScEvValue basicInd = basicInds.get(givList.get(1));
                Value init = basicInd.getInitVal();
                Value evovle = basicInd.getStepVal();
                Value constant = givList.get(2);
                boolean isInt = !init.isFloat();
                if (!isInt) {
                    continue;
                }

                Instruction evovleGiv, initGiv;
                if (isMult) {
                    evovleGiv = new ALU(evovle, "*", constant, true);
                } else {
                    evovleGiv = new ALU(evovle, "+", new ConstNumber(0), true);
                }
                if (constant.equals(giv.getOperand1())) {
                    initGiv = new ALU(constant, giv.getOperator(), init, true);
                    if (giv.getOperator().equals("-")) {
                        preheader.addInstructionBeforeBranch(evovleGiv);
                        evovleGiv = new ALU((new ConstNumber(0)).withType(true), "-", evovleGiv, true);
                    }
                } else {
                    initGiv = new ALU(init, giv.getOperator(), constant, true);
                }

                Instruction initPhi = new ALU(initGiv, "-", evovleGiv, true);

                Phi phiInHeader = new Phi(true, IRManager.getInstance().declareTempVar());
                Instruction givInsideLoop = new ALU(phiInHeader, "+", evovleGiv, true);
                phiInHeader.addCond(initPhi, preheader);
                phiInHeader.addCond(givInsideLoop, latch);

                preheader.addInstructionBeforeBranch(evovleGiv);
                preheader.addInstructionBeforeBranch(initGiv);
                preheader.addInstructionBeforeBranch(initPhi);
                giv.beReplacedBy(givInsideLoop);
                header.addInstrAtEntry(givInsideLoop);
                giv.getBlock().removeInstruction(giv);
                giv.destroy();
                header.addInstrAtEntry(phiInHeader);

                ScEvValue phiScev = new ScEvValue(initPhi, evovleGiv);
                ScEvValue givScev = new ScEvValue(initGiv, evovleGiv);
                basicInds.put(givInsideLoop, givScev);
                basicInds.put(phiInHeader, phiScev);
                loop.addInd(phiInHeader, givInsideLoop, phiScev, givScev);
            }
            givLists.clear();
        }
    }

    public void remLift() {
        for (Loop loop : Optimizer.instance().getLoopAnalyze().getLoops()) {
            if (loop instanceof MinLoop minLoop) {
                Br br = (Br) minLoop.getExiting().getLastInstruction();
                Cmp cmp = (Cmp) br.getCond();
                Value n;
                if (loop.isConstant(cmp.getOperandList().get(0))) {
                    n = cmp.getOperandList().get(0);
                } else {
                    n = cmp.getOperandList().get(1);
                }
                for (Instruction i : minLoop.getHeader().getInstructionList()) {
                    if (!(i instanceof Phi phi)) {
                        continue;
                    }
                    if (i.getUserList().size() == 1 && phi.valueFromBlock(minLoop.getLatch()) instanceof ALU alu) {
                        if (alu.getOperator().equals("%") && alu.getOperand2() instanceof ConstNumber n1 &&
                                alu.getOperand1() instanceof ALU alu1
                                && (alu1.getOperator().equals("+")) &&
                                (alu1.getOperand1().equals(i) && alu1.getOperand2() instanceof ConstNumber n2 && Integer.MAX_VALUE / n2.getVal().intValue() > n1.getVal().intValue()
                                        || alu1.getOperand2().equals(i) && alu1.getOperand1() instanceof ConstNumber n3 && Integer.MAX_VALUE / n3.getVal().intValue() > n1.getVal().intValue()
                                )
                                && alu1.getUserList().size() == 1 &&
                                alu.getUserList().stream().allMatch(user -> (user.equals(i) || !loop.getBlocksInLoop().contains(user.getBlock())))) {
                            // rem外提
//                            i.replaceOperand(alu, alu1);
//                            ALU remN = new ALU(minLoop.getRepeatNumber(), "%", alu.getOperand2(), true);
//                            loop.getPreheader().addInstructionBeforeBranch(remN);
//                            cmp.replaceOperand(n, remN);
                            i.getBlock().removeInstruction(i);
                            alu.getBlock().removeInstruction(alu);
                            alu1.getBlock().removeInstruction(alu1);
                            ALU remedCnt = new ALU(minLoop.getRepeatNumber(), "%", n1, true);
                            ALU remAdd;
                            if (alu1.getOperand1().equals(i)) {
                                remAdd = new ALU(remedCnt, "*", alu1.getOperand2(), true);
                            } else {
                                remAdd = new ALU(remedCnt, "*", alu1.getOperand1(), true);
                            }
                            ALU newSum = new ALU(remAdd, "+", phi.valueFromBlock(minLoop.getPreheader()), true);
                            alu.beReplacedBy(newSum);
                            alu.destroy();
                            i.destroy();
                            alu1.destroy();
                            minLoop.getPreheader().addInstructionBeforeBranch(remedCnt);
                            minLoop.getPreheader().addInstructionBeforeBranch(remAdd);
                            minLoop.getPreheader().addInstructionBeforeBranch(newSum);
                            break;
                        }
                    }
                }
            }
        }

    }
}
