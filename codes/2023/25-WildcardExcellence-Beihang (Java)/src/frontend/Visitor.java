package frontend;

import midend.MyModule;
import midend.Type;
import midend.irTable.IrTable;
import midend.irTable.IrTableItem;
import midend.value.*;
import midend.value.global.GlobalVariable;
import midend.value.instrs.*;
import org.antlr.v4.runtime.tree.ParseTree;

import java.util.ArrayList;
import java.util.Stack;

import static midend.value.Value.BLOCK_NUM;

public class Visitor extends SysYBaseVisitor<Value> {
    private Type varType;
    private boolean isGlobal = true;
    private BasicBlock curBB = null;
    private Function curFunc = null;
    private boolean isForce = false;
    private final Stack<IrTable> tableStack = new Stack<>();
    private final Stack<BasicBlock> whileStart = new Stack<>();
    private final Stack<BasicBlock> whileEnd = new Stack<>();

    @Override
    public Value visitCompUnit(SysYParser.CompUnitContext ctx) {
        isGlobal = true;
        tableStack.push(new IrTable(null));
        return super.visitCompUnit(ctx);
    }

    @Override
    public Value visitFuncDef(SysYParser.FuncDefContext ctx) {
        isGlobal = false;
        Type type = ctx.funcType().getText().equals("int") ? Type.IntegerType.I32 :
                ctx.funcType().getText().equals("float") ? new Type.FloatType() : new Type.VoidType();
        String name = ctx.IDENT().getText();
        ArrayList<Value> arguments = new ArrayList<>();
        Function function = new Function(null, "", arguments);
        function.setType(type);
        function.setName(name);
        curFunc = function;
        MyModule.myModule.addFunction(function);
        curBB = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
        tableStack.peek().addItem(new IrTableItem(function.getName(), function.getType(), false, function, null));
        tableStack.push(new IrTable(tableStack.peek()));
        arguments = getArgs(ctx.funcFParams());
        function.setArgument(arguments);
        visitBlock(ctx.block());
        tableStack.pop();
        isGlobal = true;
        return null;
    }

    @Override
    public Value visitBlock(SysYParser.BlockContext ctx) {
        ctx.blockItem().forEach(this::visitBlockItem);
        return null;
    }

    @Override
    public Value visitBlockItem(SysYParser.BlockItemContext ctx) {
        visitChildren(ctx);
        return null;
    }

    @Override
    public Value visitExp(SysYParser.ExpContext ctx) {
        return visit(ctx.addExp());
    }

    @Override
    public Value visitStmt(SysYParser.StmtContext ctx) {
        if (ctx.getChild(0).getText().equals("return")) {
            //对exp进行visit，获取返回值value
            Value retValue = null; //return;
            if (!ctx.getChild(1).getText().equals(";")) {
                retValue = visitExp((SysYParser.ExpContext) ctx.getChild(1)); //visit exp;
            }
            return new Return(retValue, curBB);
        }
        else if (ctx.getChild(0) instanceof SysYParser.LValContext && ctx.getChild(1).getText().equals("=")) {
            Value pointer = visitLVal(ctx.lVal()); //最终的pointer
            Value value = visitExp(ctx.exp()); // lval = exp;
            new Store(value, pointer, curBB);
        }
        else if (ctx.getChild(0).getText().equals("if")) {
            return visitIf(ctx);
        }
        else if (ctx.getChild(0) instanceof SysYParser.BlockContext) {
            tableStack.push(new IrTable(tableStack.peek()));
            visitBlock(ctx.block());
            tableStack.pop();
        }
        else if (ctx.getChild(0).getText().equals("while")) {
            return visitWhile(ctx);
        }
        else if (ctx.getChild(0).getText().equals("break")) {
            return new Jump(whileEnd.peek(), curBB);
        }
        else if (ctx.getChild(0).getText().equals("continue")) {
            return new Jump(whileStart.peek(), curBB);
        }
        else {
            visitChildren(ctx);
        }
        return null;
    }

