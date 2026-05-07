package backend.RISCVCode;


public enum OperandType {
    imm, reg, stackRoom, label, globalVar // 操作数类型：立即数，寄存器，栈空间，label, 全局变量
}
