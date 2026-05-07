package cn.edu.bit.newnewcc.backend.asm;

import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.*;
import cn.edu.bit.newnewcc.backend.asm.util.ConstantMultiplyPlanner;
import cn.edu.bit.newnewcc.backend.asm.util.Utility;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.type.PointerType;
import cn.edu.bit.newnewcc.ir.value.BaseFunction;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Constant;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstArray;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInteger;
import cn.edu.bit.newnewcc.ir.value.constant.ConstLong;
import cn.edu.bit.newnewcc.ir.value.instruction.*;
import cn.edu.bit.newnewcc.pass.ir.structure.I32ValueRangeAnalyzer;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static java.lang.Math.abs;


/**
 * 汇编基本块
 */
public class AsmBasicBlock {
    private final Label blockLabel;
    private final AsmLabel instBlockLabel;
    private final AsmFunction function;
    private final I32ValueRangeAnalyzer rangeAnalyzer;
    private final BasicBlock irBlock;
    private final Map<BasicBlock, List<AsmInstruction>> phiOperation = new HashMap<>();
    private final Map<BasicBlock, Map<Value, List<Register>>> phiValueMap = new HashMap<>();

    public AsmBasicBlock(AsmFunction function, BasicBlock block, I32ValueRangeAnalyzer rangeAnalyzer) {
        this.function = function;
        this.rangeAnalyzer = rangeAnalyzer;
        this.blockLabel = new Label(function.getFunctionName() + "_" + block.getValueName(), false);
        this.irBlock = block;
        this.instBlockLabel = new AsmLabel(blockLabel);
    }

    /**
     * 生成基本块的汇编代码，向函数中输出指令
     */
    public void emitToFunction() {
        function.appendInstruction(instBlockLabel);
        for (Instruction instruction : irBlock.getInstructions()) {
            translate(instruction);
        }
        function.appendInstruction(new AsmBlockEnd());
    }

    public AsmLabel getInstBlockLabel() {
        return instBlockLabel;
    }

    public Label getBlockLabel() {
        return blockLabel;
    }

    public void preTranslatePhiInstructions() {
        List<PhiInst> phiInstList = new ArrayList<>();
        for (var inst : irBlock.getInstructions()) {
            if (inst instanceof PhiInst phiInst) {
                phiInstList.add(phiInst);
            } else {
                break;
            }
        }
        for (var inst : phiInstList) {
            Register tmp = function.getRegisterAllocator().allocate(inst);
            for (var block : inst.getEntrySet()) {
                if (!phiOperation.containsKey(block)) {
                    phiOperation.put(block, new ArrayList<>());
                    phiValueMap.put(block, new HashMap<>());
                }
                Value source = inst.getValue(block);
                if (source instanceof Constant constant) {
                    AsmOperand constantVar = function.getConstantVar(constant, (ins) -> phiOperation.get(block).add(ins));
                    if (constantVar instanceof Register)
                        phiOperation.get(block).add(new AsmMove(tmp, (Register) constantVar));
                    else
                        phiOperation.get(block).add(new AsmLoad(tmp, constantVar));
                } else {
                    if (!phiValueMap.get(block).containsKey(source)) {
                        phiValueMap.get(block).put(source, new ArrayList<>());
                    }
                    phiValueMap.get(block).get(source).add(tmp);
                }
            }
        }
    }

    private void sufTranslatePhiInstructions() {
        for (BasicBlock block : irBlock.getExitBlocks()) {
            var next = function.getBasicBlock(block);
            if (next.phiValueMap.containsKey(irBlock)) {
                function.appendAllInstruction(next.phiOperation.get(irBlock));
                for (Value value : next.phiValueMap.get(irBlock).keySet()) {
                    Register reg = function.getValueToRegister(value);
                    for (var ar : next.phiValueMap.get(irBlock).get(value)) {
                        function.appendInstruction(new AsmMove(ar, reg));
                    }
                }
            }
        }
    }

    private void translatePhiInst(PhiInst phiInst) {
        var tmp = (Register) function.getValue(phiInst);
        var reg = function.getRegisterAllocator().allocate(phiInst);
        function.appendInstruction(new AsmMove(reg, tmp));
    }

