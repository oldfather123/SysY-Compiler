package IR;

import IR.IRType.IRArrayType;
import IR.IRType.IRFunctionType;
import IR.IRType.IRPointerType;
import IR.IRType.IRType;
import IR.IRValueRef.*;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.*;

import static IR.IRType.IRFloatType.IRFloatType;
import static IR.IRType.IRInt32Type.IRInt32Type;

public class IRModule {
    private final String name; /*模块名*/
    private final ArrayList<IRFunctionBlockRef> functionBlocks; /*模块中的函数，对应LLVM模块中的多个函数*/
    private final LinkedHashMap<IRGlobalRegRef, IRValueRef> globalVariables; /*模块中的全局变量，对应LLVM模块中的全局变量*/
    private final StringBuilder stringBuilder; /*用于存储IR代码的字符串*/
    private static final IRType int32Type = IRInt32Type();
    private static final IRType floatType = IRFloatType();

    private IRModule(String name) {
        /*
         * 该类是IR模块的类，用于表示IR中的模块
         * 职责是存储函数块和全局变量（函数块中存储了基本块，基本块中存储了指令）
         */
        /*模块名*/
        this.functionBlocks = new ArrayList<>();
        this.globalVariables = new LinkedHashMap<>();
        this.name = name;
        this.stringBuilder = new StringBuilder();
        this.stringBuilder.append("; ModuleID = '").append(name).append("'\n");
        this.stringBuilder.append("source_filename = \"").append(name).append("\"\n\n");
        declare();
    }

    public String getName() {
        return name;
    }

    /*
     * @brief 添加运行时库的函数声明，包括IO函数，计时函数等，详细见SysY运行时库文档
     * @param void
     * @return void
     */
    private void declare() {
        stringBuilder.append("declare i32 @getint()\n");
        stringBuilder.append("declare i32 @getch()\n");
        stringBuilder.append("declare float @getfloat()\n");
        stringBuilder.append("declare i32 @getarray(i32* %0)\n");
        stringBuilder.append("declare i32 @getfarray(float* %0)\n");

        stringBuilder.append("declare void @putint(i32 %0)\n");
        stringBuilder.append("declare void @putch(i32 %0)\n");
        stringBuilder.append("declare void @putarray(i32 %0, i32* %1)\n");
        stringBuilder.append("declare void @putfloat(float %0)\n");
        stringBuilder.append("declare void @putfarray(i32 %0, float* %1)\n");

        stringBuilder.append("declare void @putf(i32* %0, ...)\n");
        stringBuilder.append("declare void @before_main()\n");
        stringBuilder.append("declare void @after_main()\n");
        stringBuilder.append("declare void @_sysy_starttime(i32 %0)\n");
        stringBuilder.append("declare void @_sysy_stoptime(i32 %0)\n");

        stringBuilder.append("\n");
    }

    //提供给前段输出ir的接口
    public static IRModule IRModuleCreateWithName(String name) {
        return new IRModule(name);
    }

    public static void IRPrintModuleToFile(IRModule module, String path) {
        //输出模块到文件
        for (IRFunctionBlockRef function : module.functionBlocks) {
            module.stringBuilder.append(function.genIRCodes());
        }
        try {
            BufferedWriter out = new BufferedWriter(new FileWriter(path));
            out.write(module.stringBuilder.toString());
            out.close();
        } catch (IOException e) {
            System.err.println("ERROR：Failed to print ir to file " + path);
        }
    }

    public static IRFunctionBlockRef IRAddFunction(IRModule module, String funcName, IRFunctionType functionType) {
        IRFunctionBlockRef function = new IRFunctionBlockRef(funcName, functionType);//创建函数
        module.addFunction(function);
        return function;
    }

    //处理全局变量
    public static IRValueRef IRAddGlobal(IRModule module, IRType type, String globalVarName) {
        //todo: array type
        IRType baseType = new IRPointerType(type);
        IRValueRef resRegister = new IRGlobalRegRef(globalVarName, baseType);
        module.append(resRegister.getText() + " =  global " + ((IRPointerType) resRegister.getType()).getBaseType().getText() + " ");
        return resRegister;
    }

    //针对全局变量的初始化
    public static void IRSetInitializer(IRModule module, IRGlobalRegRef globalVar, IRValueRef constRef) {
        //设置全局变量的初始化
        module.addGlobalVariable(globalVar, constRef);
        //打印全局变量的初始化
        //System.out.println("globalVarName: " + globalVar.getIdentity() + " constRef: " + constRef.getText());
        module.appendln(constRef.getText());
    }

