package backend.risc_v.entity.register;


import backend.risc_v.handler.RegisterManager;

/**
 * Register类用于表示RISC-V架构下的寄存器。
 * 支持区分整型和浮点型寄存器，记录分配变量名、锁定状态及溢出回写需求。
 * 提供寄存器属性的设置与获取方法，便于寄存器分配与管理。
 */
public class Register {
    private final int index;
    private final boolean isIntRegister;


    public Register(int index, boolean isIntRegister) {
        this.index = index;
        this.isIntRegister = isIntRegister;
    }


    @Override
    public String toString() {
        return (isIntRegister ? "x" : "f") + index;
    }
}