    public Value visitWhile(SysYParser.StmtContext ctx) {
        SysYParser.CondContext cond = (SysYParser.CondContext) ctx.getChild(2); // cond表达式
        SysYParser.StmtContext whileStmt = (SysYParser.StmtContext) ctx.getChild(4);
        BasicBlock startBlock = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
        new Jump(startBlock, curBB);
        BasicBlock whileBB = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
        BasicBlock followBB = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
        curBB = startBlock;
        getCond(cond, whileBB, followBB);
        curBB = whileBB;
        whileStart.push(startBlock);
        whileEnd.push(followBB);
        visitStmt(whileStmt);
        whileStart.pop();
        whileEnd.pop();
        new Jump(startBlock, curBB);
        curBB = followBB;
        return null;
    }

    public Value visitIf(SysYParser.StmtContext ctx) {
        SysYParser.StmtContext trueStmt = null;
        SysYParser.StmtContext falseStmt = null;
        SysYParser.CondContext cond = ctx.cond(); // cond表达式
        for (ParseTree child : ctx.children) {
            if (child instanceof SysYParser.StmtContext) {
                if (trueStmt != null) {
                    falseStmt = (SysYParser.StmtContext) child;
                }
                else {
                    trueStmt = (SysYParser.StmtContext) child;
                }
            }
        }
        BasicBlock trueBB = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
        BasicBlock falseBB = null;
        if (falseStmt != null) {
            falseBB = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
        }
        BasicBlock followBB = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
        if (falseStmt != null) {
            //if else
            getCond(cond, trueBB, falseBB);
            curBB = trueBB;
            visitStmt(trueStmt);
            new Jump(followBB, curBB);
            curBB = falseBB;
            visitStmt(falseStmt);
        }
        else {
            //if
            getCond(cond, trueBB, followBB);
            curBB = trueBB;
            assert trueStmt != null;
            visitStmt(trueStmt);
        }
        new Jump(followBB, curBB);
        curBB = followBB;
        return null;
    }

    public void getCond(ParseTree ctx, BasicBlock trueBlock, BasicBlock falseBlock) {
        if (ctx instanceof SysYParser.CondContext) {
            getCond(ctx.getChild(0), trueBlock, falseBlock);
        }
        else if (ctx instanceof SysYParser.LOrExpContext) {
            int size = ctx.getChildCount(); // if (a || b)
            BasicBlock tempFalse;
            for (int i = 0; i < size; i++) {
                if (i == 1) {
                    continue;
                }
                if (i == size - 1) {
                    //最后一个操作数
                    tempFalse = falseBlock;
                }
                else {
                    tempFalse = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
                }
                getCond(ctx.getChild(i), trueBlock, tempFalse);
                curBB = tempFalse;
            }
        }
        else if (ctx instanceof SysYParser.LAndExpContext) {
            int size = ctx.getChildCount();
            BasicBlock tempTrue;
            for (int i = 0; i < size; i++) {
                if (i == 1) {
                    continue;
                }
                if (i == size - 1) {
                    tempTrue = trueBlock;
                }
                else {
                    tempTrue = new BasicBlock(new Type.VoidType(), "Block" + BLOCK_NUM++, curFunc);
                }
                if (ctx.getChild(i) instanceof SysYParser.EqExpContext) {
                    Value subCond = visitEqExp((SysYParser.EqExpContext) ctx.getChild(i));
                    if (subCond.getType().toString().equals("i32")) {
                        subCond = new BinaryOp(true, subCond,
                                Op.Ne, new Constant.ConstantInteger(Type.IntegerType.I32, "0"), curBB);
                    }
                    else if (subCond.getType().isFloatType()) {
                        subCond = new BinaryOp(true, subCond, Op.Ne, new Constant.ConstantInteger(Type.IntegerType.I32, "0"), curBB);
                    }
                    new Branch(subCond, tempTrue, falseBlock, curBB);
                }
                else {
                    getCond(ctx.getChild(i), tempTrue, falseBlock);
                }
                curBB = tempTrue;
            }
        }
    }

    public ArrayList<Value> getRealArgs(SysYParser.FuncRParamsContext ctx) {
        ArrayList<Value> args = new ArrayList<>();
        if (ctx == null) {
            return args;
        }
        for (SysYParser.ExpContext token : ctx.exp()) {
            args.add(visit(token));
        }
        return args;
    }

