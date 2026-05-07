package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Shift;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;
import mid.SymbolTable.SymbolTableManager;

import java.util.ArrayList;
import java.util.HashSet;

public class MemcpyOpt {
    /*
        考虑 while (ind<n) { a[i]=constant; ...} ,要求ind为{init,1},必须为minLoop
        则可以转为为memset(a+initInd, constant, repeatNumber * 4)
        在llvm形式下，表现为出现一对ptr = gep a, ind和store constant, ptr 指令

        更进一步，考虑多重循环嵌套下的赋值语句，可以递归地被表示为memset
        即内部循环先转为memset(gep,val,size)，之后考虑外部的minLoop循环
        其中，size是constant（暂定）；gep a,ind，ind = {init,step}，要求step>=size/4
        简单起见，也可以要求要么 step == size/4，要么 size == 4
        这样，可以将其转为 memset(a+init,val,step * repeatNumber * 4)
     */

    public void optimize() {
        HashSet<Loop> loops = new HashSet<>(Optimizer.instance().getLoopAnalyze().getLoops());
        boolean hasChanged = true;
        while (hasChanged) {
            hasChanged = false;
            for (Loop loop : loops) {
                if (loop instanceof MinLoop minLoop) {
                    hasChanged |= optimizeFor(minLoop);
                }
            }
        }
    }

    private boolean optimizeFor(MinLoop minLoop) {
        boolean hasChanged = false;
        for (BasicBlock block : minLoop.getBlocksInLoop()) {
            for (Instruction instr : new ArrayList<>(block.getInstructionList())) {
                if (instr instanceof Store store) {
                    Value constant = store.getSrc();
                    Value ptr = store.getAddr();

                    if (!(ptr instanceof GetElementPtr gep && minLoop.isConstant(constant))) {
                        continue;
                    }

                    Value array = gep.getPtr();
                    Value ind = gep.getElemIndex();
                    if (!(array.isPointer() && minLoop.getInds().containsKey(ind))) {
                        continue;
                    }

                    ScEvValue scev = minLoop.getInds().get(ind);
                    Value step = scev.getStepVal();
                    if (!(step instanceof ConstNumber n && n.getVal().floatValue() == 1.0)) {
                        continue;
                    }

                    Value init = scev.getInitVal();
                    GetElementPtr initPtr = new GetElementPtr(array, init);
//                    Shift size = new Shift(false, minLoop.getRepeatNumber(), new ConstNumber(2));
                    BasicBlock exit = minLoop.getExit();

                    Function memset = (Function) SymbolTableManager.getInstance().getIRValue("@memset");
                    ArrayList<Value> params = new ArrayList<>();
                    params.add(initPtr);
                    params.add(constant);
                    params.add(minLoop.getRepeatNumber());
                    exit.addInstrAtEntry(new Call(memset, params));
                    exit.addInstrAtEntry(initPtr);

                    store.getBlock().removeInstruction(store);
                    store.destroy();
                    hasChanged = true;
                } else if (instr instanceof Call call &&
                        call.getCallingFunction() instanceof LibFunction.Memset) {
                    Value ptr = call.getOperandList().get(0);
                    Value val = call.getOperandList().get(1);
                    Value size = call.getOperandList().get(2);

                    if (!(ptr instanceof GetElementPtr gep) ||
                            !minLoop.isConstant(size) || !minLoop.isConstant(val)) {
                        continue;
                    }

                    Value array = gep.getPtr();
                    Value ind = gep.getElemIndex();
                    ScEvValue scev = minLoop.getInds().get(ind);
                    Value init = scev.getInitVal();
                    Value step = scev.getStepVal();

                    if (size instanceof ConstNumber n) {
                        // 如果是n，需要大于等于
                        if (!(step instanceof ConstNumber nstp &&
                                nstp.getVal().floatValue() >= n.getVal().floatValue() / 4)) {
                            continue;
                        }
                    } else if (!(size instanceof Shift shift &&
                            shift.getOperandList().get(0).equals(step))) {
                        continue;
                    }


                    GetElementPtr initPtr = new GetElementPtr(array, init);
                    ALU stepSum = new ALU(step, "*", minLoop.getRepeatNumber(), !step.isFloat());
//                    Shift cmbSize = new Shift(false, stepSum,
//                            new ConstNumber(2));
                    BasicBlock exit = minLoop.getExit();
                    Function memset = (Function) SymbolTableManager.getInstance().getIRValue("@memset");
                    ArrayList<Value> params = new ArrayList<>();
                    params.add(initPtr);
                    params.add(val);
                    params.add(stepSum);
                    exit.addInstrAtEntry(new Call(memset, params));
                    exit.addInstrAtEntry(initPtr);
//                    exit.addInstrAtEntry(cmbSize);
                    exit.addInstrAtEntry(stepSum);

                    call.getBlock().removeInstruction(call);
                    call.destroy();
                    hasChanged = true;
                }
            }
        }
        return hasChanged;
    }
}