    private void translateBinaryInstruction(BinaryInstruction binaryInstruction) {
        if (binaryInstruction instanceof IntegerAddInst integerAddInst) {
            int bitLength = integerAddInst.getType().getBitWidth();
            var addx = function.getOperandToIntRegister(function.getValue(integerAddInst.getOperand1()));
            var addy = function.getValue(integerAddInst.getOperand2());
            IntRegister register = function.getRegisterAllocator().allocateInt(integerAddInst);
            function.appendInstruction(new AsmAdd(register, addx, addy, bitLength));
        } else if (binaryInstruction instanceof IntegerSubInst integerSubInst) {
            int bitLength = integerSubInst.getType().getBitWidth();
            var subx = function.getOperandToIntRegister(function.getValue(integerSubInst.getOperand1()));
            var suby = function.getValue(integerSubInst.getOperand2());
            IntRegister register = function.getRegisterAllocator().allocateInt(integerSubInst);
            if (suby instanceof Immediate) {
                function.appendInstruction(new AsmAdd(register, subx, new Immediate(-((Immediate) suby).getValue()), 64));
            } else {
                function.appendInstruction(new AsmSub(register, subx, (IntRegister) suby, bitLength));
            }
        } else if (binaryInstruction instanceof IntegerMultiplyInst integerMultiplyInst) {
            translateIntegerMultiplyInst(integerMultiplyInst);
        } else if (binaryInstruction instanceof IntegerSignedDivideInst integerSignedDivideInst) {
            translateIntegerSignedDivideInst(integerSignedDivideInst);
        } else if (binaryInstruction instanceof IntegerSignedRemainderInst integerSignedRemainderInst) {
            translateIntegerSignedRemainderInst(integerSignedRemainderInst);
        } else if (binaryInstruction instanceof CompareInst compareInst) {
            translateCompareInst(compareInst);
        } else if (binaryInstruction instanceof FloatAddInst floatAddInst) {
            var rx = function.getOperandToFloatRegister(function.getValue(floatAddInst.getOperand1()));
            var ry = function.getOperandToFloatRegister(function.getValue(floatAddInst.getOperand2()));
            FloatRegister result = function.getRegisterAllocator().allocateFloat(floatAddInst);
            function.appendInstruction(new AsmAdd(result, rx, ry));
        } else if (binaryInstruction instanceof FloatSubInst floatSubInst) {
            var rx = function.getOperandToFloatRegister(function.getValue(floatSubInst.getOperand1()));
            var ry = function.getOperandToFloatRegister(function.getValue(floatSubInst.getOperand2()));
            FloatRegister result = function.getRegisterAllocator().allocateFloat(floatSubInst);
            function.appendInstruction(new AsmSub(result, rx, ry));
        } else if (binaryInstruction instanceof FloatMultiplyInst floatMultiplyInst) {
            var rx = function.getOperandToFloatRegister(function.getValue(floatMultiplyInst.getOperand1()));
            var ry = function.getOperandToFloatRegister(function.getValue(floatMultiplyInst.getOperand2()));
            FloatRegister result = function.getRegisterAllocator().allocateFloat(floatMultiplyInst);
            function.appendInstruction(new AsmMul(result, rx, ry));
        } else if (binaryInstruction instanceof FloatDivideInst floatDivideInst) {
            var rx = function.getOperandToFloatRegister(function.getValue(floatDivideInst.getOperand1()));
            var ry = function.getOperandToFloatRegister(function.getValue(floatDivideInst.getOperand2()));
            FloatRegister result = function.getRegisterAllocator().allocateFloat(floatDivideInst);
            function.appendInstruction(new AsmFloatDivide(result, rx, ry));
        } else if (binaryInstruction instanceof ShiftLeftInst shiftLeftInst) {
            int bitLength = shiftLeftInst.getType().getBitWidth();
            var shx = function.getOperandToIntRegister(function.getValue(shiftLeftInst.getOperand1()));
            var shy = function.getValue(shiftLeftInst.getOperand2());
            IntRegister result = function.getRegisterAllocator().allocateInt(shiftLeftInst);
            function.appendInstruction(new AsmShiftLeft(result, shx, shy, bitLength));
        } else {
            throw new RuntimeException("inst type not translated : " + binaryInstruction);
        }
    }

    private void translateIntegerSignedRemainderInst(IntegerSignedRemainderInst integerSignedRemainderInst) {
        if (integerSignedRemainderInst.getOperand2() instanceof ConstInt constInt) {
            if (rangeAnalyzer.getValueRangeAtBlock(
                    integerSignedRemainderInst.getOperand1(), integerSignedRemainderInst.getBasicBlock()
            ).minValue() >= 0 && Utility.isPowerOf2(constInt.getValue())) {
                var mask = constInt.getValue() - 1;
                var reg1 = (IntRegister) function.getValueToRegister(integerSignedRemainderInst.getOperand1());
                var finalReg = (IntRegister) function.getRegisterAllocator().allocate(integerSignedRemainderInst);
                var immMask = function.getValue(ConstInt.getInstance(mask));
                function.appendInstruction(new AsmAnd(finalReg, reg1, immMask));
            } else {
                var a = integerSignedRemainderInst.getOperand1();
                var b = integerSignedRemainderInst.getOperand2();
                var inst1 = new IntegerSignedDivideInst(integerSignedRemainderInst.getType(), a, b);
                var inst2 = new IntegerMultiplyInst(integerSignedRemainderInst.getType(), inst1, b);
                var inst3 = new IntegerSubInst(integerSignedRemainderInst.getType(), a, inst2);
                inst1.insertBefore(integerSignedRemainderInst);
                inst2.insertBefore(integerSignedRemainderInst);
                inst3.insertBefore(integerSignedRemainderInst);
                translate(inst1);
                translate(inst2);
                translate(inst3);
                var inst3Reg = function.getRegisterAllocator().get(inst3);
                var finalReg = function.getRegisterAllocator().allocate(integerSignedRemainderInst);
                function.appendInstruction(new AsmMove(finalReg, inst3Reg));
            }
        } else {
            int bitLength = integerSignedRemainderInst.getType().getBitWidth();
            var divx = function.getOperandToIntRegister(function.getValue(integerSignedRemainderInst.getOperand1()));
            var divy = function.getOperandToIntRegister(function.getValue(integerSignedRemainderInst.getOperand2()));
            IntRegister register = function.getRegisterAllocator().allocateInt(integerSignedRemainderInst);
            function.appendInstruction(new AsmSignedIntegerRemainder(register, divx, divy, bitLength));
        }
    }

