package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstFloat;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.*;

import java.util.*;

/**
 * 模式替换 <br>
 * 基于模式匹配的方式，简化基本块内的语句 <br>
 * 功能包括：<br>
 * <ul>
 *     <li>常量折叠</li>
 *     <li>自定义的匹配替换规则</li>
 * </ul>
 */
public class PatternReplacementPass {

    private static class MatchFailedException extends Exception {
    }

    private static class Symbol extends Value {
        protected Symbol(Type type) {
            super(type);
        }

        private String name;

        @Override
        public String getValueName() {
            if (name == null) {
                name = String.format("%s#%s", this.getClass(), UUID.randomUUID());
            }
            return name;
        }

        @Override
        public String getValueNameIR() {
            return '%' + name;
        }

        @Override
        public void setValueName(String valueName) {
            name = valueName;
        }
    }

    private static class SymbolMap {
        private final HashMap<Symbol, Value> symbolMap = new HashMap<>();
        private final HashMap<Symbol, Symbol> aliasMap = new HashMap<>();

        private Symbol getRootSymbol(Symbol symbol) {
            if (!aliasMap.containsKey(symbol)) {
                return symbol;
            } else {
                var ret = getRootSymbol(aliasMap.get(symbol));
                aliasMap.put(symbol, ret);
                return ret;
            }
        }

        public void setValue(Symbol symbol, Value value) throws MatchFailedException {
            if (value instanceof Symbol symbol1) {
                alias(symbol, symbol1);
            } else {
                symbol = getRootSymbol(symbol);
                if (symbolMap.containsKey(symbol)) {
                    if (value != symbolMap.get(symbol)) {
                        // 匹配失败：同一个符号对应了不同的值
                        throw new MatchFailedException();
                    }
                } else {
                    symbolMap.put(symbol, value);
                }
            }
        }

        public void alias(Symbol u, Symbol v) throws MatchFailedException {
            u = getRootSymbol(u);
            v = getRootSymbol(v);
            if (symbolMap.containsKey(u) && symbolMap.containsKey(v)) {
                // 匹配失败：同一个符号对应了不同的值
                throw new MatchFailedException();
            }
            if (symbolMap.containsKey(v)) {
                var temp = u;
                u = v;
                v = temp;
            }
            aliasMap.put(v, u);
        }

        public Value getValue(Symbol symbol) {
            return symbolMap.get(getRootSymbol(symbol));
        }

        public void clear() {
            symbolMap.clear();
            aliasMap.clear();
        }

    }

    private abstract static class InstructionPattern {

        /**
         * 替换后这条指令应该被哪个值代替的符号
         */
        public Symbol replaceSymbol;

        /**
         * 尝试用该pattern匹配传入的instruction
         *
         * @param instruction 待测试的指令
         * @param symbolMap   符号表
         * @throws MatchFailedException 当匹配失败时
         */
        public final void match(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
            replaceSymbol = new Symbol(instruction.getType());
            match_(instruction, symbolMap);
        }

        protected abstract void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException;
    }

    private abstract static class CodePattern {

        protected final SymbolMap symbolMap = new SymbolMap();
        protected final Map<Instruction, Symbol> newValueMap = new HashMap<>();
        protected final List<InstructionPattern> patternList = new ArrayList<>();
        protected final List<Instruction> newInstructions = new ArrayList<>();

        protected Collection<InstructionPattern> getPatterns() {
            return patternList;
        }

        protected List<Instruction> getNewInstructions() {
            return newInstructions;
        }

        private void match(List<Instruction> list, int index) throws MatchFailedException {
            symbolMap.clear();
            newValueMap.clear();
            newInstructions.clear();
            for (InstructionPattern pattern : getPatterns()) {
                if (index >= list.size()) {
                    // 匹配失败：没有更多指令了
                    throw new MatchFailedException();
                }
                runPattern(pattern, list.get(index++));
            }
        }

        protected void runPattern(InstructionPattern pattern, Instruction instruction) throws MatchFailedException {
            pattern.match(instruction, symbolMap);
            newValueMap.put(instruction, pattern.replaceSymbol);
        }