    public ArrayList<Value> getArgs(SysYParser.FuncFParamsContext ctx) {
        ArrayList<Value> arguments = new ArrayList<>();
        if (ctx == null) {
            // 没有参数
            return arguments;
        }
        for (SysYParser.FuncFParamContext funcFParamContext : ctx.funcFParam()) {
            Value arg = visitFuncFParam(funcFParamContext);
            arguments.add(arg);
            Value pointer = new Alloc(varType, curBB);
            pointer.setInitialType(arg.getInitialType());
            new Store(arg, pointer, curBB);
            tableStack.peek().addItem(new IrTableItem(((Argument) arg).getOriginName(), varType, false, pointer, null));
        }
        return arguments;
    }

    @Override
    public Value visitFuncFParam(SysYParser.FuncFParamContext ctx) {
        String name = ctx.IDENT().getText();
        varType = ctx.bType().getText().equals("int") ? Type.IntegerType.I32 : new Type.FloatType();
        ;
        boolean flag = false;
        int size = 0;
        ArrayList<Integer> arrayList = new ArrayList<>();
        for (int i = 0; i < ctx.getChildCount(); i++) {
            if (ctx.getChild(i).getText().equals("[")) {
                flag = true;
            }
            else if (ctx.getChild(i) instanceof SysYParser.ExpContext) {
                Value value = visit(ctx.getChild(i));
                if (value instanceof Constant.ConstantInteger) {
                    size = Integer.parseInt(value.getName());
                    arrayList.add(size);
                }
                else {
                    System.out.println("you cannot get const!!!");
                }
            }
        }

        Type tmpType = varType;
        for (int i = arrayList.size() - 1; i >= 0; i--) {
            tmpType = new Type.ArrayType(tmpType, arrayList.get(i));
        }
        Type newType = varType;
        if (arrayList.size() > 0) {
            newType = new Type.ArrayType(varType, tmpType.getLength());
        }
        if (flag) {
            //至少有一个 [
            varType = new Type.PointerType(newType);
        }
        Value arg = new Argument(varType, name);
        arg.setInitialType(tmpType);
        return arg;
    }

    @Override
    public Value visitDecl(SysYParser.DeclContext ctx) {
        return visitChildren(ctx);
    }

    @Override
    public Value visitConstDecl(SysYParser.ConstDeclContext ctx) {
        Type type = new Type.VoidType();
        String typeName = ctx.getChild(1).getText();
        if (typeName.equals("int")) {
            type = Type.IntegerType.I32;
        }
        else if (typeName.equals("float")) {
            type = new Type.FloatType();
        }
        varType = type;
        ctx.constDef().forEach(this::visitConstDef);
        return null;
    }

    @Override
    public Value visitVarDecl(SysYParser.VarDeclContext ctx) {
        Type type = new Type.VoidType();
        String typeName = ctx.getChild(0).getText();
        if (typeName.equals("int")) {
            type = Type.IntegerType.I32;
        }
        else if (typeName.equals("float")) {
            type = new Type.FloatType();
        }
        varType = type;
        ctx.varDef().forEach(this::visitVarDef);
        return null;
    }

    public Value simplify(Value first, Value second, Op op) {
        if (first instanceof Constant.ConstantInteger && second instanceof Constant.ConstantInteger) {
            return new Constant.ConstantInteger(first.getType(), eval(first.getName(), second.getName(), op));
        }
        if (first instanceof Constant.ConstantFloat && second instanceof Constant.ConstantFloat) {
            return new Constant.ConstantFloat(first.getType(), evalFloat(((Constant.ConstantFloat) first).getConstantFloatValue(), ((Constant.ConstantFloat) second).getConstantFloatValue(), op));
        }
        if (first instanceof Constant.ConstantFloat && second instanceof Constant.ConstantInteger) {
            float newSecond = ((Constant.ConstantInteger) second).getConstantIntegerValue();
            return new Constant.ConstantFloat(first.getType(), evalFloat(((Constant.ConstantFloat) first).getConstantFloatValue(), newSecond, op));
        }
        if (first instanceof Constant.ConstantInteger && second instanceof Constant.ConstantFloat) {
            float newFirst = ((Constant.ConstantInteger) first).getConstantIntegerValue();
            return new Constant.ConstantFloat(second.getType(), evalFloat(newFirst, ((Constant.ConstantFloat) second).getConstantFloatValue(), op));
        }
        return new BinaryOp(false, first, op, second, curBB);
    }

