package cn.edu.bit.newnewcc.backend.asm.allocator;

import cn.edu.bit.newnewcc.backend.asm.operand.FloatRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.HashMap;
import java.util.Map;
import java.util.NoSuchElementException;

/**
 * 寄存器分配器，内部包含整数分配和浮点数寄存器分配功能，分配出的寄存器均为虚拟寄存器（下标为负数）
 */
public class RegisterAllocator {
    private final FloatRegisterAllocator floatRegisterAllocator = new FloatRegisterAllocator();
    private final IntRegisterAllocator intRegisterAllocator = new IntRegisterAllocator();
    private final Map<Integer, Register> vregMap = new HashMap<>();
    private int counter = 0;

    private int getNextCounter() {
        return ++counter;
    }

    public IntRegister allocateInt() {
        var reg = intRegisterAllocator.allocate(getNextCounter());
        vregMap.put(reg.getAbsoluteIndex(), reg);
        return reg;
    }
    public FloatRegister allocateFloat() {
        var reg = floatRegisterAllocator.allocate(getNextCounter());
        vregMap.put(reg.getAbsoluteIndex(), reg);
        return reg;
    }
    public IntRegister allocateInt(Instruction instruction) {
        var reg = intRegisterAllocator.allocate(instruction, getNextCounter());
        vregMap.put(reg.getAbsoluteIndex(), reg);
        return reg;
    }
    public FloatRegister allocateFloat(Instruction instruction) {
        var reg = floatRegisterAllocator.allocate(instruction, getNextCounter());
        vregMap.put(reg.getAbsoluteIndex(), reg);
        return reg;
    }
    public Register allocate(Instruction instruction) {
        if (instruction.getType() instanceof FloatType) {
            return allocateFloat(instruction);
        } else {
            return allocateInt(instruction);
        }
    }
    public Register allocate(Register sourceReg) {
        if (sourceReg instanceof IntRegister) {
            return allocateInt();
        } else {
            return allocateFloat();
        }
    }
    public Register allocate(Type type) {
        if (type instanceof FloatType) {
            return allocateFloat();
        } else {
            return allocateInt();
        }
    }
    public boolean contain(Instruction instruction) {
        return intRegisterAllocator.contain(instruction) || floatRegisterAllocator.contain(instruction);
    }

    public Register get(Instruction instruction) {
        if (intRegisterAllocator.contain(instruction)) {
            return intRegisterAllocator.get(instruction);
        } else if (floatRegisterAllocator.contain(instruction)) {
            return floatRegisterAllocator.get(instruction);
        } else {
            throw new NoSuchElementException();
        }
    }
    public Register get(Integer index) {
        return vregMap.get(index);
    }
}
