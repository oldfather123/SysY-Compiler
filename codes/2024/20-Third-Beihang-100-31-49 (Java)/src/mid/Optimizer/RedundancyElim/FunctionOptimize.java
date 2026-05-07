package mid.Optimizer.RedundancyElim;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.Function.MainFunction;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.HyperParams;
import mid.Optimizer.Optimizer;
import mid.Optimizer.Utils.CloneManager;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class FunctionOptimize {
    private final Module module;
    //由于没有函数声明，因此实际上是调用树
    private final HashMap<Function, ArrayList<Function>> functionCallTree = new HashMap<>();
    //具有副作用的函数
    private final HashSet<Function> sideEffectFunction = new HashSet<>();

    private final HashSet<Function> recursiveFunctions = new HashSet<>();

    //每个函数被调用的次数
    private final HashMap<Function, Integer> callCounter = new HashMap<>();

    private CloneManager cloneManager;


    public FunctionOptimize() {
        module = IRManager.getModule();
    }

    public void optimize() {
        for (Function f : module.getDecledFunctions()) {
            ArrayList<Call> calls = new ArrayList<>();
            for (BasicBlock b : f.getBlocks()) {
                for (Instruction instr : b.getInstructionList()) {
                    if (instr instanceof Call call && !(call.getCallingFunction() instanceof LibFunction
                            || isRecursive(call.getCallingFunction()))) {
                        calls.add(call);
                    }
                }
            }

            for (Call call : calls) {
                if (f.instructionCount() > HyperParams.INSTR_MAX_NUM) {
                    break;
                }
                inline(call);
            }
            if (!(f instanceof MainFunction)) {
                Optimizer.instance().dominAnalyze(f);
            }
        }
        analyze();
    }

    public void optimizeForStack(int num) {
        for (Function f : module.getDecledFunctions()) {
            ArrayList<Call> calls = new ArrayList<>();
            for (BasicBlock b : f.getBlocks()) {
                for (Instruction instr : b.getInstructionList()) {
                    if (instr instanceof Call call &&
                            !(call.getCallingFunction() instanceof LibFunction || isRecursive(call.getCallingFunction()))) {
                        calls.add(call);
                    }
                }
            }

            for (Call call : calls) {
                if (f.instructionCount() + call.getCallingFunction().instructionCount() > num && call.getOperandList().size() <= 16) {
                    break;
                }
                inline(call);
            }
            if (!calls.isEmpty() && !(f instanceof MainFunction)) {
                Optimizer.instance().dominAnalyze(f);
            }
        }
    }

    public void analyze() {
        //包括调用分析和副作用分析
        functionCallTree.clear();
        sideEffectFunction.clear();
        recursiveFunctions.clear();
        callCounter.clear();

        for (Function f : module.getDecledFunctions()) {
            for (BasicBlock block : f.getBlocks()) {
                for (Instruction instruction : block.getInstructionList()) {
                    if (instruction instanceof Call call) {
                        Function calledFunction = call.getCallingFunction();

                        //只有仅调用了一次，且不在循环中的才可以加入
                        if (block.getLoopDepth() == 0 && !callCounter.containsKey(calledFunction)) {
                            callCounter.put(calledFunction, 1);
                        } else {
                            callCounter.put(calledFunction, 2);
                        }

                        if (!functionCallTree.containsKey(f)) {
                            functionCallTree.put(f, new ArrayList<>());
                        }
                        functionCallTree.get(f).add(calledFunction);

                        if (hasSideEffect(calledFunction)) {
                            sideEffectFunction.add(f);
                        }

                        if (f.equals(calledFunction)) {
                            recursiveFunctions.add(f);
                        }
                    } else if (instruction instanceof Store store && store.hasSideEffect()) {
                        sideEffectFunction.add(f);
                    }
                }
            }
        }

        checkDeadFunctions();
    }

    public void checkDeadFunctions() {
        HashSet<Function> functions = new HashSet<>(module.getFunctions());
        for (Function f : functions) {
            if (!(f instanceof LibFunction) && callCounter.getOrDefault(f, 0) == 0) {
                module.getFunctions().remove(f);
                f.destroy();
            }
        }
    }

    public boolean hasSideEffect(Function function) {
        return (sideEffectFunction.contains(function)) || function instanceof LibFunction || function instanceof MainFunction;
    }

    public boolean isRecursive(Function function) {
        return recursiveFunctions.contains(function);
    }


    public ArrayList<Function> closurseWhenCall(Function function) {
        ArrayList<Function> closure = new ArrayList<>();
        LinkedList<Function> queue = new LinkedList<>();
        queue.add(function);

        while (!queue.isEmpty()) {
            Function f = queue.poll();
            if (closure.contains(f)) {
                continue;
            }
            closure.add(f);
            if (functionCallTree.containsKey(f) && functionCallTree.get(f) != null) {
                queue.addAll(functionCallTree.get(f));
            }
        }
        return closure;
    }

    public void inline(Call call) {
        Function calledFunction = call.getCallingFunction();
        BasicBlock b = call.getBlock();
        Function f = b.getFunction();

        BasicBlock afterCallBlock = new BasicBlock();
        f.addBlock(afterCallBlock);
        ArrayList<Instruction> instrs = new ArrayList<>(b.getInstructionList());
        int idx = b.getInstructionList().indexOf(call);
        for (int i = idx + 1; i < instrs.size(); i++) {
            afterCallBlock.addInstruction(instrs.get(i));
        }
        b.getInstructionList().removeAll(afterCallBlock.getInstructionList());

        b.removeInstruction(call);
        ArrayList<BasicBlock> newBlocks = functionColne(call, afterCallBlock);
        BasicBlock entry = newBlocks.get(0);

        b.addInstructionAt(idx, new Br(entry));

        for (BasicBlock block : newBlocks) {
            f.addBlock(block);
        }
        cloneManager.cloneEnd();

        if (!calledFunction.isVoid()) {
            //phi
            call.beReplacedBy(afterCallBlock.getInstructionList().get(0));
        }
        call.destroy();
    }

    private ArrayList<BasicBlock> functionColne(Call call, BasicBlock afterBlock) {
        ArrayList<BasicBlock> blocks = new ArrayList<>();
        //填充指令
        //原函数中的value和当前内联函数中的value
        Function calledFunction = call.getCallingFunction();
        cloneManager = new CloneManager(call.getBlock().getFunction());
        ArrayList<Value> formValues = calledFunction.getParam().getParams();

        Phi phi = null;
        if (!calledFunction.isVoid()) {
            phi = new Phi(new Alloca(!calledFunction.isFloat()), IRManager.getInstance().declareLocalVar());
            afterBlock.addInstrAtEntry(phi);
        }

        //加入形参->实参的对应
        for (int i = 0; i < formValues.size(); i++) {
            cloneManager.put(formValues.get(i), call.getOperandList().get(i + 1));
        }
        //加入block的对应
        ArrayList<BasicBlock> bfsDTArray = Optimizer.instance().bfsDominTreeArray(calledFunction.getEntranceBlock());
        for (BasicBlock block : calledFunction.getBlocks()) {
            BasicBlock curBlock = new BasicBlock();
            blocks.add(curBlock);
            cloneManager.put(block, curBlock);
        }

        for (BasicBlock block : bfsDTArray) {
            //老问题：我如果向回跳入，而顺序遍历的话，会出现变量先使用后定义的情况，所以需要按支配树顺序遍历
            BasicBlock curBlock = (BasicBlock) cloneManager.get(block);

            for (Instruction instruction : block.getInstructionList()) {
                Instruction newInstruction;

                if (instruction instanceof Ret ret) {
                    if (calledFunction.isVoid()) {
                        newInstruction = new Br(afterBlock);
                    } else {
                        newInstruction = new Br(afterBlock);
                        if (phi != null) {
                            phi.addCond(cloneManager.get(ret.getRetValue()), curBlock);
                        }
                    }
                } else {
                    newInstruction = cloneManager.getNewInstruction(instruction);
                }

                cloneManager.put(instruction, newInstruction);
                curBlock.addInstruction(newInstruction);
            }
        }
        return blocks;
    }

    public boolean calledOnce(Function function) {
        return callCounter.getOrDefault(function, 0) == 1;
    }
}
