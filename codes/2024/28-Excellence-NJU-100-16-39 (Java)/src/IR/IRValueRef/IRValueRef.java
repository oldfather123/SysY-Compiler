package IR.IRValueRef;

import IR.IRType.IRType;

public interface IRValueRef {
    /*
     * 该接口是ValueRef的接口，用于表示IR中的值，类似与LLVMValueRef
     */
    String getText();
    IRType getType();/*返回该值的类型*/
    int getTypeKind();/*返回该值的类型*/
}
