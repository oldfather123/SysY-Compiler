import IR.IRBuilder;
import IR.IRConst;
import IR.IRInstruction.IRInstruction;
import IR.IRInstruction.ReturnInstruction;
import IR.IRModule;
import IR.IRType.*;
import IR.IRValueRef.*;
import IR.optimizer.GlobalArrToLocal;
import IR.optimizer.OptimizerFactory;
import antlr.SysYParser;
import antlr.SysYParserBaseVisitor;

import java.util.*;

import static IR.IRBuilder.*;
import static IR.IRModule.*;
import static IR.IRValueRef.IRBaseBlockRef.IRAppendBasicBlock;
import static IR.IRValueRef.IRFunctionBlockRef.getParamsCount;

public class IRVisitor extends SysYParserBaseVisitor<IRValueRef> {
    /*
     * 该类是IRBuilder的访问者类，用于访问抽象语法树并构建IR(等价与IRVisitor类）
     */
    Scope currentScope = new Scope("globalScope", null, null);
    private final IRModule module;
    private final IRBuilder builder;
    private final IRType i32Type;
    private final IRType i1Type;
    private final IRType floatType;
    private final IRType voidType;
    private final IRValueRef zero;
    private final IRValueRef floatZero;
    private final Map<String, Integer> ConstIntVarMap;
    private final Map<String, Float> ConstFloatVarMap;
    private IRFunctionBlockRef currentFunction = null;
    private IRBaseBlockRef currentBlock = null;
    private String currentBlockName = null;
    private int localScopeCounter;
    private boolean arrayAddr;

    public IRVisitor(String source) {
        //创建module
        //截取最后一个/后的字符串作为module的名字
        String[] temp = source.split("/");
        String[] temp2 = temp[temp.length - 1].split("\\.");
        this.module = IRModule.IRModuleCreateWithName(temp2[0]);
        //初始化IRBuilder，后续将使用这个builder去生成IR IR
        this.builder = new IRBuilder();
        //初始化基本类型
        this.i32Type = IRInt32Type.IRInt32Type();
        this.i1Type = IRInt1Type.IRInt1Type();
        this.floatType = IRFloatType.IRFloatType();
        this.voidType = IRVoidType.IRVoidType();
        this.zero = new IRConstIntRef(0, i32Type);
        this.floatZero = new IRConstFloatRef(0.0f);
        this.ConstIntVarMap = new LinkedHashMap<>();
        this.ConstFloatVarMap = new LinkedHashMap<>();
        this.arrayAddr = false;
        this.localScopeCounter = 0;
    }

    // 调用优化器进行优化
    public void optimizeModule() {
        OptimizerFactory optimizerFactory = new OptimizerFactory();
//        GlobalArrToLocal globalArrToLocal = new GlobalArrToLocal();
//        globalArrToLocal.Optimize(module);
        for (IRFunctionBlockRef function : this.module.getFunctionBlocks()) {
            optimizerFactory.optimize(function);
        }
    }

    //visit方法的部分
    @Override
    public IRValueRef visitProgram(SysYParser.ProgramContext ctx) {
        addLibs(currentScope);
        IRValueRef result = super.visitProgram(ctx);
        // 生成IR后进行优化
        optimizeModule();

        //打印优化后的ir
//        System.out.println("Instructions:");
//        for (IRFunctionBlockRef function : this.module.getFunctionBlocks()) {
//            for (IRBaseBlockRef block : function.getBaseBlocks()) {
//                System.out.println("Block: " + block.getLabel());
//                List<IRInstruction> instructions = block.getInstructionList();
//                for (IRInstruction instruction : instructions) {
//                    System.out.println(instruction);
//                }
//                System.out.println();
//            }
//        }
//        System.out.println();

        return result;
    }

    private void addLibs(Scope globalScope) {
        IRFunctionType getIntType = new IRFunctionType(new ArrayList<>(), i32Type);
        globalScope.defineVar("getint", new IRFunctionBlockRef("getint", getIntType));

        IRFunctionType getchType = new IRFunctionType(new ArrayList<>(), i32Type);
        globalScope.defineVar("getch", new IRFunctionBlockRef("getch", getchType));

        IRFunctionType getFloatType = new IRFunctionType(new ArrayList<>(), floatType);
        globalScope.defineVar("getfloat", new IRFunctionBlockRef("getfloat", getFloatType));

        List<IRType> getArrayTypeParams = new ArrayList<>();
        getArrayTypeParams.add(new IRPointerType(i32Type));
        IRFunctionType getArrayType = new IRFunctionType(getArrayTypeParams, i32Type);
        globalScope.defineVar("getarray", new IRFunctionBlockRef("getarray", getArrayType));

        List<IRType> getFArrayTypeParams = new ArrayList<>();
        getFArrayTypeParams.add(new IRPointerType(floatType));
        IRFunctionType getFArrayType = new IRFunctionType(getFArrayTypeParams, i32Type);
        globalScope.defineVar("getfarray", new IRFunctionBlockRef("getfarray", getFArrayType));


        List<IRType> putIntTypeParams = new ArrayList<>();
        putIntTypeParams.add(i32Type);
        IRFunctionType putIntType = new IRFunctionType(putIntTypeParams, voidType);
        globalScope.defineVar("putint", new IRFunctionBlockRef("putint", putIntType));

        List<IRType> putChTypeParams = new ArrayList<>();
        putChTypeParams.add(i32Type);
        IRFunctionType putChType = new IRFunctionType(putChTypeParams, voidType);
        globalScope.defineVar("putch", new IRFunctionBlockRef("putch", putChType));

        List<IRType> putArrayTypeParams = new ArrayList<>();
        putArrayTypeParams.add(i32Type);
        putArrayTypeParams.add(new IRPointerType(i32Type));
        IRFunctionType putArrayType = new IRFunctionType(putArrayTypeParams, voidType);
        globalScope.defineVar("putarray", new IRFunctionBlockRef("putarray", putArrayType));

        List<IRType> putFloatTypeParams = new ArrayList<>();
        putFloatTypeParams.add(floatType);
        IRFunctionType putFloatType = new IRFunctionType(putFloatTypeParams, voidType);
        globalScope.defineVar("putfloat", new IRFunctionBlockRef("putfloat", putFloatType));

        List<IRType> putFArrayTypeParams = new ArrayList<>();
        putFArrayTypeParams.add(i32Type);
        putFArrayTypeParams.add(new IRPointerType(floatType));
        IRFunctionType putFArrayType = new IRFunctionType(putFArrayTypeParams, voidType);
        globalScope.defineVar("putfarray", new IRFunctionBlockRef("putfarray", putFArrayType));

        //void putf(char a[], ...)
        List<IRType> putFTypeParams = new ArrayList<>();
        putFTypeParams.add(new IRPointerType(i32Type));
        IRFunctionType putFType = new IRFunctionType(putFTypeParams, voidType);
        globalScope.defineVar("putf", new IRFunctionBlockRef("putf", putFType));

        List<IRType> startTimeTypeParams = new ArrayList<>();
        IRFunctionType startTimeType = new IRFunctionType(startTimeTypeParams, voidType);
        globalScope.defineVar("starttime", new IRFunctionBlockRef("starttime", startTimeType));

        List<IRType> endTimeTypeParams = new ArrayList<>();
        IRFunctionType endTimeType = new IRFunctionType(endTimeTypeParams, voidType);
        globalScope.defineVar("stoptime", new IRFunctionBlockRef("stoptime", endTimeType));


        IRFunctionType beforeMainType = new IRFunctionType(new ArrayList<>(), voidType);
        globalScope.defineVar("before_main", new IRFunctionBlockRef("before_main", beforeMainType));

        IRFunctionType afterMainType = new IRFunctionType(new ArrayList<>(), voidType);
        globalScope.defineVar("after_main", new IRFunctionBlockRef("after_main", afterMainType));

        List<IRType> sysyStartTimeParams = new ArrayList<>();
        sysyStartTimeParams.add(i32Type);
        IRFunctionType sysyStartTime = new IRFunctionType(sysyStartTimeParams, voidType);
        globalScope.defineVar("_sysy_starttime", new IRFunctionBlockRef("_sysy_starttime", sysyStartTime));

        List<IRType> sysyEndTimeParams = new ArrayList<>();
        sysyEndTimeParams.add(i32Type);
        IRFunctionType sysyEndTime = new IRFunctionType(sysyEndTimeParams, voidType);
        globalScope.defineVar("_sysy_stoptime", new IRFunctionBlockRef("_sysy_stoptime", sysyEndTime));

    }