    public AsmOperand translateConstantMultiplyPlan(IntRegister variable, ConstantMultiplyPlanner.Operand plan, int bitWidth) {
        if (plan instanceof ConstantMultiplyPlanner.VariableOperand) {
            return variable;
        } else if (plan instanceof ConstantMultiplyPlanner.ConstantOperand constantOperand) {
            return function.getValue(ConstInt.getInstance(constantOperand.value));
        } else if (plan instanceof ConstantMultiplyPlanner.Operation operation) {
            var reg1 = function.getOperandToIntRegister(translateConstantMultiplyPlan(variable, operation.operand1, bitWidth));
            var operand2 = translateConstantMultiplyPlan(variable, operation.operand2, bitWidth);
            var finalRegister = function.getRegisterAllocator().allocateInt();
            var asmInstruction = switch (operation.type) {
                case ADD -> new AsmAdd(finalRegister, reg1, operand2, bitWidth);
                case SUB -> new AsmSub(finalRegister, reg1, (IntRegister) operand2, bitWidth);
                case SHL -> new AsmShiftLeft(finalRegister, reg1, operand2, bitWidth);
            };
            function.appendInstruction(asmInstruction);
            return finalRegister;
        } else {
            throw new CompilationProcessCheckFailedException("Unknown type of operand: " + plan.getClass());
        }
    }

    private void translateIntegerMultiplyInst(IntegerMultiplyInst integerMultiplyInst) {
        if (integerMultiplyInst.getOperand1() instanceof ConstInteger ||
                integerMultiplyInst.getOperand2() instanceof ConstInteger) {
            IntRegister variable;
            long multipliedConstant;
            if (integerMultiplyInst.getOperand1() instanceof ConstInteger constInteger) {
                variable = (IntRegister) function.getValueToRegister(integerMultiplyInst.getOperand2());
                multipliedConstant = ConstInteger.valueOf(constInteger);
            } else {
                variable = (IntRegister) function.getValueToRegister(integerMultiplyInst.getOperand1());
                multipliedConstant = ConstInteger.valueOf((ConstInteger) integerMultiplyInst.getOperand2());
            }
            var plan = ConstantMultiplyPlanner.makePlan(multipliedConstant);
            if (!(plan instanceof ConstantMultiplyPlanner.NotReducible)) {
                var planReg = translateConstantMultiplyPlan(variable, plan, integerMultiplyInst.getType().getBitWidth());
                var finalReg = function.getRegisterAllocator().allocateInt(integerMultiplyInst);
                if (planReg instanceof Register)
                    function.appendInstruction(new AsmMove(finalReg, (Register) planReg));
                else
                    function.appendInstruction(new AsmLoad(finalReg, planReg));
                return;
            }
        }
        // 默认翻译方法
        int bitLength = integerMultiplyInst.getType().getBitWidth();
        var mulx = function.getOperandToIntRegister(function.getValue(integerMultiplyInst.getOperand1()));
        var muly = function.getOperandToIntRegister(function.getValue(integerMultiplyInst.getOperand2()));
        IntRegister register = function.getRegisterAllocator().allocateInt(integerMultiplyInst);
        function.appendInstruction(new AsmMul(register, mulx, muly, bitLength));
    }

    private void translateIntegerSignedDivideInst(IntegerSignedDivideInst integerSignedDivideInst) {
        if (integerSignedDivideInst.getOperand2() instanceof ConstInt constI32Divisor) {
            if (rangeAnalyzer.getValueRangeAtBlock(
                    integerSignedDivideInst.getOperand1(), integerSignedDivideInst.getBasicBlock()
            ).minValue() >= 0 && constI32Divisor.getValue() >= 0) {
                translateUnsignedConstIntDivideInst(integerSignedDivideInst, constI32Divisor);
            } else {
                translateSignedConstIntDivideInst(integerSignedDivideInst, constI32Divisor);
            }
            return;
        }
        // 默认翻译方法
        int bitLength = integerSignedDivideInst.getType().getBitWidth();
        var divx = function.getOperandToIntRegister(function.getValue(integerSignedDivideInst.getOperand1()));
        var divy = function.getOperandToIntRegister(function.getValue(integerSignedDivideInst.getOperand2()));
        IntRegister register = function.getRegisterAllocator().allocateInt(integerSignedDivideInst);
        function.appendInstruction(new AsmSignedIntegerDivide(register, divx, divy, bitLength));
    }