    public float evalFloat(float first, float second, Op op) {
        switch (op) {
            case Add:
                return first + second;
            case Sub:
                return first - second;
            case Mul:
                return first * second;
            case Div:
                return first / second;
            case Mod:
                return first % second;
            default:
                break;
        }
        return 0;
    }


    @Override
    public Value visitAddExp(SysYParser.AddExpContext ctx) {
        int size = ctx.getChildCount();
        Value res = visit(ctx.getChild(0));
        for (int i = 1; i < size; i = i + 2) {
            Op temp = ctx.getChild(i).getText().equals("+") ? Op.Add : Op.Sub;
            Value first = res;
            Value second = visit(ctx.getChild(i + 1));
            res = simplify(first, second, temp);
        }
        return res;

    }

    @Override
    public Value visitConstExp(SysYParser.ConstExpContext ctx) {
        return visitChildren(ctx);
    }

    @Override
    public Value visitEqExp(SysYParser.EqExpContext ctx) {
        int size = ctx.getChildCount();
        Value first = visit(ctx.getChild(0));
        Value second;
        if (size == 1) {
            return first;
        }
        else {
            String sym = ctx.getChild(1).getText();
            Op temp = sym.equals("==") ? Op.Eq : Op.Ne;
            second = visit(ctx.getChild(2));
            return new BinaryOp(true, first, temp, second, curBB);
        }
    }

    @Override
    public Value visitMulExp(SysYParser.MulExpContext ctx) {
        int size = ctx.getChildCount();
        if (size == 1) {
            return visit(ctx.getChild(0));
        }
        else {
            String sym = ctx.getChild(1).getText();
            Op temp = sym.equals("*") ? Op.Mul
                    : sym.equals("/") ? Op.Div : Op.Mod;
            Value first = visit(ctx.getChild(0));
            Value second = visit(ctx.getChild(2));
            return simplify(first, second, temp);
        }
    }

    @Override
    public Value visitPrimaryExp(SysYParser.PrimaryExpContext ctx) {
        if (ctx.getChildCount() == 1) {
            if (ctx.getChild(0) instanceof SysYParser.LValContext) {
                Value pointer = visit(ctx.getChild(0));
                if (!isForce) {
                    return new Load(pointer, curBB);
                }
                isForce = false;
                return pointer;
            }
            return visit(ctx.getChild(0));
        }
        else if (ctx.getChildCount() == 3) {
            return visit(ctx.getChild(1));
        }
        return null;
    }

    public Boolean isNeg(String s) {
        if (s.length() == 10) {
            int first4Bits = Integer.parseInt(s.charAt(2) + "", 16);
            return (first4Bits >> 3) != 0;
        }
        return false;
    }

    public String parseNegHInt(String s) {
        int first4Bits = Integer.parseInt(s.charAt(2) + "", 16);
        first4Bits = first4Bits & 7;
        int last28Bits = Integer.parseInt(s.substring(3), 16);
        int res = (first4Bits << 28) + last28Bits - 2147483647 - 1;
        return String.valueOf(res);
    }