    @Override
    public IRValueRef visitFuncDef(SysYParser.FuncDefContext ctx) {
        //生成返回值类型
        IRType returnType;
        if (ctx.funcType().getText().equals("int")) {
            returnType = i32Type;
        } else if (ctx.funcType().getText().equals("float")) {
            returnType = floatType;
        } else {
            returnType = voidType;
        }
        //生成函数类型
        IRFunctionType ft;
        //获取函数参数个数

        if (ctx.funcFParams() != null) {
            int argumentCount = ctx.funcFParams().funcFParam().size();
            //生成函数参数类型
            List<IRType> argumentTypes = new ArrayList<>(argumentCount);
            for (int i = 0; i < argumentCount; i++) {
                String paramTypeName = ctx.funcFParams().funcFParam(i).bType().getText();
                if (ctx.funcFParams().funcFParam(i).L_BRACKT(0) != null) {
                    //如果是数组类型
                    if (ctx.funcFParams().funcFParam(i).L_BRACKT().size() > 1) {
                        //多维数组，先确定基本类型，再加上多维数组的维度
                        IRType baseType = judgeType(paramTypeName);
                        for (int j = ctx.funcFParams().funcFParam(i).L_BRACKT().size() - 1; j >= 1; j--) {
                            //获取数组的维度
                            IRValueRef tempValue = ctx.funcFParams().funcFParam(i).exp(j - 1).accept(this);
                            //如果是常数，直接获取其值，否则计算其值
                            if (tempValue instanceof IRConstIntRef) {
                                baseType = new IRArrayType(baseType, Integer.parseInt(tempValue.getText()));
                            } else {
                                //如果不是常数，通过访问exp节点获取其值,不确定是否正确
                                baseType = new IRArrayType(baseType, Integer.parseInt(ctx.funcFParams().funcFParam(i).exp(j - 1).getText()));
                            }
                        }
                        argumentTypes.add(new IRPointerType(baseType));
                    } else {
                        //一维数组
                        argumentTypes.add(new IRPointerType(judgeType(paramTypeName)));
                    }
                } else {
                    argumentTypes.add(judgeType(paramTypeName));
                }
            }
            ft = new IRFunctionType(argumentTypes, returnType);
        } else {
            ft = new IRFunctionType(new ArrayList<>(), returnType);
        }
        //生成函数，即向之前创建的module中添加函数
        IRFunctionBlockRef function;
        if (ctx.IDENT().getText().equals("main")) {
            function = IRAddFunction(module, /*functionName:String*/"main", ft);
        } else {
            function = IRAddFunction(module, /*functionName:String*/ctx.IDENT().getText(), ft);
        }
        currentFunction = function;
        //通过如下语句在函数中加入基本块，一个函数可以加入多个基本块
        if (ctx.IDENT().getText().equals("main")) {
            IRBaseBlockRef block = IRAppendBasicBlock(function, /*blockName:String*/"mainEntry");
            IRPositionBuilderAtEnd(builder, block);
            currentBlock = block;
            currentBlockName = "mainEntry";
            currentScope = new Scope(currentBlockName, currentScope, block);
        } else {
            String funcName = ctx.IDENT().getText() + "Entry";
            currentBlockName = funcName;
            IRBaseBlockRef block = IRAppendBasicBlock(function, /*blockName:String*/funcName);
            IRPositionBuilderAtEnd(builder, block);
            currentBlock = block;
            currentScope = new Scope(currentBlockName, currentScope, block);
        }
        int numParams = getParamsCount(function); // 获取函数的参数数量
        //将函数参数加入符号表
        for (int i = 0; i < numParams; i++) {
            String varName = ctx.funcFParams().funcFParam(i).IDENT().getText();
            IRType paramType = function.getParam(i).getType();
            IRValueRef var = IRBuildAlloca(builder, paramType, varName);
            IRBuildStore(builder, (function).getParam(i), var);
            currentScope.defineVar(varName, var);
        }
        visitBlock(ctx.block());
        //找是否有return语句，如果没有，加上return
        boolean flag = false;
        for (int i = 0; i < ctx.block().blockItem().size(); i++) {
            if (ctx.block().blockItem(i).stmt() instanceof SysYParser.Stmt_with_returnContext) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            if (returnType == i32Type) {
                IRBuildRet(builder, zero);
            } else {
                IRBuildRet(builder, null);
            }
        }
        //切换作用域
        currentScope = currentScope.getFatherScope();
        return null;
    }

