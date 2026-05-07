import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.HashMap;

public class Scope {
    /*
     * 该接口是Scope，用于表示作用域，最重要的是保存符号表（里面存储了符号名和对应的MyValueRef）
     */
    private final Scope fatherScope;/*父作用域，如果是全局作用域，则为null*/
    private String scopeName;
    private final IRBaseBlockRef block;//当前作用域对应的基本块
    private final HashMap<String, IRValueRef> varMap;//当前作用域的符号表

    public Scope(String scopeName,Scope fatherScope,IRBaseBlockRef block){
        this.fatherScope = fatherScope;
        this.scopeName = scopeName;
        this.varMap = new HashMap<>();
        this.block = block;
    }
    public IRBaseBlockRef findWhileBlock(){
        //在所有作用域查找while循环，并返回循环的基本块
        //如果找不到，返回null
        Scope currentScope = this;
        while(currentScope != null){
            if(currentScope.scopeName.contains("whileBody"))
                return currentScope.getBlock();
            currentScope = currentScope.getFatherScope();
        }
        return null;
    }
    public IRBaseBlockRef getBlock(){
        //获取当前作用域所属的函数名
        return block;
    }
    public Scope getFatherScope() {
        return fatherScope;
    }

    public void defineVar(String varName, IRValueRef varType){
        //向当前作用域添加变量
        varMap.put(varName,varType);
    }
    public IRValueRef findVarInCurrentScope(String varName){
        //在当前作用域查找变量，并返回变量的类型
        //如果找不到，返回null
        return varMap.get(varName);
    }
    public IRValueRef findVarInAllScope(String varName){
        //在所有作用域查找变量，并返回变量的类型
        //如果找不到，返回null
        Scope currentScope = this;
        while(currentScope != null){
            IRValueRef varType = currentScope.findVarInCurrentScope(varName);
            if(varType != null){
                return varType;
            }
            currentScope = currentScope.getFatherScope();
        }
        return null;
    }
}