    @Override
    public Value visitNumber(SysYParser.NumberContext ctx) {
        if (ctx.INTCONST() != null) {
            // 整型常量
            String s = ctx.INTCONST().getText();
            if (s.length() >= 2 && s.charAt(0) == '0' && (s.charAt(1) == 'x' || s.charAt(1) == 'X')) {
                // 十六进制数
                // 负数，最高位为 1
                if (isNeg(s)) {
                    return new Constant.ConstantInteger(Type.IntegerType.I32, parseNegHInt(s));
                }
                else {
                    return new Constant.ConstantInteger(Type.IntegerType.I32, String.valueOf(Integer.parseInt(s.substring(2), 16)));
                }
            }
            else if (s.length() >= 1 && s.charAt(0) == '0') {
                // 八进制数
                return new Constant.ConstantInteger(Type.IntegerType.I32, String.valueOf(Integer.parseInt(s, 8)));
            }
            else {
                return new Constant.ConstantInteger(Type.IntegerType.I32, s);
            }
        }
        else {
            // 浮点型常量
            return new Constant.ConstantFloat(new Type.FloatType(), Float.parseFloat(ctx.FLOATCONST().getText()));
        }
    }

    // addExp 或 addExp < addExp
    @Override
    public Value visitRelExp(SysYParser.RelExpContext ctx) {
        int size = ctx.getChildCount();
        if (size == 1) {
            return visit(ctx.getChild(0));
        }
        else {
            String sym = ctx.getChild(1).getText();
            Op temp = sym.equals("<") ? Op.Lt : sym.equals("<=") ? Op.Le
                    : sym.equals(">") ? Op.Gt : Op.Ge;
            Value first = visit(ctx.getChild(0));
            Value second = visit(ctx.getChild(2));
            return new BinaryOp(true, first, temp, second, curBB);
        }
    }

    @Override
    public Value visitUnaryExp(SysYParser.UnaryExpContext ctx) {
        int size = ctx.getChildCount();
        if (size == 1) {
            return visit(ctx.getChild(0));
        }
        else if (size == 2) {
            //实现 unaryOp getUnaryOp.symbol
            String sym = ctx.getChild(0).getChild(0).getText();
            Value value = visit(ctx.getChild(1));
            Op temp = sym.equals("+") ? Op.Add :
                    sym.equals("-") ? Op.Sub : Op.Not;
            if (temp.equals(Op.Add)) {
                return value;
            }
            else if (temp.equals(Op.Sub)) {
                if (value instanceof Constant.ConstantInteger) {
                    return new Constant.ConstantInteger(value.getType(), eval("0", value.getName(), temp));
                }
                else if (value instanceof Constant.ConstantFloat) {
                    return new Constant.ConstantFloat(value.getType(), -((Constant.ConstantFloat) value).getConstantFloatValue());
                }
                return new BinaryOp(false, Constant.ConstantInteger.Constant0, temp, value, curBB);
            }
            else {
                return new BinaryOp(true, Constant.ConstantInteger.Constant0, Op.Eq, value, curBB);
            }
        }
        else {
            String name = ctx.getChild(0).getText();//函数名
            Value func = MyModule.myModule.getFunctions().get(name);
            ArrayList<Value> args = new ArrayList<>();
            for (int i = 1; i < size; i++) {
                if (ctx.getChild(i) instanceof SysYParser.FuncRParamsContext) {
                    args = getRealArgs((SysYParser.FuncRParamsContext) ctx.getChild(i));
                }
                else {
                    visit(ctx.getChild(i));
                }
            }
            if (func.getType() instanceof Type.VoidType) {
                return new CallVoid(curBB, func, args);
            }
            return new Call(func.getType(), curBB, func, args);
        }
    }

    private Stack<Type> curInitType = new Stack<>();

