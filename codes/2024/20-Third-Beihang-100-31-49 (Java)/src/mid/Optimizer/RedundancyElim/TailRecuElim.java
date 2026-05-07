package mid.Optimizer.RedundancyElim;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.MainFunction;
import mid.IntermediatePresentation.Function.Param;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.ControllFlow.ControlFlowGraph;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class TailRecuElim {
    private final HashMap<Function, HashSet<Call>> tailRecuFuncs = new HashMap<>();

    public void optimize() {
        IRManager.getModule().getDecledFunctions().forEach(this::tailRecuAnalyze);
        tailRecuElim();
    }

    private void tailRecuAnalyze(Function f) {
        /*
           该调用之后，（经过DCE），仅存在br和ret语句；若此函数有返回值，则该返回值一定是本call语句
         */

        if (f instanceof MainFunction) {
            return;
        }
        boolean isVoid = f.isVoid();
        boolean isTailRescu = false;
        HashSet<Call> rescus = new HashSet<>();
        for (BasicBlock b : f.getBlocks()) {
            ArrayList<Instruction> instructions = b.getInstructionList();
            for (int i = 0; i < instructions.size(); i++) {
                Instruction instr = instructions.get(i);
                if (instr instanceof Call call && call.getCallingFunction().equals(f)) {
                    ControlFlowGraph cfg = Optimizer.instance().getCFG();
                    i++;

                    while (i < instructions.size()) {
                        Instruction inst = instructions.get(i);
                        if (!(inst instanceof Ret ret)) {
                            break;
                        } else if (!isVoid && !(ret.getRetValue().equals(call))) {
                            break;
                        }
                        isTailRescu = true;
                        rescus.add(call);
                        i++;
                    }

                    if (cfg.getChildren(b) == null || !isTailRescu) {
                        break;
                    }

                    LinkedList<BasicBlock> queue = new LinkedList<>(cfg.getChildren(b));
                    while (!queue.isEmpty()) {
                        BasicBlock block = queue.poll();
                        if (block.getInstructionList().size() == 1) {
                            Instruction inst = block.getInstructionList().get(0);
                            if (inst instanceof Br) {
                                queue.addAll(cfg.getChildren(block));
                            } else if (inst instanceof Ret ret) {
                                if (!isVoid && !ret.getRetValue().equals(call)) {
                                    break;
                                }
                                isTailRescu = true;
                                rescus.add(call);
                            }
                        } else {
                            break;
                        }
                    }
                }
            }
        }

        if (isTailRescu) {
            tailRecuFuncs.put(f, rescus);
        }
    }

    private void tailRecuElim() {
        for (Function f : tailRecuFuncs.keySet()) {
            HashSet<Call> rescus = tailRecuFuncs.get(f);

            BasicBlock newHead = new BasicBlock();
            BasicBlock prevHead = f.getEntranceBlock();
            newHead.addInstruction(new Br(prevHead));
            f.addEntranceBlock(newHead);

            for (int i = 0; i < f.getParam().getParams().size(); i++) {
                Value p = f.getParam().getParams().get(i);
                if (p.isPointer()) {
                    continue;
                }

                Phi phi = new Phi(!p.isFloat(), IRManager.getInstance().declareTempVar());
                for (User user : p.getUserList()) {
                    if (!(user instanceof Param)) {
                        user.replaceOperand(p, phi);
                    }
                }

                phi.addCond(p, newHead);
                for (Call call : rescus) {
                    phi.addCond(call.getOperandList().get(i + 1), call.getBlock());
                }
                prevHead.addInstructionAt(0, phi);
            }

            for (Call call : rescus) {
                Br br = new Br(prevHead);
                call.getBlock().addInstructionAt(
                        call.getBlock().getInstructionList().indexOf(call), br);
                call.getBlock().removeInstruction(call);
            }
        }
    }

    public void recuInfer() {
        for (Function f : IRManager.getModule().getDecledFunctions()) {
            if (Optimizer.instance().hasSideEffect(f) || f.instructionCount() > 20) {
                // 不要太复杂。。。
                continue;
            }
            ArrayList<Ret> rets = new ArrayList<>();
            ArrayList<Call> recus = new ArrayList<>();
            for (BasicBlock b : f.getBlocks()) {
                for (Instruction i : b.getInstructionList()) {
                    if (i instanceof Ret ret) {
                        rets.add(ret);
                    }
                    if (i instanceof Call call && call.getCallingFunction().equals(f)) {
                        recus.add(call);
                    }
                }
            }
            if (rets.size() == 1 && rets.get(0).getRetValue() instanceof ConstNumber n) {
                for (User user : f.getUserList()) {
                    if (user instanceof Call) {
                        user.beReplacedBy(n);
                        user.destroy();
                    }
                }
                f.destroy();
            } else if (rets.size() == 2) {
                Ret ret1 = rets.get(0);
                Ret ret2 = rets.get(1);
                BasicBlock exit1 = ret1.getBlock();
                BasicBlock exit2 = ret2.getBlock();
                Ret finalRet = null, firstRet = null;


                for (Call call : recus) {
                    BasicBlock recuBlock = call.getBlock();
                    if (Optimizer.instance().getDominAnalyzer().canDomin(recuBlock, exit1)) {
                        finalRet = ret1;
                        firstRet = ret2;
                        break;
                    } else if (Optimizer.instance().getDominAnalyzer().canDomin(recuBlock, exit2)) {
                        finalRet = ret2;
                        firstRet = ret1;
                        break;
                    }
                }

                if (finalRet == null || f.getParam().getParams().size() != 2) {
                    continue;
                }
                // 解析另一个返回值，假如符合 ret = a - call, a = b + call
                if (!(firstRet.getRetValue() instanceof ConstNumber n && n.getVal().intValue() == 0 && finalRet.getRetValue() instanceof ALU alu)) {
                    continue;
                }
                if (alu.getOperator().equals("-") && alu.getOperand1() instanceof ALU alu1 && alu.getOperand2() instanceof Call call &&
                        recus.contains(call) && alu1.getOperator().equals("+") && alu1.getOperand2() instanceof Call call1 &&
                        recus.contains(call1) && alu1.getOperand1().equals(f.getParam().getParams().get(0)) &&
                        call1.getOperandList().get(1).equals(f.getParam().getParams().get(0)) && call1.getOperandList().get(2) instanceof ALU alu2
                        && alu2.getOperator().equals("-") && alu2.getOperand1().equals(f.getParam().getParams().get(1)) &&
                        alu2.getOperand2() instanceof ConstNumber n1 && n1.getVal().intValue() == 1) {
                    for (BasicBlock b : new ArrayList<>(f.getBlocks())) {
                        f.removeBlock(b);
                        b.destroy();
                    }
                    BasicBlock entry = new BasicBlock();
                    f.addEntranceBlock(entry);
                    Value param1 = f.getParam().getParams().get(0);
                    Value param2 = f.getParam().getParams().get(1);
                    ALU mod2 = new ALU(param2, "%", new ConstNumber(2), true);
                    entry.addInstruction(mod2);
                    Cmp eq0 = new Cmp("==", mod2, new ConstNumber(0));
                    entry.addInstruction(eq0);
                    BasicBlock firstOut = new BasicBlock();
                    f.addBlock(firstOut);
                    BasicBlock finalOut = new BasicBlock();
                    f.addBlock(finalOut);
                    Br choseRet = new Br(eq0, finalOut, firstOut);
                    entry.addInstruction(choseRet);
                    firstOut.addInstruction(new Ret(new ConstNumber(0).withType(!f.isFloat()), !f.isFloat()));
                    finalOut.addInstruction(new Ret(param1, !param1.isFloat()));
                }
            }
        }
    }
}