    private void translateUnsignedConstIntDivideInst(IntegerSignedDivideInst integerUnsignedDivideInst, ConstInt constI32Divisor) {
        int divisor = constI32Divisor.getValue();
        IntRegister regAns = (IntRegister) function.getRegisterAllocator().allocate(integerUnsignedDivideInst);
        if (Utility.isPowerOf2(divisor)) {
            int x = Utility.log2(divisor);
            if (x == 0) {
                var tmp = function.getValueToRegister(integerUnsignedDivideInst.getOperand1());
                var result = function.getRegisterAllocator().allocateInt(integerUnsignedDivideInst);
                function.appendInstruction(new AsmLoad(result, tmp));
            } else if (x >= 1 && x <= 30) {
                var src = (IntRegister) function.getValueToRegister(integerUnsignedDivideInst.getOperand1());
                var immX = function.getValue(ConstInt.getInstance(x));
                // %ans = srliw %src, #x
                function.appendInstruction(new AsmShiftRightLogical(regAns, src, immX, 32));
            }
        } else {
            int l = Utility.log2(divisor);
            int sh = l;
            var temp = new BigInteger("1");
            long low = temp.shiftLeft(32 + l).divide(BigInteger.valueOf(divisor)).longValue();
            long high = temp.shiftLeft(32 + l).add(temp.shiftLeft(l + 1)).divide(BigInteger.valueOf(divisor)).longValue();
            while (((low / 2) < (high / 2)) && sh > 0) {
                low /= 2;
                high /= 2;
                sh--;
            }
            if (high < (1L << 31)) {
                // %1 = mul %src, #high
                // %2 = srai %1, #(32+sh)
                // %3 = sraiw %src, #31
                // %ans = subw %2, %3
                var src = (IntRegister) function.getValueToRegister(integerUnsignedDivideInst.getOperand1());
                var reg1 = function.getRegisterAllocator().allocateInt();
                var immHigh = function.getValue(ConstLong.getInstance(high));
                function.appendInstruction(new AsmMul(reg1, src, function.getOperandToIntRegister(immHigh), 64));
                var reg2 = function.getRegisterAllocator().allocateInt();
                var imm32PlusSh = function.getValue(ConstInt.getInstance(32 + sh));
                function.appendInstruction(new AsmShiftRightArithmetic(reg2, reg1, imm32PlusSh, 64));
                var reg3 = function.getRegisterAllocator().allocateInt();
                var imm31 = function.getValue(ConstInt.getInstance(31));
                function.appendInstruction(new AsmShiftRightArithmetic(reg3, src, imm31, 32));
                function.appendInstruction(new AsmSub(regAns, reg2, reg3, 32));
            } else {
                high = high - (1L << 32);
                // %1 = mul %src, #high
                // %2 = srai %1, #32
                // %3 = addw %2, %src
                // %4 = sariw %3, #sh
                // %5 = sariw %src, #31
                // %ans = subw %4, %5
                var reg1 = function.getRegisterAllocator().allocateInt();
                var src = (IntRegister) function.getValueToRegister(integerUnsignedDivideInst.getOperand1());
                var immHigh = function.getValue(ConstLong.getInstance(high));
                function.appendInstruction(new AsmMul(reg1, src, function.getOperandToIntRegister(immHigh), 64));
                var reg2 = function.getRegisterAllocator().allocateInt();
                var imm32 = function.getValue(ConstInt.getInstance(32));
                function.appendInstruction(new AsmShiftRightArithmetic(reg2, reg1, imm32, 64));
                var reg3 = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(new AsmAdd(reg3, reg2, src, 32));
                var reg4 = function.getRegisterAllocator().allocateInt();
                var immSh = function.getValue(ConstInt.getInstance(sh));
                function.appendInstruction(new AsmShiftRightArithmetic(reg4, reg3, immSh, 32));
                var reg5 = function.getRegisterAllocator().allocateInt();
                var imm31 = function.getValue(ConstInt.getInstance(31));
                function.appendInstruction(new AsmShiftRightArithmetic(reg5, src, imm31, 32));
                function.appendInstruction(new AsmSub(regAns, reg4, reg5, 32));
            }
        }
    }

    private void translateSignedConstIntDivideInst(IntegerSignedDivideInst integerSignedDivideInst, ConstInt constI32Divisor) {
        int divisor = abs(constI32Divisor.getValue());
        IntRegister regAns;
        // 除数小于0时，regAns得到的不是真正的答案，不能用instruction去allocate
        if (constI32Divisor.getValue() < 0) {
            regAns = function.getRegisterAllocator().allocateInt();
        } else {
            regAns = (IntRegister) function.getRegisterAllocator().allocate(integerSignedDivideInst);
        }
        if (Utility.isPowerOf2(divisor)) {
            int x = Utility.log2(divisor);
            if (x == 0) {
                var tmp = function.getValueToRegister(integerSignedDivideInst.getOperand1());
                var result = function.getRegisterAllocator().allocateInt(integerSignedDivideInst);
                function.appendInstruction(new AsmLoad(result, tmp));
            } else if (x >= 1 && x <= 30) {
                // %1 = srli %src, #(64-x)
                // %2 = addw %src, %1
                // %ans = sraiw %2, #x
                var src = (IntRegister) function.getValueToRegister(integerSignedDivideInst.getOperand1());
                var reg1 = function.getRegisterAllocator().allocateInt();
                var imm64SubX = function.getValue(ConstInt.getInstance(64 - x));
                function.appendInstruction(new AsmShiftRightLogical(reg1, src, imm64SubX, 64));
                var reg2 = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(new AsmAdd(reg2, src, reg1, 32));
                var immX = function.getValue(ConstInt.getInstance(x));
                function.appendInstruction(new AsmShiftRightArithmetic(regAns, reg2, immX, 32));
            }
        } else {
            int l = Utility.log2(divisor);
            int sh = l;
            var temp = new BigInteger("1");
            long low = temp.shiftLeft(32 + l).divide(BigInteger.valueOf(divisor)).longValue();
            long high = temp.shiftLeft(32 + l).add(temp.shiftLeft(l + 1)).divide(BigInteger.valueOf(divisor)).longValue();
            while (((low / 2) < (high / 2)) && sh > 0) {
                low /= 2;
                high /= 2;
                sh--;
            }
            if (high < (1L << 31)) {
                // %1 = mul %src, #high
                // %2 = srai %1, #(32+sh)
                // %3 = sraiw %src, #31
                // %ans = subw %2, %3
                var src = (IntRegister) function.getValueToRegister(integerSignedDivideInst.getOperand1());
                var reg1 = function.getRegisterAllocator().allocateInt();
                var immHigh = function.getValue(ConstLong.getInstance(high));
                function.appendInstruction(new AsmMul(reg1, src, function.getOperandToIntRegister(immHigh), 64));
                var reg2 = function.getRegisterAllocator().allocateInt();
                var imm32PlusSh = function.getValue(ConstInt.getInstance(32 + sh));
                function.appendInstruction(new AsmShiftRightArithmetic(reg2, reg1, imm32PlusSh, 64));
                var reg3 = function.getRegisterAllocator().allocateInt();
                var imm31 = function.getValue(ConstInt.getInstance(31));
                function.appendInstruction(new AsmShiftRightArithmetic(reg3, src, imm31, 32));
                function.appendInstruction(new AsmSub(regAns, reg2, reg3, 32));
            } else {
                high = high - (1L << 32);
                // %1 = mul %src, #high
                // %2 = srai %1, #32
                // %3 = addw %2, %src
                // %4 = sariw %3, #sh
                // %5 = sariw %src, #31
                // %ans = subw %4, %5
                var reg1 = function.getRegisterAllocator().allocateInt();
                var src = (IntRegister) function.getValueToRegister(integerSignedDivideInst.getOperand1());
                var immHigh = function.getValue(ConstLong.getInstance(high));
                function.appendInstruction(new AsmMul(reg1, src, function.getOperandToIntRegister(immHigh), 64));
                var reg2 = function.getRegisterAllocator().allocateInt();
                var imm32 = function.getValue(ConstInt.getInstance(32));
                function.appendInstruction(new AsmShiftRightArithmetic(reg2, reg1, imm32, 64));
                var reg3 = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(new AsmAdd(reg3, reg2, src, 32));
                var reg4 = function.getRegisterAllocator().allocateInt();
                var immSh = function.getValue(ConstInt.getInstance(sh));
                function.appendInstruction(new AsmShiftRightArithmetic(reg4, reg3, immSh, 32));
                var reg5 = function.getRegisterAllocator().allocateInt();
                var imm31 = function.getValue(ConstInt.getInstance(31));
                function.appendInstruction(new AsmShiftRightArithmetic(reg5, src, imm31, 32));
                function.appendInstruction(new AsmSub(regAns, reg4, reg5, 32));
            }
        }
        if (constI32Divisor.getValue() < 0) {
            var regActualAns = (IntRegister) function.getRegisterAllocator().allocate(integerSignedDivideInst);
            var imm0 = function.getValue(ConstInt.getInstance(0));
            function.appendInstruction(new AsmSub(regActualAns, function.getOperandToIntRegister(imm0), regAns, 32));
        }
    }