        /**
         * 判断list从index开始的位置能否匹配该模式
         *
         * @param list  待匹配的列表
         * @param index 匹配开始的下标
         * @return 匹配成功返回true；否则返回false
         */
        public boolean matches(List<Instruction> list, int index) {
            try {
                match(list, index);
                // 检查被外部使用的语句是否都有对应的替代值
                for (Instruction instruction : newValueMap.keySet()) {
                    var symbol = newValueMap.get(instruction);
                    if (symbolMap.getValue(symbol) != null) continue;
                    for (Operand usage : instruction.getUsages()) {
                        if (!newValueMap.containsKey(usage.getInstruction())) {
                            // 匹配失败：外部语句使用了这个语句，但是这个语句没有替代值
                            throw new MatchFailedException();
                        }
                    }
                }
                return true;
            } catch (MatchFailedException e) {
                return false;
            }
        }

        /**
         * 执行模式替换 <br>
         * 在此之前需要先执行一次matches以收集symbol等信息 <br>
         *
         * @param list  执行替换的列表
         * @param index 匹配到的模式的开头
         * @return 匹配到的模式在原list中最后一条指令的下标
         */
        public int replace(List<Instruction> list, int index) {
            var nextInst = list.get(index);
            // 加入新指令并替换掉其中占位的 symbol
            for (Instruction newInstruction : getNewInstructions()) {
                newInstruction.insertBefore(nextInst);
                for (Operand operand : newInstruction.getOperandList()) {
                    if (operand.getValue() instanceof Symbol symbol) {
                        operand.setValue(symbolMap.getValue(symbol));
                    }
                }
            }
            // 移除旧指令
            newValueMap.forEach((oldInstruction, symbol) -> {
                oldInstruction.replaceAllUsageTo(symbolMap.getValue(symbol));
                oldInstruction.waste();
            });
            return index + newValueMap.size() - 1;
        }

    }

    private static boolean initialized = false;

    private static List<CodePattern> codePatterns;

