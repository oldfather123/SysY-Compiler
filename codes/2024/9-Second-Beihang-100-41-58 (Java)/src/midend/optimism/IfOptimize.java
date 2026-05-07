package midend.optimism;

import frontend.AST.Func;
import midend.*;
import midend.LLVMType.IntegerType;
import midend.Module;
import midend.analysis.LoopInfo;
import midend.instr.BrInstr;
import midend.instr.CmpInstr;
import midend.instr.Instruction;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class IfOptimize {
    private Module module;
    private HashSet<BasicBlock> visit = new HashSet<>();
    private HashMap<Value, Integer> left = new HashMap<>();
    private HashMap<Value, Integer> right = new HashMap<>();
    private HashMap<Value, Integer> leftEq = new HashMap<>();
    private HashMap<Value, Integer> rightEq = new HashMap<>();
    private ArrayList<Function> functions;

    public IfOptimize(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            visit.clear();
            for (BasicBlock block : function.getBlockList()) {
                if (!visit.contains(block)) {
                    left.clear();
                    right.clear();
                    leftEq.clear();
                    rightEq.clear();
                    DFS(block);
                }
            }
        }
    }

    public void DFS(BasicBlock block) {
        if (visit.contains(block)) {
            return;
        }
        visit.add(block);
        HashMap<Value, Integer> tempLeft = new HashMap<>();
        HashMap<Value, Integer> tempRight = new HashMap<>();
        HashMap<Value, Integer> tempLeftEq = new HashMap<>();
        HashMap<Value, Integer> tempRightEq = new HashMap<>();
        tempLeft.putAll(left);
        tempRight.putAll(right);
        tempLeftEq.putAll(leftEq);
        tempRightEq.putAll(rightEq);
        if (block.getInstrList().getLast() instanceof BrInstr) {
            if (((BrInstr) block.getInstrList().getLast()).getIsIf()
                    && ((BrInstr) block.getInstrList().getLast()).getValue() instanceof CmpInstr) {
                BrInstr brInstr = (BrInstr) block.getInstrList().getLast();
                CmpInstr cmpInstr = (CmpInstr) brInstr.getValue();
                BasicBlock ifTrue = brInstr.getIfTrueBlock();
                BasicBlock ifFalse = brInstr.getIfFalseBlock();
                if (cmpInstr.getRight() instanceof ConstInt) {
                    Value left0 = cmpInstr.getLeft();
                    int num = ((ConstInt) cmpInstr.getRight()).getValue();
                    switch (cmpInstr.getOpStr()) {
                        case "<" -> {
                            if (right.containsKey(left0) && right.get(left0) <= num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 1));
                            } else if (rightEq.containsKey(left0) && rightEq.get(left0) < num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 1));
                            } else if (left.containsKey(left0) && left.get(left0) >= num - 1) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 0));
                            } else if (leftEq.containsKey(left0) && leftEq.get(left0) >= num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 0));
                            }
                            if (!right.containsKey(left0)) {
                                right.put(left0, num);
                            }
                            if (num < right.get(left0)) {
                                right.put(left0, num);
                            }
                        }
                        case ">" -> {
                            if (right.containsKey(left0) && right.get(left0) <= num + 1) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 0));
                            } else if (rightEq.containsKey(left0) && rightEq.get(left0) <= num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 0));
                            } else if (left.containsKey(left0) && left.get(left0) >= num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 1));
                            } else if (leftEq.containsKey(left0) && leftEq.get(left0) >= num + 1) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 1));
                            }
                            if (!left.containsKey(left0)) {
                                left.put(left0, num);
                            }
                            if (num > left.get(left0)) {
                                left.put(left0, num);
                            }
                        }
                        case "<=" -> {
                            if (right.containsKey(left0) && right.get(left0) <= num + 1) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 1));
                            } else if (rightEq.containsKey(left0) && rightEq.get(left0) <= num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 1));
                            } else if (left.containsKey(left0) && left.get(left0) >= num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 0));
                            } else if (leftEq.containsKey(left0) && leftEq.get(left0) >= num + 1) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 0));
                            }
                            if (!rightEq.containsKey(left0)) {
                                rightEq.put(left0, num);
                            }
                            if (num < rightEq.get(left0)) {
                                rightEq.put(left0, num);
                            }
                        }
                        case ">=" -> {
                            if (right.containsKey(left0) && right.get(left0) <= num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 0));
                            } else if (rightEq.containsKey(left0) && rightEq.get(left0) <= num - 1) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 0));
                            } else if (left.containsKey(left0) && left.get(left0) >= num - 1) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 1));
                            } else if (leftEq.containsKey(left0) && leftEq.get(left0) >= num) {
                                brInstr.setValue(0, new ConstInt(IntegerType.i32, 1));
                            }
                            if (!leftEq.containsKey(left0)) {
                                leftEq.put(left0, num);
                            }
                            if (num > leftEq.get(left0)) {
                                leftEq.put(left0, num);
                            }
                        }
                    }
                }
                if (ifTrue.getPreBlock().size() == 1 && !visit.contains(ifTrue)) {
                    DFS(ifTrue);
                }
                if (ifFalse.getPreBlock().size() == 1 && !visit.contains(ifFalse)) {
                    left.clear();
                    right.clear();
                    leftEq.clear();
                    rightEq.clear();
                    left.putAll(tempLeft);
                    right.putAll(tempRight);
                    leftEq.putAll(tempLeftEq);
                    rightEq.putAll(tempRightEq);
                    if (cmpInstr.getRight() instanceof ConstInt) {
                        Value left0 = cmpInstr.getLeft();
                        int num = ((ConstInt) cmpInstr.getRight()).getValue();
                        switch (cmpInstr.getOpStr()) {
                            case "<" -> {
                                if (!leftEq.containsKey(left0)) {
                                    leftEq.put(left0, num);
                                }
                                if (num > leftEq.get(left0)) {
                                    leftEq.put(left0, num);
                                }
                            }
                            case "<=" -> {
                                if (!left.containsKey(left0)) {
                                    left.put(left0, num);
                                }
                                if (num > left.get(left0)) {
                                    left.put(left0, num);
                                }
                            }
                            case ">" -> {
                                if (!rightEq.containsKey(left0)) {
                                    rightEq.put(left0, num);
                                }
                                if (num > rightEq.get(left0)) {
                                    rightEq.put(left0, num);
                                }
                            }
                            case ">=" -> {
                                if (!right.containsKey(left0)) {
                                    right.put(left0, num);
                                }
                                if (num > right.get(left0)) {
                                    right.put(left0, num);
                                }
                            }
                        }
                    }
                    DFS(ifFalse);
                }
            } else if (!((BrInstr) block.getInstrList().getLast()).getIsIf()){
                DFS(block.getNextBlock().get(0));
            }
        }

        left.clear();
        right.clear();
        leftEq.clear();
        rightEq.clear();

        left.putAll(tempLeft);
        right.putAll(tempRight);
        leftEq.putAll(tempLeftEq);
        rightEq.putAll(tempRightEq);
    }
}