    private void translateCompareInst(CompareInst compareInst) {
        if (compareInst.getUsages().size() == 0) return;
        if (compareInst instanceof IntegerCompareInst integerCompareInst) {
            // 如果某个条件只被一个条件跳转语句使用，则不翻译他，该指令会被直接合并到条件跳转指令中
            if (compareInst.getUsages().size() == 1 &&
                    compareInst.getUsages().get(0).getInstruction() instanceof BranchInst) return;
            translateIntegerCompareInst(integerCompareInst);
        } else if (compareInst instanceof FloatCompareInst floatCompareInst) {
            translateFloatCompareInst(floatCompareInst);
        }
    }

    private void translateIntegerCompareInst(IntegerCompareInst integerCompareInst) {
        var rop1 = function.getOperandToIntRegister(function.getValue(integerCompareInst.getOperand1()));
        var rop2 = function.getOperandToIntRegister(function.getValue(integerCompareInst.getOperand2()));
        var result = function.getRegisterAllocator().allocateInt(integerCompareInst);
        IntRegister tmp;
        int bitLength = Math.toIntExact(integerCompareInst.getOperand1().getType().getSize() * 8);
        switch (integerCompareInst.getCondition()) {
            case EQ -> {
                tmp = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(new AsmSub(tmp, rop1, rop2, bitLength));
                function.appendInstruction(AsmIntegerCompare.createEQZ(result, tmp));
            }
            case NE -> {
                tmp = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(new AsmSub(tmp, rop1, rop2, bitLength));
                function.appendInstruction(AsmIntegerCompare.createNEZ(result, tmp));
            }
            case SLT -> function.appendInstruction(AsmIntegerCompare.createLT(result, rop1, rop2));
            case SLE -> {
                tmp = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(AsmIntegerCompare.createLT(tmp, rop2, rop1));
                function.appendInstruction(AsmIntegerCompare.createEQZ(result, tmp));
            }
            case SGT -> function.appendInstruction(AsmIntegerCompare.createLT(result, rop2, rop1));
            case SGE -> {
                tmp = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(AsmIntegerCompare.createLT(tmp, rop1, rop2));
                function.appendInstruction(AsmIntegerCompare.createEQZ(result, tmp));
            }
        }
    }

    private void translateFloatCompareInst(FloatCompareInst floatCompareInst) {
        var result = function.getRegisterAllocator().allocateInt(floatCompareInst);
        var op1 = function.getOperandToFloatRegister(function.getValue(floatCompareInst.getOperand1()));
        var op2 = function.getOperandToFloatRegister(function.getValue(floatCompareInst.getOperand2()));
        IntRegister tmp;
        switch (floatCompareInst.getCondition()) {
            case OLE -> function.appendInstruction(AsmFloatCompare.createLE(result, op1, op2));
            case OEQ -> function.appendInstruction(AsmFloatCompare.createEQ(result, op1, op2));
            case OLT -> function.appendInstruction(AsmFloatCompare.createLT(result, op1, op2));
            case ONE -> {
                tmp = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(AsmFloatCompare.createEQ(tmp, op1, op2));
                function.appendInstruction(AsmIntegerCompare.createEQZ(result, tmp));
            }
            case OGE -> function.appendInstruction(AsmFloatCompare.createLE(result, op2, op1));
            case OGT -> function.appendInstruction(AsmFloatCompare.createLT(result, op2, op1));
        }
    }

    private void translateBranchWithZero(Value value, IntegerCompareInst.Condition condition, Label trueLabel) {
        var operand = (IntRegister) function.getValueToRegister(value);
        function.appendInstruction(AsmJump.createUnary(switch (condition) {
            case EQ -> AsmJump.Condition.EQZ;
            case NE -> AsmJump.Condition.NEZ;
            case SLT -> AsmJump.Condition.LTZ;
            case SLE -> AsmJump.Condition.LEZ;
            case SGT -> AsmJump.Condition.GTZ;
            case SGE -> AsmJump.Condition.GEZ;
        }, trueLabel, operand));
    }