    @Override
    public IRValueRef visitBlock(SysYParser.BlockContext ctx) {
        currentBlockName = currentBlockName + localScopeCounter;
        localScopeCounter++;
        currentScope = new Scope(currentBlockName, currentScope, currentBlock);
        IRValueRef ret = super.visitBlock(ctx);
        currentScope = currentScope.getFatherScope();

        return ret;
    }

    @Override
    public IRValueRef visitReturnStmt(SysYParser.ReturnStmtContext ctx) {
        //生成返回值根据返回值
        if (ctx.exp() != null) {
            IRValueRef result = visit(ctx.exp());
            IRType retType = currentFunction.getRetType();
            if (null != result && !retType.equals(result.getType())) {
                if (retType.equals(i32Type)) {
                    result = typeTransfer(builder, result, IRConst.FloatToInt);
                } else if (retType.equals(floatType)) {
                    result = typeTransfer(builder, result, IRConst.IntToFloat);
                }
            }
            return IRBuildRet(builder, /*result:IRValueRef*/result);
        } else {
            return IRBuildRet(builder, null);
        }
    }

    @Override
    public IRValueRef visitStmt_with_assign(SysYParser.Stmt_with_assignContext ctx) {
        IRValueRef lValPointer = this.visitLVal(ctx.lVal());
        IRValueRef exp = this.visit(ctx.exp());
        IRType lValType = lValPointer.getType() instanceof IRPointerType ? ((IRPointerType) lValPointer.getType()).getBaseType() : lValPointer.getType();
        lValType = lValType instanceof IRFunctionType ? ((IRFunctionType) lValType).getReturnType() : lValType;
        if (!(lValType.equals(exp.getType()))) {
            if (lValType.equals(i32Type)) {
                exp = typeTransfer(builder, exp, IRConst.FloatToInt);
            } else if (lValType.equals(floatType)) {
                exp = typeTransfer(builder, exp, IRConst.IntToFloat);
            }
        }
        return IRBuildStore(builder, exp, lValPointer);
    }

    @Override
    public IRValueRef visitExp_with_funcRParams(SysYParser.Exp_with_funcRParamsContext ctx) {
        //获取函数名
        String funcName = ctx.IDENT().getText();
        //获取函数
        IRFunctionBlockRef function = IRGetNamedFunction(module, funcName);
        if (function == null) {
            //如果函数不存在，从符号表中查找
            function = (IRFunctionBlockRef) currentScope.findVarInAllScope(funcName);
        }
        //获取函数参数个数
        int numParams = ((IRFunctionType) function.getType()).getParamsCount();
        //如果时starttime和stoptime函数，需要获取当前代码的行号,并且变成对sysy函数的调用
        if (funcName.equals("starttime") || funcName.equals("stoptime")) {
            funcName = "_sysy_"+funcName;
            IRValueRef line = new IRConstIntRef(ctx.getStart().getLine(), i32Type);
            List<IRValueRef> args = new ArrayList<>();
            args.add(line);
            function = (IRFunctionBlockRef) currentScope.findVarInAllScope(funcName);
            return IRBuildCall(builder, function, args, 1, funcName);
        }
        //调用函数
        List<IRValueRef> args = new ArrayList<>(numParams);
        for (int i = 0; i < numParams; i++) {
            IRValueRef param = ctx.funcRParams().param(i).accept(this);
            IRType paramType = ((IRFunctionType) function.getType()).getParamType(i);
            // todo:没有处理float
            if (paramType instanceof IRPointerType) {
                if ((!(((IRPointerType) paramType).getBaseType() instanceof IRArrayType)) && param.getType() instanceof IRPointerType && ((IRPointerType) param.getType()).getBaseType() instanceof IRArrayType) {
                    List<IRValueRef> indexes = new ArrayList<>();
                    indexes.add(zero);
                    indexes.add(zero);
                    param = IRBuildGEP(builder, param, indexes, indexes.size(), "new_ptr");
                } else if (!Objects.equals(paramType.getText(), param.getType().getText())) {
                    List<IRValueRef> indexes = new ArrayList<>();
                    indexes.add(zero);
                    indexes.add(zero);
                    param = IRBuildGEP(builder, param, indexes, indexes.size(), "new_ptr");
                }
            }

            if (!paramType.equals(param.getType())) {
                if (paramType.equals(i32Type)) {
                    param = typeTransfer(builder, param, IRConst.FloatToInt);
                } else if (paramType.equals(floatType)) {
                    param = typeTransfer(builder, param, IRConst.IntToFloat);
                }
            }
            args.add(i, param);
        }
        return IRBuildCall(builder, function, args, numParams, funcName);
    }

    @Override
    public IRValueRef visitExp_with_lval(SysYParser.Exp_with_lvalContext ctx) {
        String variableName = ctx.lVal().IDENT().getText();
        if (variableName.length() > 20) {
            variableName = variableName.substring(0, 20);
        }
        IRValueRef variable = currentScope.findVarInAllScope(variableName);
        if (ConstIntVarMap.get(variable.getText()) != null) {
            return new IRConstIntRef(ConstIntVarMap.get(variable.getText()), i32Type);
        }
        if (ConstFloatVarMap.get(variable.getText()) != null) {
            return new IRConstFloatRef(ConstFloatVarMap.get(variable.getText()));
        }
        IRValueRef lValPointer = visitLVal(ctx.lVal());
        if (arrayAddr) {
            arrayAddr = false;
            return lValPointer;
        }
        return IRBuildLoad(builder, lValPointer, variableName);
    }