    //针对数组的全局变量的初始化
    public static void IRSetInitializer(IRModule module, IRGlobalRegRef globalVar, List<IRValueRef> constValueRefList) {
        //这里为了处理数组的初始化，需要考虑到数组的维度
        boolean flag = true;
        if (!constValueRefList.isEmpty()) {
            //如果传入的constValueRefList不为空，说明指定了初始化值
            if (constValueRefList.get(0).getType() == floatType) {
                List<Float> initValue = new ArrayList<>();
                for (IRValueRef irValueRef : constValueRefList) {
                    //如果不是0，说明不是zero initializer
                    if (!Objects.equals(irValueRef.getText(), "0")) {
                        flag = false;
                    }
                    initValue.add(Float.valueOf(irValueRef.getText()));
                }
                //设置全局变量的初始化值
                IRArrayRef arrayRef = new IRArrayRef(globalVar.getIdentity(), floatType);
                arrayRef.setInitFloatValue(initValue);
                //todo：这里是否已经完全初始化了？
                arrayRef.setZero(flag);
                module.addGlobalVariable(globalVar, arrayRef);
            } else if (constValueRefList.get(0).getType() == int32Type) {
                List<Integer> initValue = new ArrayList<>();
                for (IRValueRef irValueRef : constValueRefList) {
                    if (!Objects.equals(irValueRef.getText(), "0")) {
                        flag = false;
                    }
                    initValue.add(Integer.valueOf(irValueRef.getText()));
                }
                IRArrayRef arrayRef = new IRArrayRef(globalVar.getIdentity(), int32Type);
                arrayRef.setInitIntValue(initValue);
                arrayRef.setZero(flag);
                module.addGlobalVariable(globalVar, arrayRef);
            }
        } else {
            List<Float> initValue = new ArrayList<>();
            IRArrayRef arrayRef = new IRArrayRef(globalVar.getIdentity(), floatType);
            arrayRef.setInitFloatValue(initValue);
            arrayRef.setZero(true);
            module.addGlobalVariable(globalVar, arrayRef);
        }
        if (flag) {
            //如果全部初始化为0，则直接输出zeroinitializer
            module.appendln("zeroinitializer");
            return;
        }
        //输出初始化值
        IRType elementType = ((IRPointerType) globalVar.getType()).getBaseType();
        StringBuilder emitStr = new StringBuilder();
        List<Integer> paramList = new ArrayList<>();
        while (elementType instanceof IRArrayType) {
            //计算数组的元素个数
            paramList.add(((IRArrayType) elementType).getLength());
            elementType = ((IRArrayType) elementType).getBaseType();
        }
        String typeStr;
        if (elementType == int32Type) {
            typeStr = int32Type.getText();
        } else {
            typeStr = floatType.getText();
        }
        //计算数组的维度，lastLength为数组的最后一维的长度
        int lastLength = paramList.get(paramList.size() - 1);
        elementType = new IRArrayType(elementType, lastLength);
        int temp = constValueRefList.size() / lastLength;
        int counter = 0;
        int counter1 = 0;
        //输出数组的初始化值
        emitStr.append("[".repeat(Math.max(0, paramList.size() - 1)));
        for (int i = 0; i < temp; i++) {
            if (paramList.size() != 1) {
                emitStr.append(elementType.getText());
            }

            boolean flg = true;
            for (int j = 0; j < lastLength; j++) {
                if (!constValueRefList.get(counter1++).getText().equals("0")) {
                    flg = false;
                    break;
                }
            }
            if (flg) {
                emitStr.append(" zeroinitializer");
                if (i != temp - 1) {
                    emitStr.append(", ");
                }
                counter = counter1;
            } else {
                emitStr.append(" [");
                for (int j = 0; j < lastLength; j++) {


                    if (j == lastLength - 1) {
                        emitStr.append(typeStr).append(" ").append(constValueRefList.get(counter++).getText());
                        break;
                    }
                    emitStr.append(typeStr).append(" ").append(constValueRefList.get(counter++).getText()).append(", ");
                }
                counter1 = counter;
                if (i == temp - 1) {
                    emitStr.append("]");
                } else {
                    emitStr.append("], ");
                }

            }
        }
        emitStr.append("]".repeat(Math.max(0, paramList.size() - 1)));
        module.appendln(emitStr.toString());
    }