    private void translateBranchInst(BranchInst branchInst) {
        sufTranslatePhiInstructions();
        var trueLabel = function.getJumpLabel(branchInst.getTrueExit());
        var falseLabel = function.getJumpLabel(branchInst.getFalseExit());
        if (branchInst.getCondition() instanceof IntegerCompareInst integerCompareInst) {
            if (integerCompareInst.getOperand1() instanceof ConstInteger constInteger &&
                    ConstInteger.valueOf(constInteger) == 0) {
                translateBranchWithZero(integerCompareInst.getOperand2(), integerCompareInst.getCondition().swap(), trueLabel);
            } else if (integerCompareInst.getOperand2() instanceof ConstInteger constInteger &&
                    ConstInteger.valueOf(constInteger) == 0) {
                translateBranchWithZero(integerCompareInst.getOperand1(), integerCompareInst.getCondition(), trueLabel);
            } else {
                var operand1 = (IntRegister) function.getValueToRegister(integerCompareInst.getOperand1());
                var operand2 = (IntRegister) function.getValueToRegister(integerCompareInst.getOperand2());
                function.appendInstruction(AsmJump.createBinary(switch (integerCompareInst.getCondition()) {
                    case EQ -> AsmJump.Condition.EQ;
                    case NE -> AsmJump.Condition.NE;
                    case SLT -> AsmJump.Condition.LT;
                    case SLE -> AsmJump.Condition.LE;
                    case SGT -> AsmJump.Condition.GT;
                    case SGE -> AsmJump.Condition.GE;
                }, trueLabel, operand1, operand2));
            }
        } else {
            var condition = function.getOperandToIntRegister(function.getValue(branchInst.getCondition()));
            function.appendInstruction(AsmJump.createNEZ(trueLabel, condition));
        }
        function.appendInstruction(AsmJump.createUnconditional(falseLabel));
    }

    private void translateStoreInst(StoreInst storeInst) {
        var address = function.getValue(storeInst.getAddressOperand());
        var valueOperand = storeInst.getValueOperand();
        if (valueOperand instanceof ConstArray constArray) {
            final MemoryAddress addressStore = function.getAddress(address);
            Utility.workOnArray(constArray, 0, (Long offset, Constant item) -> {
                if (item instanceof ConstInt constInt) {
                    MemoryAddress dest = function.getOperandToAddress(addressStore.addOffset(offset));
                    Immediate source = new Immediate(constInt.getValue());
                    IntRegister tmp = function.getRegisterAllocator().allocateInt();
                    function.appendInstruction(new AsmLoad(tmp, source));
                    function.appendInstruction(new AsmStore(tmp, dest, 32));
                }
            }, (Long offset, Long length) -> {
                for (long i = 0; i < length; i += 4) {
                    MemoryAddress dest = function.getOperandToAddress(addressStore.addOffset(offset));
                    function.appendInstruction(new AsmStore(IntRegister.ZERO, dest, 32));
                    offset += 4;
                }
            });
        } else {
            var source = function.getValue(valueOperand);
            int bitLength = 32;
            if (valueOperand.getType() instanceof IntegerType integerType) {
                bitLength = integerType.getBitWidth();
            }
            if (source instanceof Register register) {
                if (address instanceof Register)
                    function.appendInstruction(new AsmLoad((Register) address, register));
                else if (address instanceof MemoryAddress)
                    function.appendInstruction(new AsmStore(register, (MemoryAddress) address, bitLength));
                else
                    function.appendInstruction(new AsmStore(register, (StackVar) address));
            } else {
                Register register;
                if (storeInst.getValueOperand().getType() instanceof FloatType) {
                    register = function.getRegisterAllocator().allocateFloat();
                } else {
                    register = function.getRegisterAllocator().allocateInt();
                }
                if (source instanceof MemoryAddress) {
                    function.appendInstruction(new AsmLoad(register, (MemoryAddress) source, bitLength));
                    if (address instanceof Register)
                        function.appendInstruction(new AsmLoad((Register) address, register));
                    else if (address instanceof MemoryAddress)
                        function.appendInstruction(new AsmStore(register, (MemoryAddress) address, bitLength));
                    else
                        function.appendInstruction(new AsmStore(register, (StackVar) address));
                } else {
                    function.appendInstruction(new AsmLoad(register, source));
                    if (address instanceof Register)
                        function.appendInstruction(new AsmLoad((Register) address, register));
                    else if (address instanceof MemoryAddress)
                        function.appendInstruction(new AsmStore(register, (MemoryAddress) address, bitLength));
                    else
                        function.appendInstruction(new AsmStore(register, (StackVar) address));
                }
            }
        }
    }

    private void translateCallInst(CallInst callInst) {
        BaseFunction baseFunction = callInst.getCallee();
        AsmFunction asmFunction = function.getGlobalCode().getFunction(baseFunction);
        List<AsmOperand> parameters = new ArrayList<>();
        var typeList = baseFunction.getParameterTypes();
        for (int i = 0; i < asmFunction.getParameterSize(); i++) {
            parameters.add(function.getValueByType(callInst.getArgumentAt(i), typeList.get(i)));
        }
        Register returnRegister = null;
        if (asmFunction.getReturnRegister() != null) {
            returnRegister = function.getRegisterAllocator().allocate(callInst);
        }
        function.appendAllInstruction(function.call(asmFunction, parameters, returnRegister));
    }

    private void translateJumpInst(JumpInst jumpInst) {
        sufTranslatePhiInstructions();
        var targetLabel = function.getJumpLabel(jumpInst.getExit());
        function.appendInstruction(AsmJump.createUnconditional(targetLabel));
    }