    @Override
    public IRValueRef visitLVal(SysYParser.LValContext ctx) {
        String lValName = ctx.IDENT().getText();
        if (lValName.length() > 20) {
            lValName = lValName.substring(0, 20);
        }
        IRValueRef lValPointer = currentScope.findVarInAllScope(lValName);
        IRType lvalType = lValPointer.getType() instanceof IRPointerType ? ((IRPointerType) lValPointer.getType()).getBaseType() : lValPointer.getType();//对应取出如果是指针类型，需要取出指针指向的类型
        lvalType = lvalType instanceof IRFunctionType ? ((IRFunctionType) lvalType).getReturnType() : lvalType;
        if (lvalType == i32Type || lvalType == floatType) {
            return lValPointer;
        }
        if (lvalType.getText().equals(new IRPointerType(i32Type).getText()) || lvalType.getText().equals(new IRPointerType(floatType).getText())) {
            if (ctx.exp().isEmpty()) {
                return lValPointer;
            } else {
                if (ctx.exp().size() == 1) {
                    lValPointer = getIrValueRef(ctx, lValName, lValPointer);
                }
            }
        } else if (lvalType instanceof IRPointerType && (((IRPointerType) lvalType).getBaseType() instanceof IRArrayType || ((IRPointerType) lvalType).getBaseType() instanceof IRPointerType)) {
            if (ctx.exp().isEmpty()) {
                return lValPointer;
            } else {
                lValPointer = getIrValueRef(ctx, lValName, lValPointer);
                for (int i = 1; i < ctx.exp().size(); i++) {
                    List<IRValueRef> interIndexes = new ArrayList<>();
                    IRValueRef interIndex = ctx.exp(i).accept(this);
                    interIndexes.add(zero);
                    interIndexes.add(interIndex);
                    lValPointer = IRBuildGEP(builder, lValPointer, interIndexes, interIndexes.size(), lValName);
                }
            }
        } else {
            if (ctx.exp().isEmpty()) {
                List<IRValueRef> indexes = new ArrayList<>();
                indexes.add(zero);
                indexes.add(zero);
                lValPointer = IRBuildGEP(builder, lValPointer, indexes, indexes.size(), lValName);

                arrayAddr = true;
                return lValPointer;
            }

            for (SysYParser.ExpContext expContext : ctx.exp()) {
                List<IRValueRef> indexes = new ArrayList<>();
                if (lvalType instanceof IRArrayType) {
                    indexes.add(zero);
                }
                IRValueRef index = expContext.accept(this);
                indexes.add(index);
                lValPointer = IRBuildGEP(builder, lValPointer, indexes, indexes.size(), lValName);
            }
        }
        if (lValPointer.getType() instanceof IRArrayType || (lValPointer.getType() instanceof IRPointerType) && (((IRPointerType) lValPointer.getType()).getBaseType() instanceof IRArrayType)) {
            arrayAddr = true;
        }

        return lValPointer;
    }

    private IRValueRef getIrValueRef(SysYParser.LValContext ctx, String lValName, IRValueRef lValPointer) {
        List<IRValueRef> indexes = new ArrayList<>();
        IRValueRef index = ctx.exp(0).accept(this);
        indexes.add(index);
        IRValueRef pointer = IRBuildLoad(builder, lValPointer, lValName);
        lValPointer = IRBuildGEP(builder, pointer, indexes, indexes.size(), lValName);
        return lValPointer;
    }

    @Override
    public IRValueRef visitExp_with_plus_and_mius(SysYParser.Exp_with_plus_and_miusContext ctx) {
        //生成返回值，包括了浮点数和整数，会根据类型生成不同的IR
        if (ctx.getChild(1).getText().equals("+")) {
            return IRBuildAdd(builder, visit(ctx.exp(0)), visit(ctx.exp(1)), "add");
        } else {
            return IRBuildSub(builder, visit(ctx.exp(0)), visit(ctx.exp(1)), "sub");
        }
    }

    @Override
    public IRValueRef visitExp_with_symbol(SysYParser.Exp_with_symbolContext ctx) {
        //生成返回值，包括了浮点数和整数，会根据类型生成不同的IR，并且会进行隐式转换
        if (ctx.getChild(1).getText().equals("*")) {
            return IRBuildMul(builder, visit(ctx.exp(0)), visit(ctx.exp(1)), "mul");
        } else if (ctx.getChild(1).getText().equals("/")) {
            return IRBuildDiv(builder, visit(ctx.exp(0)), visit(ctx.exp(1)), "div");
        } else {
            return IRBuildRem(builder, visit(ctx.exp(0)), visit(ctx.exp(1)), "rem");
        }
    }

    @Override
    public IRValueRef visitExp_with_unaryOp(SysYParser.Exp_with_unaryOpContext ctx) {
        //生成返回值
        IRValueRef result = visit(ctx.exp());
        if (ctx.unaryOp().getText().equals("-")) {
            return IRBuildNeg(builder, result, "neg");
        } else if (ctx.unaryOp().getText().equals("!")) {
            //如果是0，返回1，否则返回0
            //符号扩展
            result = IRBuildICmp(builder, 1, new IRConstIntRef(0, i32Type), result, "icmp");
            result = IRBuildXor(builder, result, new IRConstIntRef(1, i1Type), "xor");
            result = IRBuildZExt(builder, result, i32Type, "zext");
            return result;
        } else {
            return result;
        }
    }

    @Override
    public IRValueRef visitExp_with_Paren(SysYParser.Exp_with_ParenContext ctx) {
        return visit(ctx.exp());
    }

    //两个def和initval都无法确定正确性，主要于数组有关
    // 处理数组的赋值
    List<IRValueRef> initArrayList = new ArrayList<>(); // 存数组的初始值
    List<Integer> elementDimension; // 数组每层的维度 a[4][6][3] {4, 6, 3}
    Integer curDim; // 当前位于的层数 curDim = 0/1/2

