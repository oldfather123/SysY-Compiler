package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.value.Instruction;

/**
 * 涉及内存的指令
 * <p>
 * 此类仅用于分类，无实际含义
 */
public abstract class MemoryInst extends Instruction {
    public MemoryInst(Type type) {
        super(type);
    }
}