    private void translateZeroExtensionInst(ZeroExtensionInst zeroExtensionInst) {
        var source = function.getValue(zeroExtensionInst.getSourceOperand());
        var result = function.getRegisterAllocator().allocateInt(zeroExtensionInst);
        if (source instanceof Register)
            function.appendInstruction(new AsmMove(result, (Register) source));
        else
            function.appendInstruction(new AsmLoad(result, source));
    }

    //寄存器中的值已经是符号扩展过后的64位数，因此无需专门sext
    private void translateSignedExtensionInst(SignedExtensionInst signedExtensionInst) {
        Register tmp = function.getValueToRegister(signedExtensionInst.getSourceOperand());
        Register reg = function.getRegisterAllocator().allocate(signedExtensionInst);
        function.appendInstruction(new AsmMove(reg, tmp));
    }

    private void translateLoadInst(LoadInst loadInst) {
        var source = function.getValue(loadInst.getAddressOperand());
        Register register = function.getRegisterAllocator().allocate(loadInst);
        if (loadInst.getType() instanceof IntegerType integerType) {
            if (source instanceof Immediate immediate) {
                function.appendInstruction(new AsmLoad(register, immediate));
            } else if (source instanceof Register reg) {
                function.appendInstruction(new AsmMove(register, reg));
            } else if (source instanceof MemoryAddress memoryAddress) {
                function.appendInstruction(new AsmLoad(register, memoryAddress, integerType.getBitWidth()));
            } else if (source instanceof ExStackVarContent) {
                var stackAddress = function.getOperandToAddress(source);
                function.appendInstruction(new AsmLoad(register, stackAddress, integerType.getBitWidth()));
            } else {
                throw new RuntimeException();
            }
        } else if (loadInst.getType() instanceof PointerType) {
            if (source instanceof Immediate immediate) {
                function.appendInstruction(new AsmLoad(register, immediate));
            } else if (source instanceof Register reg) {
                function.appendInstruction(new AsmMove(register, reg));
            } else if (source instanceof MemoryAddress memoryAddress) {
                function.appendInstruction(new AsmLoad(register, memoryAddress, 64));
            } else if (source instanceof ExStackVarContent) {
                var stackAddress = function.getOperandToAddress(source);
                function.appendInstruction(new AsmLoad(register, stackAddress, 64));
            } else {
                throw new RuntimeException();
            }
        } else {
            if (source instanceof Immediate immediate) {
                function.appendInstruction(new AsmLoad(register, immediate));
            } else if (source instanceof Register reg) {
                function.appendInstruction(new AsmMove(register, reg));
            } else if (source instanceof MemoryAddress memoryAddress) {
                function.appendInstruction(new AsmLoad(register, memoryAddress, 32));
            } else if (source instanceof ExStackVarContent) {
                var stackAddress = function.getOperandToAddress(source);
                function.appendInstruction(new AsmLoad(register, stackAddress, 32));
            } else {
                throw new RuntimeException();
            }
        }
    }

    private void translateReturnInst(ReturnInst returnInst) {
        var returnValue = returnInst.getReturnValue();
        if (returnValue.getType() instanceof IntegerType) {
            var ret = function.getValue(returnInst.getReturnValue());
            if (ret instanceof Register)
                function.appendInstruction(new AsmMove(function.getReturnRegister(), (Register) ret));
            else
                function.appendInstruction(new AsmLoad(function.getReturnRegister(), ret));
        } else if (returnValue.getType() instanceof FloatType) {
            var ret = function.getOperandToFloatRegister(function.getValue(returnInst.getReturnValue()));
            function.appendInstruction(new AsmMove(function.getReturnRegister(), ret));
        }
        function.appendInstruction(AsmJump.createUnconditional(function.getRetBlockLabel()));
    }

    private void translateAllocateInst(AllocateInst allocateInst) {
        var allocatedType = allocateInst.getAllocatedType();
        var sz = (allocatedType instanceof PointerType) ? 8 : allocatedType.getSize();
        sz += (4 - sz % 4) % 4;
        function.getStackAllocator().allocate(allocateInst, Math.toIntExact(sz));
    }

    private void translateGetElementPtrInst(GetElementPtrInst getElementPtrInst) {
        MemoryAddress baseAddress = function.getOperandToAddress(function.getValue(getElementPtrInst.getRootOperand()));
        long offset = baseAddress.getOffset();
        IntRegister baseRegister = baseAddress.getRegister();
        var rootType = getElementPtrInst.getRootOperand().getType();
        for (int i = 0; i < getElementPtrInst.getIndicesSize(); i++) {
            var index = function.getValue(getElementPtrInst.getIndexAt(i));
            long baseSize = ((PointerType) GetElementPtrInst.inferDereferencedType(rootType, i + 1)).getBaseType().getSize();
            if (index instanceof Immediate immediate) {
                offset += immediate.getValue() * baseSize;
            }
        }
        IntRegister offsetR = function.getRegisterAllocator().allocateInt();
        function.appendInstruction(new AsmLoad(offsetR, new Immediate(Math.toIntExact(offset))));
        for (int i = 0; i < getElementPtrInst.getIndicesSize(); i++) {
            var index = function.getValue(getElementPtrInst.getIndexAt(i));
            long baseSize = ((PointerType) GetElementPtrInst.inferDereferencedType(rootType, i + 1)).getBaseType().getSize();
            if (!(index instanceof Immediate)) {
                IntRegister tmp = function.getRegisterAllocator().allocateInt();
                IntRegister t2 = function.getRegisterAllocator().allocateInt();
                IntRegister t3 = function.getRegisterAllocator().allocateInt();
                function.appendInstruction(new AsmMove(tmp, function.getOperandToIntRegister(index)));
                IntRegister muly = function.getOperandToIntRegister(new Immediate(Math.toIntExact(baseSize)));
                function.appendInstruction(new AsmMul(t2, tmp, muly, 64));
                function.appendInstruction(new AsmAdd(t3, offsetR, t2, 64));
                offsetR = t3;
            }
        }
        IntRegister t4 = function.getRegisterAllocator().allocateInt();
        function.appendInstruction(new AsmAdd(t4, offsetR, baseRegister, 64));
        offsetR = t4;
        function.getAddressAllocator().allocate(getElementPtrInst, new MemoryAddress(0, offsetR));
    }

