package cn.edu.bit.newnewcc.backend.asm.util;

import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.operand.RegisterReplaceable;

import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class AsmInstructions {
    private AsmInstructions() {
    }

    public static Set<Integer> getWriteRegId(AsmInstruction instr) {
        if (instr instanceof AsmStore) {
            if (instr.getOperand(2) instanceof Register) {
                return Set.of(2);
            }
            return Set.of();
        }
        if (instr instanceof AsmJump || instr instanceof AsmReturn) {
            return new HashSet<>();
        }
        if (instr.getOperand(1) instanceof RegisterReplaceable) {
            return Set.of(1);
        }
        return Set.of();
    }

    public static Set<Integer> getWriteVRegId(AsmInstruction instr) {
        var result = new HashSet<Integer>();
        var regId = getWriteRegId(instr);
        for (var x : regId) {
            var reg = ((RegisterReplaceable)instr.getOperand(x)).getRegister();
            if (reg.isVirtual()) {
                result.add(x);
            }
        }
        return Collections.unmodifiableSet(result);
    }

    public static Set<Integer> getReadRegId(AsmInstruction instr) {
        var result = new HashSet<Integer>();
        for (int i = 1; i <= 3; i++) {
            if (instr.getOperand(i) instanceof RegisterReplaceable) {
                boolean flag = (instr instanceof AsmStore) ? (i == 1 || (i == 2 && !(instr.getOperand(i) instanceof Register))) :
                    (instr instanceof AsmJump || instr instanceof AsmReturn || (i > 1));
                if (flag) {
                    result.add(i);
                }
            }
        }
        return result;
    }

    public static Set<Integer> getReadVRegId(AsmInstruction instr) {
        var result = new HashSet<Integer>();
        var regId = getReadRegId(instr);
        for (var x : regId) {
            var reg = ((RegisterReplaceable) instr.getOperand(x)).getRegister();
            if (reg.isVirtual()) {
                result.add(x);
            }
        }
        return Collections.unmodifiableSet(result);
    }

    public static Set<Integer> getVRegId(AsmInstruction instr) {
        Set<Integer> result = new HashSet<>();
        result.addAll(getReadVRegId(instr));
        result.addAll(getWriteVRegId(instr));
        return Collections.unmodifiableSet(result);
    }

    public static Set<Register> getWriteRegSet(AsmInstruction instr) {
        Set<Register> result = new HashSet<>();
        for (int i : getWriteRegId(instr)) {
            RegisterReplaceable op = (RegisterReplaceable) instr.getOperand(i);
            result.add(op.getRegister());
        }
        if (instr instanceof AsmCall call && call.getReturnRegister() != null) {
            result.add(call.getReturnRegister());
        }
        return Collections.unmodifiableSet(result);
    }

    public static Set<Integer> getWriteVRegSet(AsmInstruction instr) {
        Set<Integer> result = new HashSet<>();
        for (int i : getWriteVRegId(instr)) {
            RegisterReplaceable op = (RegisterReplaceable) instr.getOperand(i);
            result.add(op.getRegister().getAbsoluteIndex());
        }
        return Collections.unmodifiableSet(result);
    }

    public static Set<Register> getReadRegSet(AsmInstruction instr) {
        Set<Register> result = new HashSet<>();
        for (int i : getReadRegId(instr)) {
            RegisterReplaceable op = (RegisterReplaceable) instr.getOperand(i);
            result.add(op.getRegister());
        }
        if (instr instanceof AsmCall call) {
            result.addAll(call.getParamRegList());
        }
        return Collections.unmodifiableSet(result);
    }

    public static Set<Integer> getReadVRegSet(AsmInstruction instr) {
        Set<Integer> result = new HashSet<>();
        for (int i : getReadVRegId(instr)) {
            RegisterReplaceable op = (RegisterReplaceable) instr.getOperand(i);
            result.add(op.getRegister().getAbsoluteIndex());
        }
        return Collections.unmodifiableSet(result);
    }

    public static boolean isMoveVToV(AsmInstruction instr) {
        return instr instanceof AsmMove && ((Register) instr.getOperand(1)).isVirtual() && ((Register) instr.getOperand(2)).isVirtual();
    }

    public static Pair<Integer, Integer> getMoveVReg(AsmInstruction instr) {
        if (!isMoveVToV(instr)) {
            throw new IllegalArgumentException();
        }
        var writeSet = AsmInstructions.getWriteVRegSet(instr);
        var readSet = AsmInstructions.getReadVRegSet(instr);
        return new Pair<>((Integer) writeSet.toArray()[0], (Integer) readSet.toArray()[0]);
    }

    public static Pair<Register, Register> getMoveReg(AsmMove instr) {
        var writeSet = AsmInstructions.getWriteRegSet(instr);
        var readSet = AsmInstructions.getReadRegSet(instr);
        return new Pair<>((Register) writeSet.toArray()[0], (Register) readSet.toArray()[0]);
    }

    public static Set<Integer> getInstRegID(AsmInstruction instr, Register reg) {
        Set<Integer> res = new HashSet<>();
        for (int i = 1; i <= 3; i++) {
            if (instr.getOperand(i) instanceof RegisterReplaceable rp && rp.getRegister().equals(reg)) {
                res.add(i);
            }
        }
        return res;
    }

    public static boolean instContainsReg(AsmInstruction instr, Register reg) {
        return getReadRegSet(instr).contains(reg) || getWriteRegSet(instr).contains(reg);
    }

    private static final Map<Object, Integer> CalculateInstClassID;
    static {
        CalculateInstClassID = Map.ofEntries(
                Map.entry(AsmAdd.class, 0),
                Map.entry(AsmAnd.class, 1),
                Map.entry(AsmFloatDivide.class, 2),
                Map.entry(AsmMax.class, 3),
                Map.entry(AsmMin.class, 4),
                Map.entry(AsmMul.class, 5),
                Map.entry(AsmShiftLeft.class, 6),
                Map.entry(AsmShiftRightArithmetic.class, 7),
                Map.entry(AsmShiftRightLogical.class, 8),
                Map.entry(AsmSignedIntegerDivide.class, 9),
                Map.entry(AsmSignedIntegerRemainder.class, 10));
    }
    public static int getCalculateInstCode(AsmInstruction instruction) {
        if (CalculateInstClassID.containsKey(instruction.getClass())) {
            return CalculateInstClassID.get(instruction.getClass());
        } else if (instruction instanceof AsmShiftLeftAdd shiftLeftAdd) {
            return 12 + shiftLeftAdd.getShiftLength();
        }
        return -1;
    }
    public static boolean isExchangeableCalculateInst(int opcode) {
        return (Set.of(0, 1, 3, 4, 5).contains(opcode));
    }
}