    private static void initialize() {
        codePatterns = new ArrayList<>();

//        // o_inst1 = cmp v1 v2                  -> n_inst1
//        // o_inst2 = zext i1 o_inst1 to i32
//        // o_inst3 = icmp ne o_inst2 0          -> n_inst1
//        // n_inst1 = cmp v1 v2
//        codePatterns.add(new CodePattern() {
//            Symbol v1, v2, o_inst1, o_inst2, o_inst3, n_inst1;
//
//            {
//                patternList.add(new InstructionPattern() {
//                    @Override
//                    protected void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
//                        if (instruction instanceof CompareInst compareInst) {
//                            // 指定所有中间符号
//                            v1 = new Symbol(compareInst.getOperand1().getType());
//                            v2 = new Symbol(compareInst.getOperand2().getType());
//                            o_inst1 = new Symbol(IntegerType.getI1());
//                            o_inst2 = new Symbol(IntegerType.getI32());
//                            o_inst3 = new Symbol(IntegerType.getI1());
//                            n_inst1 = new Symbol(IntegerType.getI1());
//                            // 创建新语句
//                            CompareInst newCompareInst;
//                            if (compareInst instanceof IntegerCompareInst integerCompareInst) {
//                                newCompareInst = new IntegerCompareInst(
//                                        (IntegerType) integerCompareInst.getOperand1().getType(),
//                                        integerCompareInst.getCondition(),
//                                        v1,
//                                        v2
//                                );
//                            } else if (compareInst instanceof FloatCompareInst floatCompareInst) {
//                                newCompareInst = new FloatCompareInst(
//                                        (FloatType) floatCompareInst.getOperand1().getType(),
//                                        floatCompareInst.getCondition(),
//                                        v1,
//                                        v2
//                                );
//                            } else {
//                                throw new IllegalArgumentException("Unknown type of compare instruction.");
//                            }
//                            newInstructions.add(newCompareInst);
//                            // 设置替代值
//                            symbolMap.setValue(replaceSymbol, n_inst1);
//                            // 确定已知的值和等价关系
//                            symbolMap.setValue(v1, compareInst.getOperand1());
//                            symbolMap.setValue(v2, compareInst.getOperand2());
//                            symbolMap.setValue(o_inst1, instruction);
//                            symbolMap.setValue(n_inst1, newCompareInst);
//                        } else {
//                            throw new MatchFailedException();
//                        }
//                    }
//                });
//                patternList.add(new InstructionPattern() {
//                    @Override
//                    protected void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
//                        if (instruction instanceof ZeroExtensionInst zeroExtensionInst) {
//                            symbolMap.setValue(o_inst2, instruction);
//                            symbolMap.setValue(o_inst1, zeroExtensionInst.getSourceOperand());
//                        } else {
//                            throw new MatchFailedException();
//                        }
//                    }
//                });
//                patternList.add(new InstructionPattern() {
//                    @Override
//                    protected void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
//                        if (instruction instanceof IntegerCompareInst integerCompareInst &&
//                                integerCompareInst.getCondition() == IntegerCompareInst.Condition.NE &&
//                                integerCompareInst.getOperand2() instanceof ConstInt operand2 &&
//                                operand2.getValue() == 0) {
//                            symbolMap.setValue(replaceSymbol, n_inst1);
//                            symbolMap.setValue(o_inst3, instruction);
//                            symbolMap.setValue(o_inst2, integerCompareInst.getOperand1());
//                        } else {
//                            throw new MatchFailedException();
//                        }
//                    }
//                });
//            }
//        });

        // o_inst1 = mul|div v1 -1     -> n_inst1
        // n_inst1 = sub 0 v1
        codePatterns.add(new CodePattern() {
            Symbol v1, o_inst1, n_inst1;

            {
                patternList.add(new InstructionPattern() {
                    @Override
                    protected void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
                        if (instruction instanceof IntegerArithmeticInst integerArithmeticInst) {
                            var type = integerArithmeticInst.getType();
                            v1 = new Symbol(type);
                            o_inst1 = new Symbol(type);
                            n_inst1 = new Symbol(type);
                            symbolMap.setValue(o_inst1, instruction);
                            symbolMap.setValue(replaceSymbol, n_inst1);
                            if (integerArithmeticInst instanceof IntegerMultiplyInst) {
                                if (integerArithmeticInst.getOperand1() instanceof ConstInt constInt && constInt.getValue() == -1) {
                                    symbolMap.setValue(v1, integerArithmeticInst.getOperand2());
                                } else if (integerArithmeticInst.getOperand2() instanceof ConstInt constInt && constInt.getValue() == -1) {
                                    symbolMap.setValue(v1, integerArithmeticInst.getOperand1());
                                } else {
                                    throw new MatchFailedException();
                                }
                            } else if (integerArithmeticInst instanceof IntegerSignedDivideInst) {
                                if (integerArithmeticInst.getOperand2() instanceof ConstInt constInt && constInt.getValue() == -1) {
                                    symbolMap.setValue(v1, integerArithmeticInst.getOperand1());
                                } else {
                                    throw new MatchFailedException();
                                }
                            } else {
                                throw new MatchFailedException();
                            }
                            var newInst = new IntegerSubInst(type, type.getZeroInitialization(), v1);
                            newInstructions.add(newInst);
                            symbolMap.setValue(n_inst1, newInst);
                        } else if (instruction instanceof FloatArithmeticInst floatArithmeticInst) {
                            var type = floatArithmeticInst.getType();
                            v1 = new Symbol(type);
                            o_inst1 = new Symbol(type);
                            n_inst1 = new Symbol(type);
                            symbolMap.setValue(o_inst1, instruction);
                            symbolMap.setValue(replaceSymbol, n_inst1);
                            if (floatArithmeticInst instanceof FloatMultiplyInst) {
                                if (floatArithmeticInst.getOperand1() instanceof ConstFloat constFloat && constFloat.getValue() == -1) {
                                    symbolMap.setValue(v1, floatArithmeticInst.getOperand2());
                                } else if (floatArithmeticInst.getOperand2() instanceof ConstFloat constFloat && constFloat.getValue() == -1) {
                                    symbolMap.setValue(v1, floatArithmeticInst.getOperand1());
                                } else {
                                    throw new MatchFailedException();
                                }
                            } else if (floatArithmeticInst instanceof FloatDivideInst) {
                                if (floatArithmeticInst.getOperand2() instanceof ConstFloat constFloat && constFloat.getValue() == -1) {
                                    symbolMap.setValue(v1, floatArithmeticInst.getOperand1());
                                } else {
                                    throw new MatchFailedException();
                                }
                            } else {
                                throw new MatchFailedException();
                            }
                            var newInst = new FloatNegateInst(type, v1);
                            newInstructions.add(newInst);
                            symbolMap.setValue(n_inst1, newInst);
                        } else {
                            throw new MatchFailedException();
                        }
                    }
                });
            }
        });

//        // o_inst1 = mul v1, v2
//        // o_inst2 = div o_inst1, v2 -> v1
//        codePatterns.add(new CodePattern() {
//            Symbol o_inst1, o_inst2, v1, v2;
//
//            {
//                patternList.add(new InstructionPattern() {
//                    @Override
//                    protected void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
//                        if (instruction instanceof IntegerMultiplyInst integerMultiplyInst) {
//                            var type = integerMultiplyInst.getType();
//                            o_inst1 = new Symbol(type);
//                            o_inst2 = new Symbol(type);
//                            v1 = new Symbol(type);
//                            v2 = new Symbol(type);
//                            symbolMap.setValue(o_inst1, integerMultiplyInst);
//                            symbolMap.setValue(v1, integerMultiplyInst.getOperand1());
//                            symbolMap.setValue(v2, integerMultiplyInst.getOperand2());
//                        } else {
//                            throw new MatchFailedException();
//                        }
//                    }
//                });
//                patternList.add(new InstructionPattern() {
//                    @Override
//                    protected void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
//                        if (instruction instanceof IntegerSignedDivideInst integerSignedDivideInst) {
//                            symbolMap.setValue(replaceSymbol, v1);
//                            symbolMap.setValue(o_inst2, integerSignedDivideInst);
//                            symbolMap.setValue(o_inst1, integerSignedDivideInst.getOperand1());
//                            symbolMap.setValue(v2, integerSignedDivideInst.getOperand2());
//                        } else {
//                            throw new MatchFailedException();
//                        }
//                    }
//                });
//            }
//        });
//
//        // o_inst1 = mul v2, v1
//        // o_inst2 = div o_inst1, v2 -> v1
//        codePatterns.add(new CodePattern() {
//            Symbol o_inst1, o_inst2, v1, v2;
//
//            {
//                patternList.add(new InstructionPattern() {
//                    @Override
//                    protected void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
//                        if (instruction instanceof IntegerMultiplyInst integerMultiplyInst) {
//                            var type = integerMultiplyInst.getType();
//                            o_inst1 = new Symbol(type);
//                            o_inst2 = new Symbol(type);
//                            v1 = new Symbol(type);
//                            v2 = new Symbol(type);
//                            symbolMap.setValue(o_inst1, integerMultiplyInst);
//                            symbolMap.setValue(v2, integerMultiplyInst.getOperand1());
//                            symbolMap.setValue(v1, integerMultiplyInst.getOperand2());
//                        } else {
//                            throw new MatchFailedException();
//                        }
//                    }
//                });
//                patternList.add(new InstructionPattern() {
//                    @Override
//                    protected void match_(Instruction instruction, SymbolMap symbolMap) throws MatchFailedException {
//                        if (instruction instanceof IntegerSignedDivideInst integerSignedDivideInst) {
//                            symbolMap.setValue(replaceSymbol, v1);
//                            symbolMap.setValue(o_inst2, integerSignedDivideInst);
//                            symbolMap.setValue(o_inst1, integerSignedDivideInst.getOperand1());
//                            symbolMap.setValue(v2, integerSignedDivideInst.getOperand2());
//                        } else {
//                            throw new MatchFailedException();
//                        }
//                    }
//                });
//            }
//        });


        initialized = true;
    }

    private static boolean runOnBasicBlock(BasicBlock basicBlock) {
        boolean bbChanged = false;
        while (true) {
            var list = basicBlock.getMainInstructions();
            boolean changed = false;
            for (var i = 0; i < list.size(); i++) {
                for (CodePattern codePattern : codePatterns) {
                    if (codePattern.matches(list, i)) {
                        i = codePattern.replace(list, i);
                        changed = true;
                        bbChanged = true;
                        break;
                    }
                }
            }
            if (!changed) break;
        }
        return bbChanged;
    }

    private static boolean runOnFunction(Function function) {
        boolean changed = false;
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            changed = changed | runOnBasicBlock(basicBlock);
        }
        return changed;
    }

    public static boolean runOnModule(Module module) {
        if (!initialized) {
            initialize();
        }
        boolean changed = false;
        for (Function function : module.getFunctions()) {
            changed = changed | runOnFunction(function);
        }
        return changed;
    }
}
