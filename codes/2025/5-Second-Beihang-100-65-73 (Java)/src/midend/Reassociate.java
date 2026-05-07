package midend;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.binop.BinaryOperation;
import frontend.ir.instr.binop.MulInstr;
import frontend.ir.instr.binop.SubInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import util.Pair;

import java.util.*;

public class Reassociate {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            // 每个函数一个映射表，从一个可以重结合（满足结合律且为整数运算）的指令映射到其展开后的操作数（<系数-元素>）列表
            Map<BinaryOperation, List<Pair<Integer, Value>>> unfoldMap = new HashMap<>();
            for (BasicBlock basicBlock : function.getBasicBlockList()) {
                for (Instruction ins : basicBlock.getInstrList()) {
                    if (ins instanceof BinaryOperation) {
                        execute4ins((BinaryOperation) ins, function, basicBlock, unfoldMap);
                    }
                }
            }
        }
    }

    private static void execute4ins(
            BinaryOperation ins, Function function, BasicBlock basicBlock,
            Map<BinaryOperation, List<Pair<Integer, Value>>> unfoldMap) {
        if (!(ins instanceof AddInstr || ins instanceof SubInstr || ins instanceof MulInstr)) {
            // 目前的中端指令集中只有这三条能够进行重结合
            return;
        }

        List<Pair<Integer, Value>> operands = new ArrayList<>();
        addOperands(ins.getOperand1(), operands, ins.getOperatorName(), unfoldMap, false);
        addOperands(ins.getOperand2(), operands, ins.getOperatorName(), unfoldMap, ins instanceof SubInstr);
        sortOperands(operands, false);
        mergeOperands(operands);
        sortOperands(operands, true);
        if (operands.getFirst().getKey() < 0) {
            return; // 全是负数就不要重结合了，还要多一个 0，虽然这种情况似乎不太可能
        }

        if (checkOnlySameOp(ins.getUserList(), ins.getOperatorName())) {
            // 有且只有一个同类指令使用，说明可以继续向上合并，不在这里处理
            unfoldMap.put(ins, operands);
            return;
        }

        if (!checkNeedRebuild(operands)) {
            // 可能有多个同类指令使用，作为一个整体向上暴露以免代码体积过度膨胀 todo 是否必要？
            // 如果经过 rebuild 应该在做完 rebuild 之后用新的指令进行映射
            unfoldMap.put(ins, List.of(new Pair<>(1, ins)));
            return;
        }

        // begin to rebuild
        List<Pair<Boolean, Value>> newOperandList = makeOperandList(ins, function, basicBlock, operands);
        Value res = rebuildChainOfIns(ins, function, basicBlock, newOperandList);

        // 用新指令建立映射
        if (res instanceof BinaryOperation binRes) {
            unfoldMap.put(binRes, List.of(new Pair<>(1, binRes)));
        }

        ins.replaceUseWith(res);
        ins.removeFromList();
    }

    private static Value rebuildChainOfIns(Instruction ins, Function function, BasicBlock basicBlock,
                                           List<Pair<Boolean, Value>> newOperandList) {
        Value res = null;
        for (Pair<Boolean, Value> newOperand : newOperandList) {
            if (res == null) {
                res = newOperand.getValue();    // 第一个项不可能是负系数项
            } else {
                Instruction newIns;
                if (ins instanceof AddInstr || ins instanceof SubInstr) {
                    if (newOperand.getKey()) {
                        newIns = new AddInstr(function.getAndAddRegIdx(), res, newOperand.getValue(), basicBlock);
                    } else {
                        newIns = new SubInstr(function.getAndAddRegIdx(), res, newOperand.getValue(), basicBlock);
                    }
                } else {
                    // ins instanceof MulInstr
                    newIns = new MulInstr(function.getAndAddRegIdx(), res, newOperand.getValue(), basicBlock);
                }
                newIns.setUse(res);
                newIns.setUse(newOperand.getValue());
                newIns.insertBefore(ins);
                res = newIns;
            }
        }
        return res;
    }

    /**
     * 合并 <系数-元素> 为可用的 Value 便于后续组件新链条
     * Pair 中的 boolean 值用于指示正负，为 true 表示正、为 false 表示负。
     *  只有加法列表可能为 false
     */
    private static List<Pair<Boolean, Value>> makeOperandList(
            Instruction ins, Function function, BasicBlock basicBlock, List<Pair<Integer, Value>> operands) {
        List<Pair<Boolean, Value>> newOperandList = new ArrayList<>();
        if (ins instanceof AddInstr || ins instanceof SubInstr) {
            for (Pair<Integer, Value> operand : operands) {
                if (operand.getKey() == 1) {
                    newOperandList.add(new Pair<>(true, operand.getValue()));
                } else if (operand.getKey() == -1) {
                    newOperandList.add(new Pair<>(false, operand.getValue()));
                } else {
                    boolean isPositive = operand.getKey() > 0;

                    Value op1 = operand.getValue();
                    Value op2 = new IntConst(
                            isPositive ? operand.getKey() : -operand.getKey()
                    );
                    MulInstr mul = new MulInstr(function.getAndAddRegIdx(), op1, op2, basicBlock);
                    mul.setUse(op1);
                    mul.setUse(op2);
                    mul.insertBefore(ins);
                    newOperandList.add(new Pair<>(isPositive, mul));
                }
            }
        } else {
            // ins instanceof MulInstr
            for (Pair<Integer, Value> operand : operands) {
                if (operand.getKey() <= 0) {
                    throw new RuntimeException("怎么会出现非正数次幂？");
                }
                if (operand.getKey() == 1) {
                    newOperandList.add(new Pair<>(true, operand.getValue()));
                } else {
                    // 快速幂，但是没有幂运算
                    int c = operand.getKey();
                    Value v = operand.getValue();
                    while (c != 0) {
                        if ((c & 1) != 0) {
                            newOperandList.add(new Pair<>(true, v));
                        }
                        c >>= 1;
                        if (c != 0) {
                            MulInstr mul = new MulInstr(function.getAndAddRegIdx(), v, v, basicBlock);
                            mul.setUse(v);
                            mul.setUse(v);
                            mul.insertBefore(ins);
                            v = mul;
                        }
                    }
                }
            }
        }

        return newOperandList;
    }

    private static boolean checkNeedRebuild(List<Pair<Integer, Value>> operands) {
        if (operands.size() > 2) {
            return true;    // 多于两个操作数，需要进行重排序
        }
        for (Pair<Integer, Value> operand : operands) {
            if (operand.getKey() > 1) {
                return true;    // 存在大于一的系数，需要进行合并
            }
        }
        return false;
    }

    /**
     * 判断一下使用者中是否有且只有一个同类操作（可以继续向上和并）
     */
    private static boolean checkOnlySameOp(List<Value> userList, String operatorName) {
        boolean noSame = true;
        for (Value user : userList) {
            if (user instanceof BinaryOperation binUser && isSameOperator(binUser.getOperatorName(), operatorName)) {
                if (noSame) {
                    noSame = false;
                } else {
                    return false;
                }
            }
        }
        return !noSame;
    }

    private static boolean isSameOperator(String operator1, String operator2) {
        return operator1.equals(operator2)
                || operator1.equals("add") && operator2.equals("sub")
                || operator1.equals("sub") && operator2.equals("add");
    }

    private static void mergeOperands(List<Pair<Integer, Value>> operands) {
        Iterator<Pair<Integer, Value>> iterator = operands.iterator();
        Pair<Integer, Value> prev = null;
        while (iterator.hasNext()) {
            Pair<Integer, Value> operand = iterator.next();
            if (prev != null && operand.getValue().equals(prev.getValue())) {
                prev.setKey(operand.getKey() + prev.getKey());  // 合并同类项
                iterator.remove();
            } else {
                prev = operand;
            }
        }
        operands.removeIf(pair -> pair.getKey() == 0);
    }

    /**
     * 对操作数列表进行排序，使得常数在后面、负系数项放后面（可选）、其它指令按照寄存器编号排序
     */
    private static void sortOperands(List<Pair<Integer, Value>> operands, boolean moveNegOpBack) {
        operands.sort((lp, rp) -> {
            int lk = lp.getKey();
            int rk = rp.getKey();
            Value lv = lp.getValue();
            Value rv = rp.getValue();

            if (lv instanceof IntConst lc && rv instanceof IntConst rc) {
                return Integer.compare(lc.getConstInt().intValue(), rc.getConstInt().intValue());
            }

            if (lv instanceof IntConst) return 1;
            if (rv instanceof IntConst) return -1;

            if (moveNegOpBack) {
                if (lk < 0 && rk > 0) {
                    return 1;
                } else if (lk > 0 && rk < 0) {
                    return -1;
                }
            }

            int lRegIdx;
            int rRegIdx;
            if (lv instanceof Instruction lIns) {
                lRegIdx = lIns.getRegIndex();
            } else if (lv instanceof Function.FParam lFParam) {
                lRegIdx = lFParam.getRegIdx();
            } else {
                throw new RuntimeException("不是整型常数就应该是指令或者形参");
            }
            if (rv instanceof Instruction rIns) {
                rRegIdx = rIns.getRegIndex();
            } else if (rv instanceof Function.FParam rFParam) {
                rRegIdx = rFParam.getRegIdx();
            } else {
                throw new RuntimeException("不是整型常数就应该是指令或者形参");
            }
            return Integer.compare(lRegIdx, rRegIdx);
        });
    }

    private static void addOperands(Value operand, List<Pair<Integer, Value>> operands, String operatorName,
                                    Map<BinaryOperation, List<Pair<Integer, Value>>> unfoldMap, boolean isNeg) {
        switch (operand) {
            case BinaryOperation binOp when unfoldMap.containsKey(binOp) && isSameOperator(binOp.getOperatorName(), operatorName) -> {
                for (Pair<Integer, Value> p : unfoldMap.get(binOp)) {
                    operands.add(new Pair<>(isNeg ? -p.getKey() : p.getKey(), p.getValue()));
                }
            }
            case MulInstr mulInstr when operatorName.equals("add") && (mulInstr.getOperand1() instanceof IntConst || mulInstr.getOperand2() instanceof IntConst) -> {
                if (mulInstr.getOperand1() instanceof IntConst) {
                    operands.add(new Pair<>(((IntConst) mulInstr.getOperand1()).getConstInt().intValue(), mulInstr.getOperand2()));
                } else {
                    operands.add(new Pair<>(((IntConst) mulInstr.getOperand2()).getConstInt().intValue(), mulInstr.getOperand1()));
                }
            }
            case MulInstr mulInstr when operatorName.equals("sub") && (mulInstr.getOperand1() instanceof IntConst || mulInstr.getOperand2() instanceof IntConst) -> {
                if (mulInstr.getOperand1() instanceof IntConst) {
                    int c = ((IntConst) mulInstr.getOperand1()).getConstInt().intValue() * (isNeg ? -1 : 1);
                    operands.add(new Pair<>(c, mulInstr.getOperand2()));
                } else {
                    int c = ((IntConst) mulInstr.getOperand2()).getConstInt().intValue() * (isNeg ? -1 : 1);
                    operands.add(new Pair<>(c, mulInstr.getOperand1()));
                }
            }
            case null, default -> operands.add(new Pair<>(isNeg ? -1 : 1, operand));
        }
    }
}
