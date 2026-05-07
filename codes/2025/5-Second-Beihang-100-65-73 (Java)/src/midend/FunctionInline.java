package midend;

import frontend.ir.cloner.FunctionCloner;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import util.CustomList;

import java.util.*;

public class FunctionInline {
    private static final int instrSize = 1024;

    public static void execute(List<Function> functions) {
        Function main = null;
        for (Function function : functions) {
            if (Objects.equals(function.getName(), "main")) {
                main = function;
            }
        }
        if (main != null) {
            HashSet<Function> path = new HashSet<>();
            dfsInline(main, null, path);
        } else {
            throw new RuntimeException("没有找到 main 函数");
        }

        removeUnusedFunc(functions);
    }

    private static void dfsInline(Function function, Function parent, HashSet<Function> path) {
        path.add(function);
        for (Function callee : new HashSet<>(function.getCalleeMap().keySet())) {
            if (path.contains(callee)) {
                continue;
            }
            dfsInline(callee, function, path);
        }
        if (isInlineable(function)) {
            inlineFunc(function, parent);
        }
        path.remove(function);
    }


    private static boolean isInlineable(Function function) {
        if (Objects.equals(function.getName(), "main") || function.isRecursive() || function instanceof Function.LibFunc) {
            return false;
        }
        int sumInstr = function.getInstrNum();
        return sumInstr <= instrSize;
    }

    public static void inlineFunc(Function from, Function to) {
        for (BasicBlock basicBlock : to.getBasicBlockList()) {
            for (Instruction instruction : basicBlock.getInstrList()) {
                if (instruction instanceof CallInstr callInstr) {
                    if (callInstr.getCallee() == from) {
                        inlineCall(from, callInstr);
                    }
                }
            }
        }
        to.rearrangeAlloca();
    }

    public static void inlineCall(Function function, CallInstr callInstr) {
        BasicBlock preBB = callInstr.getParentBB();
        BasicBlock aftBB = new BasicBlock(callInstr.getParentBB().getParentFunc().getAndAddBlkIdx());
        preBB.replaceOnlyPhi(aftBB);
        CustomList.Node ins = callInstr.getNext();
        while (ins != null) {
            Instruction nxt = (Instruction) ins.getNext();
            if (ins instanceof Terminator t) {
                preBB.getUsedList().remove(t);
                aftBB.getUsedList().add(t);
                t.getUserList().remove(preBB);
                t.getUserList().add(aftBB);
            }
            if (ins instanceof Instruction instr) {
                instr.liteRemoveFromList();
                aftBB.addInstruction(instr);
            }
            ins = nxt;
        }
        aftBB.close();

        aftBB.getSucs().addAll(preBB.getSucs());
        for (BasicBlock suc : preBB.getSucs()) {
            suc.getPres().remove(preBB);
            suc.getPres().add(aftBB);
        }
        preBB.getSucs().clear();
        aftBB.insertAfter(preBB);
        FunctionCloner.getInstance().cloneFunction(function, callInstr.getParentBB().getParentFunc(), preBB, callInstr.getRParams());
        FunctionCloner.getInstance().removeRet(aftBB, callInstr);
    }

    public static void removeUnusedFunc(List<Function> functions) {
        for (Function function : new ArrayList<>(functions)) {
            if (function.getCallCnt() == 0 && !function.getName().equals("main")) {
                function.removeFromList();
                functions.remove(function);
            }
        }
    }
}
