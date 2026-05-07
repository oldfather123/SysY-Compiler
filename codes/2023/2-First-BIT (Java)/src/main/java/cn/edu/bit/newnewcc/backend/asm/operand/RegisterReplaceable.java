package cn.edu.bit.newnewcc.backend.asm.operand;

public interface RegisterReplaceable {
    AsmOperand withRegister(Register register);

    /**
     * 获取操作数内部存在的寄存器，注意，仅用于虚拟寄存器替换时使用
     * @return 返回操作数内部存在的寄存器
     */
    Register getRegister();
}