    public IrTableItem getVar(ParseTree ctx, boolean isConst) {
        String name = "";
        Value initialValue = null;
        int size = ctx.getChildCount(); // 孩子大小
        ArrayList<Integer> arrayDim = new ArrayList<>(); // 维度
        name = ctx.getChild(0).getText(); // ident
        Type tmpType = varType;
        for (int i = 0; i < size; i++) {
            if (ctx.getChild(i).getText().equals("[")) {
                Value value = visit(ctx.getChild(i + 1));
                if (value instanceof Constant.ConstantInteger) { // 添加维度数字
                    arrayDim.add(Integer.valueOf(value.getName()));
                }
                else {
                    System.out.println("you cannot get const!!!");
                }
            }
        }
        int arraySize = arrayDim.size(); //多维 [1][2]
        for (int i = arraySize - 1; i >= 0; i--) {
            tmpType = new Type.ArrayType(tmpType, arrayDim.get(i));
        }
        curInitType.push(tmpType);
        for (int i = 0; i < size; i++) {
            if (ctx.getChild(i) instanceof SysYParser.InitValContext || ctx.getChild(i) instanceof SysYParser.ConstInitValContext) {
                initialValue = visit(ctx.getChild(i)); // 获取初始值
            }
        }
        Value pointer;
        Type initType = tmpType;
        if (isGlobal) {
            if (initialValue == null) {
                //全局变量未初始化
                if (tmpType instanceof Type.ArrayType) {
                    int length = tmpType.getLength();
                    tmpType = new Type.ArrayType(varType, length);
                    initialValue = new Constant.ZeroInitArray(tmpType, "zeroinitializer");
                }
                else {
                    if (tmpType.isIntegerType()) {
                        initialValue = Constant.ConstantInteger.Constant0;
                    }
                    else if (tmpType.isFloatType()) {
                        initialValue = Constant.ConstantFloat.float0;
                    }
                }
            } else if (tmpType instanceof Type.ArrayType && (((Constant.ConstantArray) initialValue).isAllZero())) {
                int length = tmpType.getLength();
                tmpType = new Type.ArrayType(varType, length);
                initialValue = new Constant.ZeroInitArray(tmpType, "zeroinitializer");
            }
            else {
                // 把全局数组都降为1维
                if (tmpType instanceof Type.ArrayType) {
                    int length = tmpType.getLength();
                    Type newType = new Type.ArrayType(varType, length);
                    initialValue = new Constant.ConstantArray(newType, "constInitial", ((Constant.ConstantArray) initialValue).getBasicValueList());
                    ((Constant.ConstantArray) initialValue).setSize(length);
                    initialValue.setName(initialValue.toString());
                    initialValue.setInitialType(initType);
                    tmpType = newType;
                }
                else if (tmpType.isFloatType()) {
                    if (initialValue.getType().isIntegerType()) {
                        initialValue = new Constant.ConstantFloat(new Type.FloatType(), ((Constant.ConstantInteger) initialValue).getConstantIntegerValue());
                    }
                }
                else if (tmpType.isIntegerType()) {
                    if (initialValue.getType().isFloatType()) {
                        initialValue = new Constant.ConstantInteger(Type.IntegerType.I32, String.valueOf((int) ((Constant.ConstantFloat) initialValue).getConstantFloatValue()));
                    }
                }
            }
            pointer = new GlobalVariable(tmpType, name, initialValue, isConst, initType);
        }
        else {
            // 局部变量/常量
            int length = tmpType.getLength();
            if (initialValue != null) {
                if (tmpType.isArrayType()) {
                    ArrayList<Value> basicValue = ((Constant.ConstantArray) initialValue).getBasicValueList();
                    Type newType = new Type.ArrayType(varType, length);
                    initialValue = new Constant.ConstantArray(newType, "constInitial", basicValue);
                    ((Constant.ConstantArray) initialValue).setSize(length);
                    initialValue.setName(initialValue.toString());
                    initialValue.setInitialType(initType);
                    tmpType = newType;
                }
            }
            if (initialValue == null) {
                if (tmpType.isArrayType()) {
                    tmpType = new Type.ArrayType(varType, length);
                }
            }
            pointer = new Alloc(tmpType, curBB);
            pointer.setInitialType(initType);
            if (initialValue == null) {
                if (tmpType.isIntegerType()) {
                    initialValue = Constant.ConstantInteger.Constant0;
                }
                else if (tmpType.isFloatType()) {
                    initialValue = Constant.ConstantFloat.float0;
                }
            }
            if (initialValue != null) {
                if (tmpType instanceof Type.ArrayType) {
                    ArrayList<Value> basicValue = ((Constant.ConstantArray) initialValue).getBasicValueList();
                    //局部数组初始化，需要先取出基地址
                    ArrayList<Value> offsets;
                    //以基地址为基准取其他的元素
                    Value tempPointer;
                    Value tempInit;
                    for (int i = 0; i < length; i++) {
                        offsets = new ArrayList<>();
                        offsets.add(Constant.ConstantInteger.Constant0);
                        offsets.add(new Constant.ConstantInteger(Type.IntegerType.I32, String.valueOf(i)));
                        tempPointer = new GetElementPtr(pointer, offsets, curBB);
                        tempInit = basicValue.get(i);
                        new Store(tempInit, tempPointer, curBB);
                    }
                }
                else {
                    //局部变量初始化
                    new Store(initialValue, pointer, curBB);
                }
            }
        }
        return new IrTableItem(name, tmpType, isConst, pointer, initialValue); //符号表中存储当初 alloc 的 pointer
    }