    @Override
    public IRValueRef visitVarDef(SysYParser.VarDefContext ctx) {
        //获取变量名，类型
        String varName = ctx.IDENT().getText();
        if (varName.length() > 20) {
            varName = varName.substring(0, 20);
        }
        //生成变量类型
        IRType varType = judgeType(ctx.getParent().getChild(0).getText());
        IRValueRef var;
        IRValueRef initVal;
        if (varType instanceof IRInt32Type) {
            initVal = zero;
        } else {
            initVal = floatZero;
        }
        //判断是不是数组，求type，反向加载数组，迭代求解type
        List<Integer> bracketCount = new ArrayList<>();
        for (SysYParser.ConstExpContext constExpContext : ctx.constExp()) {
            //获取数组的维度
            IRValueRef temp = this.visit(constExpContext.exp());
            bracketCount.add(Integer.parseInt(temp.getText()));
        }
        for (int i = bracketCount.size() - 1; i >= 0; i--) {
            varType = new IRArrayType(varType, bracketCount.get(i));
        }
        if (varType instanceof IRArrayType) {
            elementDimension = ((IRArrayType) varType).getLengthList();
        }
        initArrayList.clear();
        curDim = 0;
        //生成变量
        //如果是全局变量
        //创建名为globalVar的全局变量
        if (currentScope.getFatherScope() == null) {
            var = IRAddGlobal(module, varType, /*globalVarName:String*/varName);
            //为全局变量设置初始化器
            if (ctx.ASSIGN() != null) {
                //如果有初始化值,通过对visit(ctx.initVal())的调用，获取初始化值并存储在全局的initArrayList中
                IRValueRef value = visit(ctx.initVal());
                if (varType instanceof IRArrayType) {
                    //如果是数组，需要将数组的值存入数组中,将value变成List<IRValueRef>
                    IRSetInitializer(module, (IRGlobalRegRef) var, initArrayList);
                } else {
                    //如果不是数组，判断是否需要类型转换
                    if (value.getType() != varType) {
                        if (value.getType() == floatType) {
                            value = typeTransfer(builder, value, IRConst.FloatToInt);
                        } else if (value.getType() == i32Type) {
                            value = typeTransfer(builder, value, IRConst.IntToFloat);
                        }
                    }
                    IRSetInitializer(module, (IRGlobalRegRef) var, value);
                }
            } else {
                //如果没有初始化值
                if (varType instanceof IRArrayType) {
                    //如果是数组，需要将数组的值存入数组中
                    IRSetInitializer(module, (IRGlobalRegRef) var, initArrayList);
                } else {
                    IRSetInitializer(module, (IRGlobalRegRef) var, /* constantVal:IRValueRef*/initVal);
                }
            }
        } else {
            //如果是局部变量,创建局部变量
            var = IRBuildAlloca(builder, varType, varName);
            if (bracketCount.isEmpty()) {
                if (ctx.ASSIGN() != null) {
                    IRValueRef value = visit(ctx.initVal());
                    //如果不是数组，判断是否需要类型转换
                    if (value.getType() != varType) {
                        if (value.getType() == floatType) {
                            value = typeTransfer(builder, value, IRConst.FloatToInt);
                        } else if (value.getType() == i32Type) {
                            value = typeTransfer(builder, value, IRConst.IntToFloat);
                        }
                    }
                    IRBuildStore(builder, value, var);
                } else {
                    IRBuildStore(builder, initVal, var);
                }
            } else {
                //如果是局部变量中数组的定义
                if (ctx.ASSIGN() != null) {
                    if (!ctx.initVal().isEmpty()) {
                        ((IRArrayType)((IRPointerType)var.getType()).getBaseType()).setNeedInit(true);
                        //如果存在初始化值，通过visit(ctx.initVal())获取初始化值
                        if (ctx.ASSIGN() != null) {
                            visitInitVal(ctx.initVal());
                        }
                        arrayDef(var);
                    }
                }
            }
        }
        //将变量加入符号表
        currentScope.defineVar(varName, var);
        return null;
    }

    private void arrayDef(IRValueRef var) {
        List<IRValueRef> arrayPointer = new ArrayList<>(elementDimension.size());//存储数组的指针
        for (int i = 0; i < initArrayList.size(); i++) {
            int totalCount = initArrayList.size();//总元素个数
            int counter = i;//当前元素的下标
            if (!Objects.equals(initArrayList.get(i).getText(), "0")) {//如果当前初始化值不为0
                arrayPointer.add(zero);
                for (Integer integer : elementDimension) {//遍历数组的维度
                    totalCount /= integer;//计算当前维度的元素个数
                    arrayPointer.add(new IRConstIntRef(counter / totalCount, i32Type));
                    counter -= (counter / totalCount) * totalCount;
                }
                int counter1 = 1;
                List<IRValueRef> paramList = new ArrayList<>();
                paramList.add(zero);
                paramList.add(arrayPointer.get(counter1++));
                IRValueRef elementPtr = IRBuildGEP(builder, var, paramList, 2, "array");
                for (int j = 0; j < elementDimension.size() - 1; j++) {
                    paramList.clear();
                    paramList.add(zero);
                    paramList.add(arrayPointer.get(counter1++));
                    elementPtr = IRBuildGEP(builder, elementPtr, paramList, 2, "array");
                }

                IRBuildStore(builder, initArrayList.get(i), elementPtr);
                arrayPointer.clear();
            }
        }
    }

