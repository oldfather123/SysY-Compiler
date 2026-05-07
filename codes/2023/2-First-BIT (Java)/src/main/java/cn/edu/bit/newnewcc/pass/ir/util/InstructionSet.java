package cn.edu.bit.newnewcc.pass.ir.util;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.*;

import java.util.*;

public class InstructionSet {

    private final Set<Function> pureFunctions;
    private final Map<List<Object>, Instruction> instructionSet = new HashMap<>();

    public InstructionSet(Set<Function> pureFunctions) {
        this.pureFunctions = pureFunctions;
    }

    public void add(Instruction instruction) {
        instructionSet.put(getFeatures(instruction), instruction);
    }

    public boolean contains(Instruction instruction) {
        return instructionSet.containsKey(getFeatures(instruction));
    }

    public Instruction get(Instruction instruction) {
        return instructionSet.get(getFeatures(instruction));
    }

    /**
     * 移除指定的指令 <br>
     * 只有传入原始指令才能成功移除，传入等价指令不会导致原始指令被移除 <br>
     *
     * @param instruction 待移除的原始指令
     * @return 成功返回true；否则返回false。
     */
    public boolean remove(Instruction instruction) {
        var features = getFeatures(instruction);
        if (instructionSet.containsKey(features) && instructionSet.get(features) == instruction) {
            instructionSet.remove(features);
            return true;
        } else {
            return false;
        }
    }

    private List<Object> getFeatures(Instruction instruction) {
        List<Object> result = new ArrayList<>();
        result.add(instruction.getClass());
        // 不可合并的指令
        if (instruction instanceof TerminateInst ||
                instruction instanceof MemoryInst) {
            result.add(instruction);
        }
        // 过程调用
        else if (instruction instanceof CallInst callInst) {
            if (callInst.getCallee() instanceof Function callee && pureFunctions.contains(callee)) {
                result.add(callInst.getCallee());
                result.addAll(callInst.getArgumentList());
            } else {
                result.add(instruction);
            }
        }
        // 可交换二元算数指令
        else if (instruction instanceof FloatAddInst ||
                instruction instanceof FloatMultiplyInst ||
                instruction instanceof IntegerAddInst ||
                instruction instanceof IntegerMultiplyInst) {
            var binaryInstruction = (BinaryInstruction) instruction;
            var opSet = new HashSet<>();
            opSet.add(binaryInstruction.getOperand1());
            opSet.add(binaryInstruction.getOperand2());
            result.add(opSet);
        } else if (instruction instanceof SignedMinInst ||
                instruction instanceof SignedMaxInst) {
            var opSet = new HashSet<>();
            if (instruction instanceof SignedMinInst signedMinInst) {
                opSet.add(signedMinInst.getOperand1());
                opSet.add(signedMinInst.getOperand2());
            } else {
                SignedMaxInst signedMaxInst = (SignedMaxInst) instruction;
                opSet.add(signedMaxInst.getOperand1());
                opSet.add(signedMaxInst.getOperand2());
            }
            result.add(opSet);
        }
        // 不可交换二元算数指令
        else if (instruction instanceof FloatDivideInst ||
                instruction instanceof FloatSubInst ||
                instruction instanceof IntegerSignedDivideInst ||
                instruction instanceof IntegerSignedRemainderInst ||
                instruction instanceof IntegerSubInst) {
            var binaryInstruction = (BinaryInstruction) instruction;
            result.add(binaryInstruction.getOperand1());
            result.add(binaryInstruction.getOperand2());
        }
        // 比较指令
        else if (instruction instanceof FloatCompareInst floatCompareInst) {
            switch (floatCompareInst.getCondition()) {
                case OEQ, ONE, OLE, OLT -> {
                    result.add(floatCompareInst.getCondition());
                    result.add(floatCompareInst.getOperand1());
                    result.add(floatCompareInst.getOperand2());
                }
                case OGE, OGT -> {
                    result.add(floatCompareInst.getCondition().swap());
                    result.add(floatCompareInst.getOperand2());
                    result.add(floatCompareInst.getOperand1());
                }
            }
        } else if (instruction instanceof IntegerCompareInst integerCompareInst) {
            switch (integerCompareInst.getCondition()) {
                case EQ, NE, SLT, SLE: {
                    result.add(integerCompareInst.getCondition());
                    result.add(integerCompareInst.getOperand1());
                    result.add(integerCompareInst.getOperand2());
                }
                case SGE, SGT: {
                    result.add(integerCompareInst.getCondition().swap());
                    result.add(integerCompareInst.getOperand2());
                    result.add(integerCompareInst.getOperand1());
                }
            }
        }
        // 特殊指令
        else if (instruction instanceof PhiInst phiInst) {
            var map = new HashMap<BasicBlock, Value>();
            phiInst.forEach(map::put);
            result.add(map);
        } else if (instruction instanceof FloatNegateInst floatNegateInst) {
            result.add(floatNegateInst.getOperand1());
        } else if (instruction instanceof GetElementPtrInst getElementPtrInst) {
            result.add(getElementPtrInst.getRootOperand());
            result.add(getElementPtrInst.getIndexOperands());
        }
        // 类型转换指令
        else if (instruction instanceof FloatToSignedIntegerInst floatToSignedIntegerInst) {
            result.add(floatToSignedIntegerInst.getSourceOperand());
            result.add(floatToSignedIntegerInst.getTargetType());
        } else if (instruction instanceof SignedIntegerToFloatInst signedIntegerToFloatInst) {
            result.add(signedIntegerToFloatInst.getSourceOperand());
            result.add(signedIntegerToFloatInst.getTargetType());
        } else if (instruction instanceof ZeroExtensionInst zeroExtensionInst) {
            result.add(zeroExtensionInst.getSourceOperand());
            result.add(zeroExtensionInst.getTargetType());
        } else if (instruction instanceof SignedExtensionInst signedExtensionInst) {
            result.add(signedExtensionInst.getSourceOperand());
            result.add(signedExtensionInst.getTargetType());
        } else if (instruction instanceof TruncInst truncInst) {
            result.add(truncInst.getSourceOperand());
            result.add(truncInst.getTargetType());
        } else if (instruction instanceof BitCastInst bitCastInst) {
            result.add(bitCastInst.getSourceOperand());
            result.add(bitCastInst.getTargetType());
        }
        // 未知指令
        else {
            throw new CompilationProcessCheckFailedException("Unable to extract feature from instruction of class " + instruction.getClass());
        }
        return result;
    }

}