    private void translateFloatNegateInst(FloatNegateInst floatNegateInst) {
        FloatRegister result = function.getRegisterAllocator().allocateFloat(floatNegateInst);
        FloatRegister source = function.getOperandToFloatRegister(function.getValue(floatNegateInst.getOperand1()));
        function.appendInstruction(new AsmFloatNegate(result, source));
    }

    private void translateFloatToSignedIntegerInst(FloatToSignedIntegerInst floatToSignedIntegerInst) {
        IntRegister result = function.getRegisterAllocator().allocateInt(floatToSignedIntegerInst);
        FloatRegister source = function.getOperandToFloatRegister(function.getValue(floatToSignedIntegerInst.getSourceOperand()));
        function.appendInstruction(new AsmConvertFloatInt(result, source));
    }

    private void translateSignedIntegerToFloatInst(SignedIntegerToFloatInst signedIntegerToFloatInst) {
        FloatRegister result = function.getRegisterAllocator().allocateFloat(signedIntegerToFloatInst);
        IntRegister source = function.getOperandToIntRegister(function.getValue(signedIntegerToFloatInst.getSourceOperand()));
        function.appendInstruction(new AsmConvertFloatInt(result, source));
    }

    private void translateBitCastInst(BitCastInst bitCastInst) {
        var address = function.getOperandToAddress(function.getValue(bitCastInst.getSourceOperand())).getAddress();
        function.getAddressAllocator().allocate(bitCastInst, address);
    }

    private void translateSignedMaxInst(SignedMaxInst signedMaxInst) {
        var op1 = (IntRegister) function.getValueToRegister(signedMaxInst.getOperand1());
        var op2 = (IntRegister) function.getValueToRegister(signedMaxInst.getOperand2());
        var regFinal = (IntRegister) function.getRegisterAllocator().allocate(signedMaxInst);
        function.appendInstruction(new AsmMax(regFinal, op1, op2));
    }

    private void translateSignedMinInst(SignedMinInst signedMinInst) {
        var op1 = (IntRegister) function.getValueToRegister(signedMinInst.getOperand1());
        var op2 = (IntRegister) function.getValueToRegister(signedMinInst.getOperand2());
        var regFinal = (IntRegister) function.getRegisterAllocator().allocate(signedMinInst);
        function.appendInstruction(new AsmMin(regFinal, op1, op2));
    }

    private void translateTruncInst(TruncInst truncInst) {
        var reg = function.getRegisterAllocator().allocateInt(truncInst);
        var source = function.getValue(truncInst.getSourceOperand());
        var rs = function.getOperandToIntRegister(source);
        function.appendInstruction(new AsmMove(reg, rs));
    }

    private void translate(Instruction instruction) {
        if (instruction instanceof ReturnInst returnInst) {
            translateReturnInst(returnInst);
        } else if (instruction instanceof AllocateInst allocateInst) {
            translateAllocateInst(allocateInst);
        } else if (instruction instanceof LoadInst loadInst) {
            translateLoadInst(loadInst);
        } else if (instruction instanceof ZeroExtensionInst zeroExtensionInst) {
            translateZeroExtensionInst(zeroExtensionInst);
        } else if (instruction instanceof SignedExtensionInst signedExtensionInst) {
            translateSignedExtensionInst(signedExtensionInst);
        } else if (instruction instanceof BranchInst branchInst) {
            translateBranchInst(branchInst);
        } else if (instruction instanceof JumpInst jumpInst) {
            translateJumpInst(jumpInst);
        } else if (instruction instanceof StoreInst storeInst) {
            translateStoreInst(storeInst);
        } else if (instruction instanceof BinaryInstruction binaryInstruction) {
            translateBinaryInstruction(binaryInstruction);
        } else if (instruction instanceof CallInst callInst) {
            translateCallInst(callInst);
        } else if (instruction instanceof GetElementPtrInst getElementPtrInst) {
            translateGetElementPtrInst(getElementPtrInst);
        } else if (instruction instanceof FloatNegateInst floatNegateInst) {
            translateFloatNegateInst(floatNegateInst);
        } else if (instruction instanceof FloatToSignedIntegerInst floatToSignedIntegerInst) {
            translateFloatToSignedIntegerInst(floatToSignedIntegerInst);
        } else if (instruction instanceof SignedIntegerToFloatInst signedIntegerToFloatInst) {
            translateSignedIntegerToFloatInst(signedIntegerToFloatInst);
        } else if (instruction instanceof BitCastInst bitCastInst) {
            translateBitCastInst(bitCastInst);
        } else if (instruction instanceof PhiInst phiInst) {
            translatePhiInst(phiInst);
        } else if (instruction instanceof SignedMinInst signedMinInst) {
            translateSignedMinInst(signedMinInst);
        } else if (instruction instanceof SignedMaxInst signedMaxInst) {
            translateSignedMaxInst(signedMaxInst);
        } else if (instruction instanceof TruncInst truncInst) {
            translateTruncInst(truncInst);
        } else {
            throw new IllegalArgumentException();
        }
    }
}