    // 隐式初始化
    public Value getArrayInitial(ParseTree ctx) {
        ArrayList<Value> values = new ArrayList<>();
        Constant.ConstantArray constArray = new Constant.ConstantArray(null, "constInitial", values);
        for (int i = 0; i < ctx.getChildCount(); i++) {
            if (ctx.getChild(i) instanceof SysYParser.ConstInitValContext || ctx.getChild(i) instanceof SysYParser.InitValContext) {
                curInitType.push(curInitType.peek().getElementType());
                values.add(visit(ctx.getChild(i)));
                curInitType.pop();
            }
        }
        int tarLen = curInitType.peek().getLength();
        int i = 0;
        for (Value value : values) {
            i += value.getType().getLength();
        }
        for (; i < tarLen; i++) {
            values.add(Constant.ConstantInteger.Constant0);
        }
        constArray.setSize(values.size());
        constArray.setType(new Type.ArrayType(values.get(0).getType(), values.size()));
        constArray.setName(constArray.toString());
        return constArray;
    }

    @Override
    public Value visitConstDef(SysYParser.ConstDefContext ctx) {
        tableStack.peek().addItem(getVar(ctx, true));
        return null;
    }

    @Override
    public Value visitVarDef(SysYParser.VarDefContext ctx) {
        tableStack.peek().addItem(getVar(ctx, false));
        return null;
    }

    @Override
    public Value visitConstInitVal(SysYParser.ConstInitValContext ctx) {
        if (ctx.getChildCount() == 1) {
            return visit(ctx.getChild(0));
        }
        else {
            return getArrayInitial(ctx);
        }
    }

    @Override
    public Value visitInitVal(SysYParser.InitValContext ctx) {
        if (ctx.getChildCount() == 1) {
            return visit(ctx.getChild(0));
        } else {
            return getArrayInitial(ctx);
        }
    }

    private boolean isConst = false;
    private boolean canGet = true;
    private Value initialValue;

    public ArrayList<Value> getOffsets(ParseTree ctx) {
        ArrayList<Value> offsets = new ArrayList<>();
        for (int i = 0; i < ctx.getChildCount(); i++) {
            if (ctx.getChild(i) instanceof SysYParser.ExpContext) {
                Value value = visit(ctx.getChild(i));
                if (!(value instanceof Constant.ConstantInteger)) {
                    this.canGet = false;
                }
                offsets.add(value);
            }
        }
        return offsets;
    }

    public ArrayList<Value> getOffset2gtr(ArrayList<Value> offsets) {
        int size = offsets.size();
        ArrayList<Value> offset2gtr = new ArrayList<>();
        offset2gtr.add(Constant.ConstantInteger.Constant0);
        if (size == 0) {
            //强制降维
            isForce = true;
            offset2gtr.add(Constant.ConstantInteger.Constant0);
        }
        else {
            offset2gtr.addAll(offsets);
        }
        return offset2gtr;
    }

    public Value getDown(Value pointer, ArrayList<Value> offset) {
        Value newPointer = new GetElementPtr(pointer, offset, curBB);
        //此时 newPointer 可能降到了 一维， 相当于 a，那就还需要 0 0
        while (!isForce && !newPointer.getType().getElementType().isBasicType()) {
            offset = getOffset2gtr(new ArrayList<>());
            newPointer = new GetElementPtr(newPointer, offset, curBB);
        }
        return newPointer;
    }

