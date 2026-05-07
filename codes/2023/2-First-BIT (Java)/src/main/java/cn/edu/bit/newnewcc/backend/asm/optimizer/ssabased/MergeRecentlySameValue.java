package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.ExStackVarOffset;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;
import cn.edu.bit.newnewcc.backend.asm.util.AsmInstructions;
import cn.edu.bit.newnewcc.backend.asm.util.Registers;

import java.util.*;

public class MergeRecentlySameValue implements ISSABasedOptimizer {
    static class OpValue {
        private static final Map<Register, Integer> regMap = new HashMap<>();
        private static final Map<Integer, Integer> intMap = new HashMap<>();
        private static int operandCounter;
        public static void init() {
            regMap.clear();
            intMap.clear();
            operandCounter = 0;
        }
        public static Integer getReg(SSABasedOptimizer ssaBasedOptimizer, Register reg) {
            if (ssaBasedOptimizer.getValueSource(reg) == null && !Registers.CONSTANT_REGISTERS.contains(reg)) {
                return null;
            }
            if (!regMap.containsKey(reg)) {
                regMap.put(reg, ++operandCounter);
            }
            return regMap.get(reg);
        }
        public static Integer getImm(Immediate immediate) {
            if (immediate instanceof ExStackVarOffset) {
                return null;
            }
            var v = immediate.getValue();
            if (!intMap.containsKey(v)) {
                intMap.put(v, ++operandCounter);
            }
            return intMap.get(v);
        }
    }

    static class Value {
        int opcode, v1, v2;
        private Value(int opcode, int v1, int v2) {
            this.opcode = opcode;
            this.v1 = v1;
            this.v2 = v2;
        }
        public static Value getValue(SSABasedOptimizer ssaBasedOptimizer, AsmInstruction instruction) {
            int opcode = AsmInstructions.getCalculateInstCode(instruction);
            if (opcode < 0) {
                return null;
            }
            var op1 = instruction.getOperand(2);
            var op2 = instruction.getOperand(3);
            Integer v1 = null, v2 = null;
            if (op1 instanceof Immediate imm) {
                v1 = OpValue.getImm(imm);
            } else if (op1 instanceof Register reg) {
                v1 = OpValue.getReg(ssaBasedOptimizer, reg);
            }
            if (op2 instanceof Immediate imm) {
                v2 = OpValue.getImm(imm);
            } else if (op2 instanceof Register reg) {
                v2 = OpValue.getReg(ssaBasedOptimizer, reg);
            }
            if (v1 == null || v2 == null) {
                return null;
            }
            return new Value(opcode, v1, v2);
        }

        @Override
        public int hashCode() {
            if (AsmInstructions.isExchangeableCalculateInst(opcode)) {
                return Objects.hash(opcode, Set.copyOf(List.of(v1, v2)));
            } else {
                return Objects.hash(opcode, v1, v2);
            }
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof Value value) {
                return opcode == value.opcode && v1 == value.v1 && v2 == value.v2;
            }
            return false;
        }
    }

    private static final int TIME_LIMIT = 16;
    private final Map<Value, Register> valueSavedMap = new HashMap<>();
    private final Map<Value, Integer> valueTimeStamp = new HashMap<>();
    private int instCounter;

    @Override
    public void setBlockBegins() {
        ISSABasedOptimizer.super.setBlockBegins();
        instCounter = 0;
        valueSavedMap.clear();
        valueTimeStamp.clear();
        OpValue.init();
    }

    @Override
    public OptimizeResult getReplacement(SSABasedOptimizer ssaBasedOptimizer, AsmInstruction instruction) {
        OptimizeResult result = null;
        instCounter++;
        Value value = Value.getValue(ssaBasedOptimizer, instruction);
        if (value != null) {
            Register resultRegister = (Register) instruction.getOperand(1);
            if (resultRegister.isVirtual() && ssaBasedOptimizer.getValueSource(resultRegister) != null) {
                if (valueTimeStamp.containsKey(value) && instCounter <= valueTimeStamp.get(value) + TIME_LIMIT) {
                    Register previousRegister = valueSavedMap.get(value);
                    if (!previousRegister.equals(resultRegister)) {
                        result = OptimizeResult.getNew();
                        result.addRegisterMapping(resultRegister, previousRegister);
                    }
                } else {
                    valueSavedMap.put(value, resultRegister);
                }
                valueTimeStamp.put(value, instCounter);
            }
        }
        return result;
    }
}
