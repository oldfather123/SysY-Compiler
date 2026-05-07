package backend;

import IR.IRModule;
import IR.IRValueRef.IRFunctionBlockRef;

import java.util.List;

/**
 * 变量地址分配的接口
 */
public interface RegisterAllocator {

    void setFunction(IRFunctionBlockRef function); /*设置所在函数*/

    void setModule(IRModule module); /*设置所在module，用于判断全局变量*/

    int allocate();/*返回栈帧大小*/

    List<String> getRegister(String variable);/*返回变量对应的寄存器或栈地址*/

}