    @Override
    public IRValueRef visitConstDef(SysYParser.ConstDefContext ctx) {
        //获取变量名，类型
        String varName = ctx.IDENT().getText();
        if (varName.length() > 20) {
            varName = varName.substring(0, 20);
        }
        //生成变量类型
        IRType varType = judgeType(ctx.getParent().getChild(1).getText());
        IRValueRef var;
        IRValueRef initVal;
        if (varType instanceof IRInt32Type) {
            initVal = zero;
        } else {
            initVal = floatZero;
        }
        //判断是不是数组，求type，反向加载数组，迭代求解type
        List<Integer> bracketCount = new ArrayList<>();
        for (SysYParser.ConstExpContext constExpContext : ctx.constExp()) {
            //获取数组的维度
            IRValueRef temp = this.visit(constExpContext.exp());
            bracketCount.add(Integer.parseInt(temp.getText()));
        }
        for (int i = bracketCount.size() - 1; i >= 0; i--) {
            varType = new IRArrayType(varType, bracketCount.get(i));
        }
        if (varType instanceof IRArrayType) {
            elementDimension = ((IRArrayType) varType).getLengthList();
        }
        initArrayList.clear();
        curDim = 0;
        //生成变量
        //如果是全局变量
        //创建名为globalVar的全局变量
        if (currentScope.getFatherScope() == null) {
            var = IRAddGlobal(module, varType, /*globalVarName:String*/varName);
            //为全局变量设置初始化器
            if (ctx.ASSIGN() != null) {
                //如果有初始化值,通过对visit(ctx.initVal())的调用，获取初始化值并存储在全局的initArrayList中
                IRValueRef value = visit(ctx.constInitVal());
                if (varType instanceof IRArrayType) {
                    //如果是数组，需要将数组的值存入数组中,将value变成List<IRValueRef>
                    IRSetInitializer(module, (IRGlobalRegRef) var, initArrayList);
                } else {
                    //如果不是数组，判断是否需要类型转换
                    if (value.getType() != varType) {
                        if (value.getType() == floatType) {
                            int i = ((int) ((IRConstFloatRef) value).getValue());
                            value = new IRConstIntRef(i, i32Type);
                        } else if (value.getType() == i32Type) {
                            value = new IRConstFloatRef(Float.parseFloat(value.getText()));
                        }
                    }
                    if (value instanceof IRConstIntRef) {
                        ConstIntVarMap.put(var.getText(), Integer.parseInt(value.getText()));
                    }
                    if (value instanceof IRConstFloatRef) {
                        ConstFloatVarMap.put(var.getText(), ((IRConstFloatRef) value).getValue());
                    }
                    IRSetInitializer(module, (IRGlobalRegRef) var, value);
                }
            } else {
                //如果没有初始化值
                if (varType instanceof IRArrayType) {
                    //如果是数组，需要将数组的值存入数组中
                    IRSetInitializer(module, (IRGlobalRegRef) var, initArrayList);
                } else {
                    if (initVal instanceof IRConstIntRef) {
                        ConstIntVarMap.put(var.getText(), Integer.parseInt(initVal.getText()));
                    }
                    if (initVal instanceof IRConstFloatRef) {
                        ConstFloatVarMap.put(var.getText(), Float.parseFloat(initVal.getText()));
                    }
                    IRSetInitializer(module, (IRGlobalRegRef) var, /* constantVal:IRValueRef*/initVal);
                }
            }
        } else {
            //如果是局部变量,创建局部变量
            var = IRBuildAlloca(builder, varType, varName);
            if (bracketCount.isEmpty()) {
                if (ctx.ASSIGN() != null) {
                    IRValueRef value = visit(ctx.constInitVal());
                    //如果不是数组，判断是否需要类型转换
                    if (value.getType() != varType) {
                        if (value.getType() == floatType) {
                            value = typeTransfer(builder, value, IRConst.FloatToInt);
                        } else if (value.getType() == i32Type) {
                            value = typeTransfer(builder, value, IRConst.IntToFloat);
                        }
                    }
                    IRBuildStore(builder, value, var);
                    if (value instanceof IRConstIntRef) {
                        ConstIntVarMap.put(var.getText(), Integer.parseInt(value.getText()));
                    }
                    if (value instanceof IRConstFloatRef) {
                        ConstFloatVarMap.put(var.getText(),((IRConstFloatRef) value).getValue());
                    }
                } else {
                    IRBuildStore(builder, initVal, var);
                    if (initVal instanceof IRConstIntRef) {
                        ConstIntVarMap.put(var.getText(), Integer.parseInt(initVal.getText()));
                    }
                    if (initVal instanceof IRConstFloatRef) {
                        ConstFloatVarMap.put(var.getText(), Float.parseFloat(initVal.getText()));
                    }
                }

            } else {
                //如果是局部变量中数组的定义
                // 正确性验证
                if (ctx.ASSIGN() != null) {
                    if (!ctx.constInitVal().isEmpty()) {
                        ((IRArrayType)((IRPointerType)var.getType()).getBaseType()).setNeedInit(true);
                        if (ctx.ASSIGN() != null) visitConstInitVal(ctx.constInitVal());
                        arrayDef(var);
                    }
                }
            }
        }
        //将变量加入符号表
        currentScope.defineVar(varName, var);
        return null;
    }

    @Override
    public IRValueRef visitConstInitVal(SysYParser.ConstInitValContext ctx) {
        if (ctx.constExp() != null) {
            return ctx.constExp().exp().accept(this);
        }
        // 处理initVal嵌套的情况
        int layerDim = curDim;
        boolean dump = false;
        int count = 0; // 本层已有的数目
        int fullCount = 0; // 需要进到上一层的数目
        for (SysYParser.ConstInitValContext initValContext : ctx.constInitVal()) {
            if (initValContext.constExp() != null) {
                // 没有{}的处理
                if (!dump) {
                    dump = true;
                    curDim = elementDimension.size() - 1;
                    count = 0;
                    fullCount = elementDimension.get(curDim);
                }
                initArrayList.add(initValContext.constExp().exp().accept(this));
            } else {
                int tmpDim = curDim;
                curDim = tmpDim + 1;
                visitConstInitVal(initValContext);
                curDim = tmpDim;
            }
            count++;
            if (count == fullCount) {
                dump = false;
                curDim--;
                if (curDim <= layerDim) curDim = layerDim;
                fullCount = elementDimension.get(curDim);
                int layerParamCount = 1;
                for (int i = curDim + 1; i < elementDimension.size(); i++) layerParamCount *= elementDimension.get(i);
                count = initArrayList.size() / layerParamCount;
            }
        }
        int totalParamCount = 1;
        for (int i = layerDim; i < elementDimension.size(); i++) {
            totalParamCount *= elementDimension.get(i);
        }
        boolean empty = (ctx.constInitVal().isEmpty());
        if (initArrayList.size() % totalParamCount != 0 || empty) {
            for (int i = initArrayList.size() % totalParamCount; i < totalParamCount; i++) initArrayList.add(zero);
        }
        return null;
    }

    @Override
    public IRValueRef visitInitVal(SysYParser.InitValContext ctx) {
        // 单独处理exp()
        if (ctx.exp() != null) {
            return ctx.exp().accept(this);
        }
        // 处理initVal嵌套的情况
        int layerDim = curDim;
        boolean dump = false;
        int count = 0; // 本层已有的数目
        int fullCount = 0; // 需要进到上一层的数目
        for (SysYParser.InitValContext initValContext : ctx.initVal()) {
            if (initValContext.exp() != null) {
                // 没有{}的处理
                if (!dump) {
                    dump = true;
                    curDim = elementDimension.size() - 1;
                    count = 0;
                    fullCount = elementDimension.get(curDim);
                }
                initArrayList.add(initValContext.exp().accept(this));
            } else {
                int tmpDim = curDim;
                curDim = tmpDim + 1;
                visitInitVal(initValContext);
                curDim = tmpDim;
            }
            count++;
            if (count == fullCount) {
                dump = false;
                curDim--;
                if (curDim <= layerDim) curDim = layerDim;
                fullCount = elementDimension.get(curDim);
                int layerParamCount = 1;
                for (int i = curDim + 1; i < elementDimension.size(); i++) layerParamCount *= elementDimension.get(i);
                count = initArrayList.size() / layerParamCount;
            }
        }
        int totalParamCount = 1;
        for (int i = layerDim; i < elementDimension.size(); i++) {
            totalParamCount *= elementDimension.get(i);
        }
        boolean empty = (ctx.initVal().isEmpty());
        if (initArrayList.size() % totalParamCount != 0 || empty) {
            for (int i = initArrayList.size() % totalParamCount; i < totalParamCount; i++) initArrayList.add(zero);
        }
        return null;
    }