    public Value getRawPointer(ParseTree ctx) {
        String ident = ctx.getChild(0).getText();
        IrTableItem irTableItem = tableStack.peek().findItem(ident);
        isConst = irTableItem.isConst();
        initialValue = irTableItem.getInitValue();
        return irTableItem.getPointer();
    }

    @Override
    public Value visitLVal(SysYParser.LValContext ctx) {
        this.canGet = true; //默认可以取值
        ArrayList<Value> offsets = getOffsets(ctx);
        int size = offsets.size();
        Value pointer = getRawPointer(ctx); //符号表中的 alloc 时的 pointer
        boolean isArray = false;
        if (!(pointer.getType() instanceof Type.PointerType)) {
            System.out.println("the lval's pointer is not pointer");
        }
        else if (!(pointer.getType().getElementType()).isBasicType()) {
            isArray = true; //是数组或指针
        }
        if (isGlobal || isConst && canGet) {
            //目前在解析全局变量 读取a，则a必为constant int
            //局部变量也可能访问全局常量
            //在全局访问即 Global 中，访问的全局必为可以求值的 I32
            //在局部访问常量时，不一定可以求出值，则必须限制条件为可以求出常量，即偏移权全为 i32
            isForce = true;
            if (!isArray) {
                return initialValue;
            }
            else {
                int off = 0;
                Type curType = initialValue.getInitialType();
                for (Value offset : offsets) {
                    int tmp = ((Constant.ConstantInteger) offset).getConstantIntegerValue();
                    tmp *= curType.getElementType().getLength();
                    curType = curType.getElementType();
                    off += tmp;
                }
                return ((Constant.ConstantArray) initialValue).getValueByIndex(off);
            }
        }
        else {
            //局部变量
            if (isArray) {
                Type initType;
                if (pointer.getType().getElementType() instanceof Type.PointerType) {
                    initType = new Type.PointerType(pointer.getInitialType());
                    if (!pointer.getType().getElementType().getElementType().isBasicType()) {
                        pointer = new Load(pointer, curBB);
                    }
                    else {
                        pointer = new Load(pointer, curBB);
                        if (size != 0) {
                            return getDown(pointer, offsets);
                        }
                        else {
                            isForce = true;
                            return pointer;
                        }
                    }
                }
                else {
                    initType = pointer.getInitialType();
                }
                ArrayList<Value> offset2gtr = new ArrayList<>();
                offset2gtr.add(Constant.ConstantInteger.Constant0);

                // 优化：常数偏移不直接生成 mul 和 add 指令，而是加和之后最后生成一个 add
                Value off = Constant.ConstantInteger.Constant0;
                int constOff = 0;
                for (Value now : offsets) {
                    if (now instanceof Constant.ConstantInteger constInt) {
                        int i = constInt.getConstantIntegerValue();
                        int len = initType.getElementType().getLength();
                        constOff += i * len;
                    }
                    else {
                        now = new BinaryOp(false, now, Op.Mul, new Constant.ConstantInteger(Type.IntegerType.I32, String.valueOf(initType.getElementType().getLength())), curBB);
                        off = new BinaryOp(false, off, Op.Add, now, curBB);
                    }
                    initType = initType.getElementType();
                }
                // 如果常数偏移不为 0，生成 add 指令
                if (constOff > 0) {
                    var constOffValue = new Constant.ConstantInteger(Type.IntegerType.I32, String.valueOf(constOff));
                    off = new BinaryOp(false, off, Op.Add, constOffValue, curBB);
                }
                offset2gtr.add(off);
                pointer = new GetElementPtr(pointer, offset2gtr, curBB);
                if (!initType.isBasicType()) {
                    isForce = true;
                }
                return pointer;
            }
            return pointer; //返回地址
        }
    }

    public String eval(String a, String b, Op op) {
        int first = Integer.parseInt(a);
        int second = Integer.parseInt(b);
        switch (op) {
            case Add:
                return String.valueOf(first + second);
            case Sub:
                return String.valueOf(first - second);
            case Mul:
                return String.valueOf(first * second);
            case Div:
                return String.valueOf(first / second);
            case Mod:
                return String.valueOf(first % second);
            default:
                break;
        }
        return null;
    }
}