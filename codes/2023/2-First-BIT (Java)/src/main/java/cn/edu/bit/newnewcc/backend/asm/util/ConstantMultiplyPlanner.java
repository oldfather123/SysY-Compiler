package cn.edu.bit.newnewcc.backend.asm.util;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class ConstantMultiplyPlanner {
    /**
     * 构建乘法值的操作数
     */
    public static abstract class Operand {
        /**
         * 操作的等级，每进行一次算数运算加一级
         */
        public final int level;

        protected Operand(int level) {
            this.level = level;
        }

    }

    /**
     * 常数
     */
    public static class ConstantOperand extends Operand {
        public final int value;

        public ConstantOperand(int value) {
            super(0);
            this.value = value;
        }

    }

    /**
     * 乘法的原始值
     */
    public static class VariableOperand extends Operand {
        private VariableOperand() {
            super(0);
        }

        private static final VariableOperand instance = new VariableOperand();

        public static VariableOperand getInstance() {
            return instance;
        }

    }

    /**
     * 用于标记某个值不可削减
     */
    public static class NotReducible extends Operand {
        private NotReducible() {
            super(0x3fffffff); // 使用该值以避免两个值相加产生溢出
        }

        private static final NotReducible instance = new NotReducible();

        public static NotReducible getInstance() {
            return instance;
        }

    }

    /**
     * 另一个操作作为操作数
     */
    public static class Operation extends Operand {
        public enum Type {
            ADD, SUB, SHL
        }

        public final Type type;
        public final Operand operand1, operand2;

        public Operation(Type type, Operand operand1, Operand operand2) {
            super(operand1.level + operand2.level + 1);
            this.type = type;
            this.operand1 = operand1;
            this.operand2 = operand2;
        }

    }

    private static final int OPERATION_LIMIT = 3;

    private static final Map<Long, Operand> operandMap = new HashMap<>();

    private static boolean isInitialized = false;

    private static void addOperationIfPossible(ArrayList<Pair<Long, Operand>> levelLdOperations, long idAdd, Operand odAdd) {
        if (!operandMap.containsKey(idAdd)) {
            operandMap.put(idAdd, odAdd);
            levelLdOperations.add(new Pair<>(idAdd, odAdd));
        }
    }

    private static void initialize() {
        ArrayList<ArrayList<Pair<Long, Operand>>> operations = new ArrayList<>();
        var level0Operations = new ArrayList<Pair<Long, Operand>>();
        addOperationIfPossible(level0Operations, 0, new ConstantOperand(0));
        addOperationIfPossible(level0Operations, 1, VariableOperand.getInstance());
        operations.add(level0Operations);
        // ld -> level dest
        // id -> integer dest
        // i1 -> integer 1
        // o1 -> operand 1
        for (int ld = 1; ld <= OPERATION_LIMIT; ld++) {
            var levelLdOperations = new ArrayList<Pair<Long, Operand>>();
            for (int l1 = 0; l1 < ld; l1++) {
                // SHL
                for (var pair1 : operations.get(l1)) {
                    var i1 = pair1.a;
                    var o1 = pair1.b;
                    if (i1 == 0) continue;
                    for (int i = 0; ; i++) {
                        var idShl = i1 << i;
                        if (i != 0) {
                            var odShl = new Operation(Operation.Type.SHL, o1, new ConstantOperand(i));
                            addOperationIfPossible(levelLdOperations, idShl, odShl);
                        }
                        if ((idShl & (1L << 63)) != 0) break;
                    }
                }
                for (int l2 = 0; l1 + l2 < ld; l2++) {
                    // ADD, SUB
                    for (var pair1 : operations.get(l1)) {
                        var i1 = pair1.a;
                        var o1 = pair1.b;
                        for (var pair2 : operations.get(l2)) {
                            var i2 = pair2.a;
                            var o2 = pair2.b;
                            var idAdd = i1 + i2;
                            var odAdd = new Operation(Operation.Type.ADD, o1, o2);
                            addOperationIfPossible(levelLdOperations, idAdd, odAdd);
                            var idSub = i1 - i2;
                            var odSub = new Operation(Operation.Type.SUB, o1, o2);
                            addOperationIfPossible(levelLdOperations, idSub, odSub);
                        }
                    }
                }
            }
            operations.add(levelLdOperations);
        }
        for (ArrayList<Pair<Long, Operand>> operationList : operations) {
            for (Pair<Long, Operand> pair : operationList) {
                operandMap.put(pair.a, pair.b);
            }
        }
        isInitialized = true;
    }

    public static Operand makePlan(long multipliedConstant) {
        if (!isInitialized) initialize();
        return operandMap.getOrDefault(multipliedConstant, NotReducible.getInstance());
    }
}
