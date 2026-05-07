package cn.edu.bit.newnewcc.backend.asm.allocator;

import cn.edu.bit.newnewcc.backend.asm.operand.FloatRegister;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.HashMap;
import java.util.Map;

public class FloatRegisterAllocator {
    private final Map<Instruction, FloatRegister> registerMap;

    public FloatRegisterAllocator() {
        registerMap = new HashMap<>();
    }

    public FloatRegister allocate(Instruction instruction, int index) {
        FloatRegister reg = FloatRegister.getVirtual(index);
        registerMap.put(instruction, reg);
        return reg;
    }

    public FloatRegister allocate(int index) {
        return FloatRegister.getVirtual(index);
    }

    public FloatRegister get(Instruction instruction) {
        return registerMap.get(instruction);
    }

    public boolean contain(Instruction instruction) {
        return registerMap.containsKey(instruction);
    }
}