    private void addFunction(IRFunctionBlockRef function) {
        functionBlocks.add(function);//使用ArrayList存储函数
    }

    private void addGlobalVariable(IRGlobalRegRef name, IRValueRef value) {
        globalVariables.put(name, value);
    }


    private void appendln(String code) {
        this.stringBuilder.append(code).append("\n");
    }

    public void append(String code) {//直接添加字符串到module中
        this.stringBuilder.append(code);
    }

    public LinkedHashMap<IRGlobalRegRef, IRValueRef> getGlobalVariables() {
        return globalVariables;
    }

    //提供给后端遍历module的接口
    public static IRFunctionBlockRef IRGetFirstFunction(IRModule module) {
        return module.functionBlocks.get(0);
    }

    public static IRFunctionBlockRef IRGetNextFunction(IRModule module, IRFunctionBlockRef function) {
        int index = module.functionBlocks.indexOf(function);
        if (index == module.functionBlocks.size() - 1) {
            return null;
        } else {
            return module.functionBlocks.get(index + 1);
        }
    }

    /**
     * 获取第一个全局变量
     *
     * @param module
     * @return
     */
    public static IRGlobalRegRef IRGetFirstGlobal(IRModule module) {
        if (module.globalVariables.isEmpty())
            return null;
        return (IRGlobalRegRef) module.globalVariables.keySet().toArray()[0];
    }

    /**
     * 获取下一个全局变量
     *
     * @param module
     * @param global
     * @return
     */
    public static IRGlobalRegRef IRGetNextGlobal(IRModule module, IRValueRef global) {
        for (IRGlobalRegRef key : module.globalVariables.keySet()) {
            if ((key) == global) {
                int index = new ArrayList<>(module.globalVariables.keySet()).indexOf(key);
                if (index == module.globalVariables.size() - 1) {
                    return null;
                } else {
                    return new ArrayList<>(module.globalVariables.keySet()).get(index + 1);
                }
            }
        }
        return null;
    }

    public static String IRGetValueName(IRValueRef value) {
        return value.getText();
    }

    /**
     * 获得常数操作数的值
     *
     * @param constant
     * @return
     */
    public static long IRConstIntGetSExtValue(IRValueRef constant) {
        if (constant instanceof IRConstIntRef) {
            return ((IRConstIntRef) constant).getValue();
        } else
            System.err.println("wrong number");
        return Long.MIN_VALUE;
    }

    public static long IRConstFloatGetSExtValue(IRValueRef constant) {
        if (constant instanceof IRConstFloatRef) {
            return Float.floatToIntBits(((IRConstFloatRef) constant).getValue());
        } else
            System.err.println("wrong float number");
        return Long.MIN_VALUE;
    }

    /**
     * 判断是否为常数
     *
     * @param valueRef
     * @return
     */
    public static boolean IRIsIntConstant(IRValueRef valueRef) {
        return valueRef instanceof IRConstIntRef;
    }

    public static boolean IRIsFloatConstant(IRValueRef valueRef) {
        return valueRef instanceof IRConstFloatRef;
    }

    /**
     * 获取全局变量的初始值
     *
     * @param module
     * @param global
     * @return
     */
    public static IRValueRef IRGetInitializer(IRModule module, IRValueRef global) {
        for (IRGlobalRegRef key : module.globalVariables.keySet()) {
            if ((key) == global) {
                return module.globalVariables.get(key);
            }
        }
        return null;
    }

    public static IRFunctionBlockRef IRGetNamedFunction(IRModule module, String name) {
        for (IRFunctionBlockRef functionBlockRef : module.functionBlocks) {
            if (functionBlockRef.getFunctionName().equals(name)) {
                return functionBlockRef;
            }
        }
        return null;
    }

    /**
     * 判断是否为全局变量
     *
     * @param module
     * @param operand
     * @return
     */
    public static boolean IRIsAGlobalVariable(IRModule module, IRValueRef operand) {
        for (IRValueRef globalVar : module.getGlobalVariables().keySet()) {
            if ((operand).equals(globalVar))
                return true;
        }
        return false;
    }

    public ArrayList<IRFunctionBlockRef> getFunctionBlocks() {
        return this.functionBlocks;
    }
}