    @Override
    public IRValueRef visitNumber(SysYParser.NumberContext ctx) {
        String num = ctx.getText();
        if (ctx.INTEGER_CONST() != null) {
            //如果是整数,进行进制转换
            return calculateInt(num);
        } else if (ctx.FLOAT_CONST() != null) {
            //如果是浮点数,进行进制转换
            return calculateFloat(num);
        }
        return new IRConstIntRef(0, i32Type);
    }

    @Override
    public IRValueRef visitCond(SysYParser.CondContext ctx) {
        //生成返回值
        //实现短路求值，在and和or中实现
        IRValueRef condition;
        if (ctx.getChildCount() == 1) {
            condition = visit(ctx.getChild(0));
        } else if (ctx.getChild(1).getText().equals("==")) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IREQ, visit(ctx.getChild(0)), visit(ctx.getChild(2)), "tmp_");
        } else if (ctx.getChild(1).getText().equals("!=")) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, visit(ctx.getChild(0)), visit(ctx.getChild(2)), "tmp_");
        } else if (ctx.getChild(1).getText().equals(">")) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRSGT, visit(ctx.getChild(0)), visit(ctx.getChild(2)), "tmp_");
        } else if (ctx.getChild(1).getText().equals(">=")) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRSGE, visit(ctx.getChild(0)), visit(ctx.getChild(2)), "tmp_");
        } else if (ctx.getChild(1).getText().equals("<")) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRSLT, visit(ctx.getChild(0)), visit(ctx.getChild(2)), "tmp_");
        } else if (ctx.getChild(1).getText().equals("<=")) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRSLE, visit(ctx.getChild(0)), visit(ctx.getChild(2)), "tmp_");
        } else if (ctx.getChild(1).getText().equals("&&")) {
            IRValueRef var = IRBuildAlloca(builder, i32Type, "and_build");
            IRValueRef cond1 = visit(ctx.getChild(0));
            cond1 = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, cond1, zero, "tmp_");
            IRBaseBlockRef ifTrue = IRAppendBasicBlock(currentFunction, "andTrue");
            IRBaseBlockRef ifFalse = IRAppendBasicBlock(currentFunction, "andFalse");
            IRBaseBlockRef entry = IRAppendBasicBlock(currentFunction, "andCondition");
            if (cond1 instanceof IRConstFloatRef) {
                //cond1 = typeTransfer(builder,cond1,IRConst.FloatToInt);
                cond1 = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, cond1, zero, "tmp_");
            }
            IRBuildCondBr(builder,
                    /*condition:IRValueRef*/ cond1,
                    ifTrue,
                    /*ifFalse:IRBasicBlockRef*/ ifFalse);
            //生成ifTrue的指令，如果cond1不等于0，判断第二个条件
            IRPositionBuilderAtEnd(builder, ifTrue);
            currentBlock = ifTrue;
            currentBlockName = "ifTrue";
            IRValueRef variable = visit(ctx.getChild(2));
            if (variable instanceof IRConstFloatRef) {
                variable = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, variable, zero, "tmp_");
                variable = IRBuildZExt(builder, variable, i32Type, "tmp_");
            }
            IRBuildStore(builder, variable, var);
            IRBuildBr(builder, entry);
            //生成ifFalse的指令，如果cond1等于0，直接返回0
            IRPositionBuilderAtEnd(builder, ifFalse);
            currentBlock = ifFalse;
            currentBlockName = "ifFalse";
            IRBuildStore(builder, zero, var);
            IRBuildBr(builder, entry);
            //生成ifFalse之后的指令
            IRPositionBuilderAtEnd(builder, entry);
            currentBlock = entry;
            currentBlockName = "entry";
            return IRBuildLoad(builder, var, "and_load");
        } else if (ctx.getChild(1).getText().equals("||")) {
            //如果第一个条件为大于0的数，直接返回1
            IRValueRef var = IRBuildAlloca(builder, i32Type, "or_build");
            IRValueRef cond1 = visit(ctx.getChild(0));
            cond1 = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, cond1, zero, "tmp_");
            IRBaseBlockRef ifTrue = IRAppendBasicBlock(currentFunction, "orTrue");
            IRBaseBlockRef ifFalse = IRAppendBasicBlock(currentFunction, "orFalse");
            IRBaseBlockRef entry = IRAppendBasicBlock(currentFunction, "orCondition");
            if (cond1 instanceof IRConstFloatRef) {
                //cond1 = typeTransfer(builder,cond1,IRConst.FloatToInt);
                cond1 = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, cond1, zero, "tmp_");
            }
            IRBuildCondBr(builder,
                    /*condition:IRValueRef*/ cond1,
                    ifTrue,
                    /*ifFalse:IRBasicBlockRef*/ ifFalse);
            //生成ifTrue的指令,如果cond1不等于0，直接返回1
            IRPositionBuilderAtEnd(builder, ifTrue);
            currentBlock = ifTrue;
            currentBlockName = "ifTrue";
            IRBuildStore(builder, new IRConstIntRef(1, i32Type), var);
            IRBuildBr(builder, entry);
            //生成ifFalse的指令，如果cond1等于0，判断第二个条件
            IRPositionBuilderAtEnd(builder, ifFalse);
            currentBlock = ifFalse;
            currentBlockName = "ifFalse";
            IRValueRef variable = visit(ctx.getChild(2));
            if (variable instanceof IRConstFloatRef) {
                variable = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, variable, zero, "tmp_");
                variable = IRBuildZExt(builder, variable, i32Type, "tmp_");
            }
            IRBuildStore(builder, variable, var);
            IRBuildBr(builder, entry);
            //生成ifFalse之后的指令
            IRPositionBuilderAtEnd(builder, entry);
            currentBlock = entry;
            currentBlockName = "entry";
            return IRBuildLoad(builder, var, "or_load");
        } else {
            condition = visit(ctx.getChild(0));
        }
        //将int1转换为int32
        if (condition.getType().equals(i1Type)) {
            condition = IRBuildZExt(builder, condition, i32Type, "tmp_");
        }
        //将float转换为int32
        return condition;
    }

    @Override
    public IRValueRef visitStmt_with_if(SysYParser.Stmt_with_ifContext ctx) {
        //跳转指令决定跳转到哪个块
        //生成条件
        IRValueRef condition = visit(ctx.cond());
        //将int32转换为int1
        if (condition.getType().equals(i32Type)) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, zero, condition, "tmp_");
        }
        IRBaseBlockRef ifTrue = IRAppendBasicBlock(currentFunction, "true");
        //生成ifFalse
        IRBaseBlockRef ifFalse = IRAppendBasicBlock(currentFunction, "false");
        //生成ifFalse之后的指令
        IRBaseBlockRef entry = IRAppendBasicBlock(currentFunction, "entry");
        if (condition instanceof IRConstFloatRef) {
            //condition = typeTransfer(builder,condition,IRConst.FloatToInt);
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, condition, zero, "tmp_");
        }
        IRBuildCondBr(builder,
                /*condition:IRValueRef*/ condition,
                /*ifTrue:IRBasicBlockRef*/ ifTrue,
                /*ifFalse:IRBasicBlockRef*/ ifFalse);
        //生成ifTrue的指令
        IRPositionBuilderAtEnd(builder, ifTrue);
        currentBlock = ifTrue;
        currentBlockName = "ifTrue";
        visit(ctx.stmt(0));
        //访问完ifTrue后，需要跳转到ifFalse后面的指令
        IRBuildBr(builder, entry);
        //生成ifFalse的指令
        IRPositionBuilderAtEnd(builder, ifFalse);
        currentBlock = ifFalse;
        currentBlockName = "ifFalse";
        if (ctx.stmt().size() == 2) {
            visit(ctx.stmt(1));
        }
        //访问完ifFalse后，需要跳转到ifFalse后面的指令
        IRBuildBr(builder, entry);
        //切换作用域
        //生成ifFalse之后的指令
        IRPositionBuilderAtEnd(builder, entry);
        currentBlock = entry;
        currentBlockName = "entry";
        return null;
    }

    @Override
    public IRValueRef visitStmt_with_while(SysYParser.Stmt_with_whileContext ctx) {
        //生成条件
        //从上一个基本块跳转到whileCond
        IRBaseBlockRef whileCond = IRAppendBasicBlock(currentFunction, "whileCondition");
        IRBuildBr(builder, whileCond);
        IRPositionBuilderAtEnd(builder, whileCond);
        currentBlock = whileCond;
        currentBlockName = "whileCondition";
        IRValueRef condition = visit(ctx.cond());
        //将int32转换为int1
        if (condition.getType().equals(i32Type)) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, zero, condition, "tmp_");
        }
        //生成whileTrue
        IRBaseBlockRef whileBody = IRAppendBasicBlock(currentFunction, "whileBody");
        //生成entry,即while循环结束后的指令
        IRBaseBlockRef entry = IRAppendBasicBlock(currentFunction, "entry");
        if (condition instanceof IRConstFloatRef) {
            condition = IRBuildICmp(builder, /*这是个int型常量，表示比较的方式*/IRConst.IRNE, condition, zero, "tmp_");
        }
        IRBuildCondBr(builder,
                /*condition:IRValueRef*/ condition,
                /*ifTrue:IRBasicBlockRef*/ whileBody,
                /*ifFalse:IRBasicBlockRef*/ entry);
        //生成whileBody
        IRPositionBuilderAtEnd(builder, whileBody);
        //将entry连接到whileBody
        currentBlock = whileBody;
        currentBlock.getSuccList().add(entry);
        currentBlockName = "whileBody";
        visit(ctx.stmt());
        //要在whileBody的最后加上循环指令
        IRBuildBr(builder, whileCond);
        //生成entry
        IRPositionBuilderAtEnd(builder, entry);
        currentBlock = entry;
        return null;
    }

    @Override
    public IRValueRef visitStmt_with_break(SysYParser.Stmt_with_breakContext ctx) {
        //找到while的基本块
        IRBaseBlockRef whileBlock = currentScope.findWhileBlock();
        if (whileBlock == null) {
            currentScope = new Scope("whileBody", currentScope, currentBlock);
            whileBlock = currentScope.findWhileBlock();
            //找到while基本快的entry
            IRBaseBlockRef entry = IRGetNextBasicBlock(whileBlock);
            //跳转到while的基本块
            IRBuildBr(builder, entry);
            currentScope = currentScope.getFatherScope();
        } else {
            //找到while基本快的entry
            IRBaseBlockRef entry = IRGetNextBasicBlock(whileBlock);
            //跳转到while的基本块
            IRBuildBr(builder, entry);
        }
        return null;

    }

    @Override
    public IRValueRef visitStmt_with_continue(SysYParser.Stmt_with_continueContext ctx) {
        //找到while的基本块
        IRBaseBlockRef whileBlock = currentScope.findWhileBlock();
        //跳转到while的基本块的条件，即whileCond
        IRBaseBlockRef whileCond = IRGetPreviousBasicBlock(whileBlock);
        IRBuildBr(builder, whileCond);
        return null;
    }

    //辅助函数的部分
    public IRValueRef calculateInt(String number) {
        int num;
        if (number.startsWith("0x") || number.startsWith("0X")) {
            num = Integer.parseInt(number.substring(2), 16);
        } else if (number.startsWith("0") && number.length() != 1) {
            num = Integer.parseInt(number.substring(1), 8);
        } else {
            num = Integer.parseInt(number);
        }
        return new IRConstIntRef(num, i32Type);
    }

    public IRValueRef calculateFloat(String number) {
        float num = (float) Double.parseDouble(number);
        return new IRConstFloatRef(num);
    }

    private IRType judgeType(String typeName) {
        if (typeName.contains("int")) return i32Type;
        else if (typeName.contains("float")) return floatType;
        return voidType;
    }

    public IRModule getModule() {
        return module;
    }
}
