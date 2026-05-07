package backend.ir.handler;

import backend.ir.entity.*;
import backend.ir.entity.insts.*;
import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenType;
import frontend.semantic.symbol.*;
import frontend.syntactic.entity.ast.*;
import glob.types.BLOCK_TYPE;
import java.util.*;

enum LVAL_TYPE {
    ADDRESS,
    VALUE
}
/**
 * &#064;Classname IRParser
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 16:07
 * &#064;Created MuJue
 */
public class IRParser {
    private final List<BasicBlock> blockStack;
    // add -> mul1 + mul2 valueStack[top - 2]: mul1, valueStack[top - 1]: mul2
    // 取出栈顶的两个值，也就是mul1和mul2，然后相加 -> BinOpInst -> 押入到valueStack的栈顶
    private final List<Value> valueStack;
    // for 'BREAK' stmt
    private final List<BasicBlock> finalBlockStack;
    // for 'CONTINUE' stmt
    private final List<BasicBlock> condBlockStack;
    // 这是重点！！如果在处理到continue和break **恰好在** if分支中，那么此时因为会直接
    // 跳转到循环的条件处（continue）或直接跳出循环（break），而不需要跳转到if的最终块了。
    // 也就是说，我们应该记录那些已经有br指令的基本块。只要有了br指令，那么其后续就不应该有任何
    // Inst了！！！
    private final Set<BasicBlock> endBlock = new HashSet<>();
    private Program program;
    private Function curFunction;
    public static Integer indexInFunction = 0;
    private BasicBlock retBlock;
    private AllocateInst retAllocaInst;
    private List<AllocateInst> funcAllocaInsts = new ArrayList<>();
    private Integer strconNumber = 0;

    public IRParser() {
        blockStack = new ArrayList<>();
        valueStack = new ArrayList<>();
        finalBlockStack = new ArrayList<>();
        condBlockStack = new ArrayList<>();
        retBlock = null;
        retAllocaInst = null;
    }

    void addLibFunc(Program program) {
        createDeclare(new Token(TokenType.IDENTIFIER, "getint"), IRBasicType.I32, program);
        createDeclare(new Token(TokenType.IDENTIFIER, "getch"), IRBasicType.I32, program);
        createDeclare(new Token(TokenType.IDENTIFIER, "getfloat"), IRBasicType.FLOAT, program);

        Declare getarray = createDeclare(new Token(TokenType.IDENTIFIER, "getarray"), IRBasicType.I32, program);
        getarray.addArgument(new Argument("noundef", IRBasicType.I32, false, true, List.of(-1)));

        Declare getfarray = createDeclare(new Token(TokenType.IDENTIFIER, "getfarray"), IRBasicType.I32, program);
        getfarray.addArgument(new Argument("noundef", IRBasicType.FLOAT,false, true, List.of(-1)));

        Declare putint = createDeclare(new Token(TokenType.IDENTIFIER, "putint"), IRBasicType.VOID, program);
        putint.addArgument(new Argument("noundef", IRBasicType.I32, false, false,null));

        Declare putch = createDeclare(new Token(TokenType.IDENTIFIER, "putch"), IRBasicType.VOID, program);
        putch.addArgument(new Argument("noundef", IRBasicType.I32, false, false,null));

        Declare putfloat = createDeclare(new Token(TokenType.IDENTIFIER, "putfloat"), IRBasicType.VOID, program);
        putfloat.addArgument(new Argument("noundef", IRBasicType.FLOAT, false, false, List.of(-1)));

        Declare putarray = createDeclare(new Token(TokenType.IDENTIFIER, "putarray"), IRBasicType.VOID, program);
        putarray.addArgument(new Argument("noundef", IRBasicType.I32, false, false,null));
        putarray.addArgument(new Argument("noundef", IRBasicType.I32, false, true, List.of(-1)));

        Declare putfarray = createDeclare(new Token(TokenType.IDENTIFIER, "putfarray"), IRBasicType.VOID, program);
        putfarray.addArgument(new Argument("noundef", IRBasicType.I32, false, false, null));
        putfarray.addArgument(new Argument("noundef", IRBasicType.FLOAT,false, true,  List.of(-1)));

        Declare putf = createDeclare(new Token(TokenType.IDENTIFIER, "putf"), IRBasicType.VOID, program);
        putf.addArgument(new Argument("noundef", IRBasicType.I8, false, true, List.of(-1)));
        putf.addArgument(new Argument("...", IRBasicType.NONE, false, false,null));

        Declare mset = createDeclare(new Token(TokenType.IDENTIFIER, "llvm.memset.p0i8.i64"), IRBasicType.VOID, program);
        mset.addArgument(new Argument("noundef", IRBasicType.I8, false, true, List.of(-1)));
        mset.addArgument(new Argument("noundef", IRBasicType.I8, false, false, null));
        mset.addArgument(new Argument("noundef", IRBasicType.I64, false, false, null));
        mset.addArgument(new Argument("noundef", IRBasicType.I1, false, false, null));
    }

    public Program getProgram(CompileUnitNode compileUnitNode) {
        SymbolTable symbolTable = new SymbolTable();
        program = new Program(symbolTable);
        addLibFunc(program);
        parseCompileUnit(compileUnitNode);
        return program;
    }
    private void parseCompileUnitDecl(CompileUnitDeclNode compileUnitDeclNode) {
        if (compileUnitDeclNode.nodeType.equals(CompileUnitDeclNode.declType)) {
            parseDecl(compileUnitDeclNode.declNode);
        } else {
            parseFuncDef(compileUnitDeclNode.funcDefNode);
        }
    }

    private void parseCompileUnit(CompileUnitNode compileUnitNode) {
        for (CompileUnitDeclNode compileUnitDeclNode : compileUnitNode.declList) {
            parseCompileUnitDecl(compileUnitDeclNode);
        }
    }

    private List<Integer> getDimListWhenDef(List<ConstExpNode> constExpList) {
        List<Integer> dimList = new ArrayList<>();
        for (ConstExpNode constExpNode : constExpList) {
            parseConstExp(constExpNode);
            Value value = valueStack.getLast();
            assert value instanceof LiteralConst;
            dimList.add(((LiteralConst) value).getIntValue());
        }
        return dimList;
    }

    private void parseConstDecl(ConstDeclNode constDeclNode, boolean isGlobal) {
        for (ConstDefNode constDefNode : constDeclNode.constDefNodeList) {
            parseConstDef(constDefNode, constDeclNode.bTypeNode, isGlobal);
        }
    }
    private void parseConstDef(ConstDefNode constDefNode, BTypeNode bTypeNode, boolean isGlobal) {
        String varName = constDefNode.identToken.content;
        IRBasicType basicType = IRBasicType.BTNode2BT(bTypeNode);
        BasicBlock basicBlock = isGlobal ? null : blockStack.getLast();
        SymbolTable symbolTable = isGlobal ? program.getSymbolTable() : basicBlock.getSymbolTable();
        if (constDefNode.isArray()) {
            if (isGlobal) {
                List<Integer> dimList = getDimListWhenDef(constDefNode.dimensionExpList);
                GlobalVar globalVar = createGlobalVar(basicType, varName, true, true, dimList,  program);

                ArraySymbol arraySymbol = createArraySymbol(basicType, varName, true, true, dimList, symbolTable);
                arraySymbol.setValue(globalVar);

                parseConstInitVal(constDefNode.constInitValNode, bTypeNode, new ArrayList<>());

                assert valueStack.getLast() instanceof ArrayValue;
                ArrayValue arrayValue = (ArrayValue) valueStack.removeLast();
                if(!isEmptyArrayValue(arrayValue)){
                    arrayValue = getExpectArrayValue(basicType, getTotalSpace(dimList), dimList, arrayValue);
                    transArrayValueToTarBT(arrayValue, basicType, basicBlock);

                    arraySymbol.setArrayInitValue(arrayValue);
                    globalVar.addUsee(arrayValue);
                }
            } else {
                List<Integer> dimList = getDimListWhenDef(constDefNode.dimensionExpList);
                AllocateInst allocateInst = createAllocateInst(basicType,true, dimList, basicBlock);

                ArraySymbol arraySymbol = createArraySymbol(basicType, varName, true, false, dimList, symbolTable);
                arraySymbol.setValue(allocateInst);

                parseConstInitVal(constDefNode.constInitValNode, bTypeNode, new ArrayList<>());

                assert valueStack.getLast() instanceof ArrayValue;
                ArrayValue arrayValue = (ArrayValue) valueStack.removeLast();
                // 对于局部变量，如果没有初始值，还必须为其生成以一条全部清零的指令，这个指令
                // 还不是
                if(!isEmptyArrayValue(arrayValue)){
                    arrayValue = getExpectArrayValue(basicType, getTotalSpace(dimList), dimList, arrayValue);
                    transArrayValueToTarBT(arrayValue, basicType, basicBlock);

                    arraySymbol.setArrayInitValue(arrayValue);
                    parseArrayInit(basicBlock, allocateInst, arrayValue, dimList, basicType);
                }
                else{
                    createLocalZeroInitializeInst(allocateInst, getTotalSpace(dimList) * 4);
                }
            }
        } else {
            if (isGlobal) {
                VarSymbol varSymbol = createVarSymbol(basicType, varName, true, true, symbolTable);
                GlobalVar globalVar = createGlobalVar(basicType, varName, true, false,  null, program);
                varSymbol.setValue(globalVar);

                parseConstInitVal(constDefNode.constInitValNode, bTypeNode, new ArrayList<>());
                assert valueStack.getLast() instanceof LiteralConst;

                LiteralConst literalConst = (LiteralConst) valueStack.removeLast();
                literalConst = (LiteralConst)createTransInst(literalConst, basicType, basicBlock);
                globalVar.addUsee(literalConst);
                varSymbol.setConstInit(literalConst);
            } else {
                VarSymbol varSymbol = createVarSymbol(basicType, varName, true, false, symbolTable);
                AllocateInst allocateInst = createAllocateInst(basicType, true, null, basicBlock);
                varSymbol.setValue(allocateInst);

                parseConstInitVal(constDefNode.constInitValNode, bTypeNode, new ArrayList<>());
                assert valueStack.getLast() instanceof LiteralConst;

                LiteralConst literalConst = (LiteralConst) valueStack.removeLast();
                literalConst = (LiteralConst)createTransInst(literalConst, basicType, basicBlock);
                varSymbol.setConstInit(literalConst);

                createStoreInst(literalConst, allocateInst, basicBlock);
            }
        }
    }

    private void parseConstInitVal(ConstInitValNode constInitValNode, BTypeNode bTypeNode, ArrayList<Integer> integerArrayList) {
        if (constInitValNode.nodeType.equals(ConstInitValNode.constExpNodeType)) {
            parseConstExp(constInitValNode.constExpNode);
        } else {
            ArrayValue arrayValue = new ArrayValue("###ArrayValue", IRBasicType.BTNode2BT(bTypeNode));
            arrayValue.dimensionPlaceList.addAll(integerArrayList);
            for (int index = 0; index < constInitValNode.constInitValNodeArrayList.size(); index++) {
                ArrayList<Integer> indexList = new ArrayList<>(integerArrayList);
                indexList.add(index);
                parseConstInitVal(constInitValNode.constInitValNodeArrayList.get(index), bTypeNode, indexList);
                arrayValue.arrayValueUseList.add(valueStack.removeLast());
            }
            valueStack.addLast(arrayValue);
        }
    }

    private void parseDecl(DeclNode declNode) {
        if (declNode.nodeType.equals(DeclNode.constDeclType)) {
            parseConstDecl(declNode.constDeclNode, declNode.isGlobal);
        } else {
            parseVarDecl(declNode.varDeclNode, declNode.isGlobal);
        }
    }

    private void parseVarDecl(VarDeclNode varDeclNode, boolean isGlobal) {
        for (VarDefNode varDefNode : varDeclNode.varDefNodeList) {
            parseVarDef(varDefNode, varDeclNode.bTypeNode, isGlobal);
        }
    }

    private void parseVarDef(VarDefNode varDefNode, BTypeNode bTypeNode, boolean isGlobal) {
        String varName = varDefNode.identToken.content;
        IRBasicType basicType = IRBasicType.BTNode2BT(bTypeNode);
        BasicBlock basicBlock = isGlobal ? null : blockStack.getLast();
        SymbolTable symbolTable = isGlobal ? program.getSymbolTable() : basicBlock.getSymbolTable();

        if (varDefNode.isArrayDef()) {
            if (isGlobal) {
                List<Integer> dimList = getDimListWhenDef(varDefNode.constExpNodeArrayList);
                GlobalVar globalVar = createGlobalVar(basicType, varName, false, true, dimList, program);
                ArraySymbol arraySymbol = createArraySymbol(basicType, varName, false, true, dimList, symbolTable);
                arraySymbol.setValue(globalVar);

                if (varDefNode.initValNode != null) {
                    parseInitVal(varDefNode.initValNode, bTypeNode, new ArrayList<>());
                    assert valueStack.getLast() instanceof ArrayValue;
                    ArrayValue arrayValue = (ArrayValue) valueStack.removeLast();
                    if(!isEmptyArrayValue(arrayValue)){
                        arrayValue = getExpectArrayValue(basicType, getTotalSpace(dimList), dimList, arrayValue);
                        transArrayValueToTarBT(arrayValue, basicType, basicBlock);
                        globalVar.addUsee(arrayValue);
                    }
                }
            } else {
                List<Integer> dimList = getDimListWhenDef(varDefNode.constExpNodeArrayList);
                AllocateInst allocateInst = createAllocateInst(basicType, true, dimList, basicBlock);

                ArraySymbol arraySymbol = createArraySymbol(basicType, varName, false, false,  dimList, symbolTable);
                arraySymbol.setValue(allocateInst);

                if (varDefNode.initValNode != null) {
                    parseInitVal(varDefNode.initValNode, bTypeNode, new ArrayList<>());
                    assert valueStack.getLast() instanceof ArrayValue;
                    ArrayValue arrayValue = (ArrayValue) valueStack.removeLast();

                    if(!isEmptyArrayValue(arrayValue)){
                        arrayValue = getExpectArrayValue(basicType, getTotalSpace(dimList), dimList, arrayValue);
                        transArrayValueToTarBT(arrayValue, basicType, basicBlock);
                        parseArrayInit(basicBlock, allocateInst, arrayValue, dimList, basicType);
                    }
                    else{
                        createLocalZeroInitializeInst(allocateInst, getTotalSpace(dimList) * 4);
                    }
                }
            }
        } else {
            if (isGlobal) {
                VarSymbol varSymbol = createVarSymbol(basicType, varName, false, true, symbolTable);
                GlobalVar globalVar = createGlobalVar(basicType, varName, false, false, null, program);
                varSymbol.setValue(globalVar);

                if (varDefNode.initValNode != null) {
                    parseInitVal(varDefNode.initValNode, bTypeNode, new ArrayList<>());
                    Value initVal = valueStack.removeLast();
                    initVal = createTransInst(initVal, basicType, basicBlock);
                    globalVar.addUsee(initVal);
                }
            } else {
                AllocateInst allocateInst = createAllocateInst(basicType, false, null, basicBlock);
                VarSymbol varSymbol = createVarSymbol(basicType, varName, false, false, symbolTable);
                varSymbol.setValue(allocateInst);

                if (varDefNode.initValNode != null) {
                    parseInitVal(varDefNode.initValNode, bTypeNode, new ArrayList<>());
                    Value initExp = valueStack.removeLast();
                    IRBasicType tarBT = allocateInst.getBasicType();
                    initExp = createTransInst(initExp, tarBT, basicBlock);
                    createStoreInst(initExp, allocateInst, basicBlock);
                }
            }
        }
    }

    public void parseInitVal(InitValNode initValNode, BTypeNode bTypeNode, ArrayList<Integer> integerArrayList) {
        if (initValNode.nodeType.equals(InitValNode.expNodeType)) {
            parseExp(initValNode.expNode);
        } else {
            ArrayValue arrayValue = new ArrayValue("###ArrayValue", IRBasicType.BTNode2BT(bTypeNode));
            arrayValue.dimensionPlaceList.addAll(integerArrayList);
            for (int index = 0; index < initValNode.initValNodeArrayList.size(); index++) {
                ArrayList<Integer> indexList = new ArrayList<>(integerArrayList);
                indexList.add(index);
                parseInitVal(initValNode.initValNodeArrayList.get(index), bTypeNode, indexList);
                arrayValue.arrayValueUseList.add(valueStack.removeLast());
            }
            valueStack.addLast(arrayValue);
        }
    }

    private void handleFuncArg(Function function){
        BasicBlock childBlock = function.getSuperBlock();
        SymbolTable childTable = childBlock.getSymbolTable();

        List<Symbol> argSymbols = new ArrayList<>();
        List<Argument> arguments = function.getArguments();

        int argSize = arguments.size();
        for (Argument argument : arguments) {
            if (argument.isArray()) {
                ArraySymbol arraySymbol = createArraySymbol(argument.getBasicType(), argument.getName(), false, false, argument.getDimList(), childTable);
                argument.setName("%param_" + indexInFunction++);
                arraySymbol.setValue(argument);
                argSymbols.add(arraySymbol);
            } else {
                VarSymbol varSymbol = createVarSymbol(argument.getBasicType(), argument.getName(), false, false, childTable);
                argument.setName("%param_" + indexInFunction++);
                argSymbols.add(varSymbol);
            }
        }

        childBlock.setName("Block_"+Integer.toString(indexInFunction++));

        List<AllocateInst> allocateInsts = new ArrayList<>();

        for(int i = 0; i < argSize; i++){
            Symbol symbol = argSymbols.get(i);
            if(symbol instanceof ArraySymbol){
                allocateInsts.add(null);
                continue;
            }
            VarSymbol varSymbol = (VarSymbol) symbol;
            AllocateInst allocateInst = createAllocateInst(symbol.getType(), false, null, childBlock);
            varSymbol.setValue(allocateInst);
            allocateInsts.add(allocateInst);
        }
        for(int i = 0;i < argSize;i++){
            AllocateInst allocateInst = allocateInsts.get(i);
            if(allocateInst == null) continue;
            Argument argument = arguments.get(i);
            createStoreInst(argument, allocateInst, childBlock);
        }
    }
    private void parseFuncDef(FuncDefNode funcDefNode) {
        IRBasicType basicType = IRBasicType.FTNode2BT(funcDefNode.funcTypeNode);
        Function function = createFunc(funcDefNode.identToken, basicType, program);

        curFunction = function;
        endBlock.clear();
        indexInFunction = 0;
        funcAllocaInsts.clear();

        for (FuncFParamNode funcFParamNode : funcDefNode.funcFParamNodeArrayList) {
            parseFuncFParam(funcFParamNode);
        }

        SymbolTable rootTable = program.getSymbolTable();
        SymbolTable childTable = new SymbolTable(rootTable);

        BasicBlock childBlock = createEmptyBB(childTable, function, BLOCK_TYPE.SUPER_BLOCK);
        function.setSuperBlock(childBlock);
        handleFuncArg(function);

        // return basic block is something different with normal basic block.
        retBlock = createEmptyBB(childTable, function, BLOCK_TYPE.NORMAL_BLOCK);
        if (!basicType.equals(IRBasicType.VOID)) {
            retAllocaInst = createAllocateInst(basicType,false, null, childBlock);
        }

        blockStack.add(childBlock);
        parseBlock(funcDefNode.blockNode);
        blockStack.removeLast();

        int allocaSize = funcAllocaInsts.size();
        for(int i = allocaSize - 1; i >= 0; i--){
            AllocateInst allocateInst =  funcAllocaInsts.get(i);
            childBlock.addUseeAt(allocateInst, 0);
        }

        RetInst retInst = createRetInst(basicType, retBlock);
        retBlock.setName("Block_"+Integer.toString(indexInFunction));
        indexInFunction++;

        if (!basicType.equals(IRBasicType.VOID)) {
            LoadInst loadInst = createLoadInst(basicType, retAllocaInst, retBlock);
            retInst.addUsee(loadInst);
        }
        retBlock.addUsee(retInst);
        makeBlockEnd(childBlock);
        function.getSuperBlock().addUsee(retBlock);

        // 需要确保每个基本块都已经结束了。这个问题主要针对void函数，因为其可以不显式地调用
        // return，所以有可能出现“空”块。

        retBlock = null;
        curFunction = null;
        retAllocaInst = null;
    }
    private void parseFuncFParam(FuncFParamNode funcFParamNode) {
        assert curFunction != null;

        BTypeNode bTypeNode = funcFParamNode.bTypeNode;
        IRBasicType basicType = IRBasicType.BTNode2BT(bTypeNode);
        String paramName = funcFParamNode.identToken.content;
        List<Integer> dim = null;
        // 重要更新！！！！当数组的最后一个维度是-1，则代表这个变量是一个指针！！！
        if (funcFParamNode.isArrayParam) {
            dim = new ArrayList<>();
            for (ExpNode expNode : funcFParamNode.expNodeArrayList) {
                parseExp(expNode);
                assert valueStack.getLast() instanceof LiteralConst;
                LiteralConst lc = (LiteralConst) valueStack.removeLast();
                dim.add(lc.getIntValue());
            }
            dim.add(-1);
        }
        Argument argument = new Argument(paramName, basicType, false, dim != null, dim);
        curFunction.addArgmument(argument);
    }
    private void makeBlockEnd(BasicBlock basicBlock){
        if(!endBlock.contains(basicBlock)){
            createUCJumpInst(retBlock, basicBlock);
        }
        for(Value value: basicBlock.getUsees()){
            if(value instanceof BasicBlock childBlock){
                makeBlockEnd(childBlock);
            }
        }
    }
    private void parseBlock(BlockNode blockNode) {
        for (BlockItemNode blockItemNode : blockNode.blockItemNodeArrayList) {
            parseBlockItem(blockItemNode);
            if (endBlock.contains(blockStack.getLast())) {
                break;
            }
        }
    }
    private void parseBlockItem(BlockItemNode blockItemNode) {
        if (blockItemNode.nodeType.equals(BlockItemNode.declNodeType)) {
            parseDecl(blockItemNode.declNode);
        } else if (blockItemNode.nodeType.equals(BlockItemNode.stmtNodeType)) {
            parseStmt(blockItemNode.stmtNode);
        } else {
            System.out.println("the block item has wrong type");
        }
    }

    private void parseStmt(StmtNode stmtNode) {
        switch (stmtNode.stmtType) {
            case LVAL_STMT -> {
                BasicBlock currBlock = blockStack.getLast();
                parseLVal(stmtNode.lValStmtNode.lValNode, LVAL_TYPE.ADDRESS);
                Value lVal = valueStack.removeLast();
                parseExp(stmtNode.lValStmtNode.expNode);
                Value exp = valueStack.removeLast();
                IRBasicType tarBT = lVal.getBasicType();
                exp = createTransInst(exp, tarBT, currBlock);
                createStoreInst(exp, lVal, currBlock);
            }
            case EXP_STMT -> {
                if (stmtNode.expStmtNode.hasExp()) {
                    parseExp(stmtNode.expStmtNode.expNode);
                }
            }
            case BLOCK_STMT -> {
                BasicBlock currBlock = blockStack.removeLast();

                SymbolTable fatherTable = currBlock.getSymbolTable();
                SymbolTable childTable = new SymbolTable(fatherTable);
                BasicBlock childBlock = createBB(childTable, currBlock);

                createUCJumpInst(childBlock, currBlock);
                currBlock.addUsee(childBlock);

                blockStack.add(childBlock);
                parseBlock(stmtNode.blockStmtNode.blockNode);
                childBlock = blockStack.getLast();

                // 理论上，应该在childBlock中添加一条跳转到最终块的指令，而这个最终块和currBlock是同级别的。
                // **但是** ，由于currBlock可能是超级块，为了解决这个问题，我们就直接把Final Block当作currBlock
                // 的孩子块就行了。
                if (!endBlock.contains(childBlock)) {
                    BasicBlock finalBlock = createBB(currBlock.getSymbolTable(), currBlock);
                    currBlock.addUsee(finalBlock);
                    createUCJumpInst(finalBlock, childBlock);
                    blockStack.removeLast();
                    blockStack.add(finalBlock);
                }
            }
            // 对于IF和WHILE有一个重要结论:
            // 在处理完其Cond, Stmt之后，当前基本块 **不一定是提前创建好的** 那个基本块！
            case IF_STMT -> {
                // 获取当前基本块
                BasicBlock oriBlock =  blockStack.getLast();
                SymbolTable currTable = oriBlock.getSymbolTable();

                // 当前基本块应当作为if-else的父基本块
                BasicBlock fatherBlock = oriBlock;

                BasicBlock currBlock = oriBlock;
                // 所有新产生的基本块都和原先的基本块共同使用一个SymbolTable，是平级的。因为它们都是**逻辑上**的
                // 基本块，并不是程序中真正的基本块，而是我们为了方便处理if else而进行分割的基本块。

                SymbolTable condTable = currTable;
                BasicBlock condBlock = createEmptyBB(condTable, fatherBlock);

                SymbolTable trueTable = currTable;
                BasicBlock trueBlock = createEmptyBB(trueTable, fatherBlock);

                SymbolTable falseTable = currTable;
                BasicBlock falseBlock = createEmptyBB(falseTable, fatherBlock);

                SymbolTable finalTable = currTable;
                BasicBlock finalBlock = falseBlock;

                if (stmtNode.ifStmtNode.hasElseStmt()) {
                    finalBlock = createEmptyBB(finalTable, fatherBlock);
                }
                // 在执行完成无条件跳转后，currBlock的使命结束，因此退栈。
                createUCJumpInst(condBlock, currBlock);
                blockStack.removeLast();

                setBBNameLater(condBlock);
                fatherBlock.addUsee(condBlock);
                blockStack.add(condBlock);
                parseCond(stmtNode.ifStmtNode.condNode, trueBlock, falseBlock);
                blockStack.removeLast();


                setBBNameLater(trueBlock);
                fatherBlock.addUsee(trueBlock);
                // stmt在trueBlock中，所以需要进行设置
                blockStack.add(trueBlock);
                parseStmt(stmtNode.ifStmtNode.stmtNode);

                // 重点！！！如果if和else相关的stmt全部都return, continue, break，
                // 那么Final Block是不可达的！！！这种情况就没必要创建这个块了！
                // 而且，如果只有单if，那么在进行常量传播前，最终块一定是可达的。
                boolean isFinalReachable = false;

                currBlock = blockStack.removeLast();

                if(endBlock.contains(currBlock)){
                    Inst jumpInst = currBlock.getLastInst();
                    if(jumpInst instanceof BranchInst branchInst){
                        isFinalReachable |= branchInst.getUsee(1).equals(finalBlock) || branchInst.getUsee(2).equals(finalBlock);
                    }
                    else if(jumpInst instanceof UCJumpInst ucJumpInst){
                        isFinalReachable |= ucJumpInst.getUsee(0).equals(finalBlock);
                    }
                }
                else{
                    isFinalReachable = true;
                }
                createUCJumpInst(finalBlock, currBlock);

                // 如果还有else部分，那么还需要进行类似的处理
                if (stmtNode.ifStmtNode.hasElseStmt()) {
                    setBBNameLater(falseBlock);
                    fatherBlock.addUsee(falseBlock);
                    blockStack.add(falseBlock);
                    parseStmt(stmtNode.ifStmtNode.elseStmtNode);

                    currBlock = blockStack.removeLast();

                    if(endBlock.contains(currBlock)){
                        Inst jumpInst = currBlock.getLastInst();
                        if(jumpInst instanceof BranchInst branchInst){
                            isFinalReachable |= branchInst.getUsee(1).equals(finalBlock) || branchInst.getUsee(2).equals(finalBlock);
                        }
                        else if(jumpInst instanceof UCJumpInst ucJumpInst){
                            isFinalReachable |= ucJumpInst.getUsee(0).equals(finalBlock);
                        }
                    }
                    else{
                        isFinalReachable = true;
                    }
                    createUCJumpInst(finalBlock, currBlock);
                }
                else{
                    // 单if语句的Final Block一定可达。
                    isFinalReachable = true;
                }

                // 最后，我们现在应该处于finalBlock中。注意，如果Final Block不可达，
                // 我们没有必要进行设置；但是由于先前弹栈了一次，所以在这种情况下，可以把
                // 原先的基本块压栈。
                if(isFinalReachable){
                    setBBNameLater(finalBlock);
                    fatherBlock.addUsee(finalBlock);
                    blockStack.add(finalBlock);
                }
                else{
                    blockStack.add(oriBlock);
                }
            }
            case WHILE_STMT -> {
                // 取出当前的基本块。
                BasicBlock currBlock = blockStack.getLast();
                SymbolTable currTable = currBlock.getSymbolTable();

                BasicBlock fatherBlock = currBlock;

                // 单独为Cond建立一个基本块，这个基本块和原先的基本块是平级的
                SymbolTable condTable = currTable;
                BasicBlock condBlock = createEmptyBB(condTable, fatherBlock);

                SymbolTable trueTable = currTable;
                BasicBlock trueBlock = createEmptyBB(trueTable, fatherBlock);

                SymbolTable finalTable = currTable;
                BasicBlock finalBlock = createEmptyBB(finalTable, fatherBlock);

                // 当前处于的基本块是Cond基本块
                // 在currBlock中插入一个无条件跳转到condBlock的语句
                createUCJumpInst(condBlock, currBlock);
                blockStack.removeLast();

                setBBNameLater(condBlock);
                fatherBlock.addUsee(condBlock);
                blockStack.add(condBlock);
                parseCond(stmtNode.whileStmtNode.condNode, trueBlock, finalBlock);
                blockStack.removeLast();

                // 这个时候可以出现break和continue的语句了
                condBlockStack.add(condBlock);
                finalBlockStack.add(finalBlock);

                // 当前处于的基本块是Stmt的基本块
                setBBNameLater(trueBlock);
                blockStack.add(trueBlock);
                fatherBlock.addUsee(trueBlock);
                parseStmt(stmtNode.whileStmtNode.stmtNode);
                // 加入一个无条件挑战到Cond基本块的指令
                currBlock = blockStack.getLast();
                createUCJumpInst(condBlock, currBlock);

                blockStack.removeLast();
                condBlockStack.removeLast();
                finalBlockStack.removeLast();

                setBBNameLater(finalBlock);
                fatherBlock.addUsee(finalBlock);
                blockStack.add(finalBlock);
            }
            case BREAK_STMT -> {
                BasicBlock curBlock = blockStack.getLast();
                BasicBlock tarBlock = finalBlockStack.getLast();
                createUCJumpInst(tarBlock, curBlock);
            }
            case CONTINUE_STMT -> {
                BasicBlock curBlock = blockStack.getLast();
                BasicBlock tarBlock = condBlockStack.getLast();
                createUCJumpInst(tarBlock, curBlock);
            }
            case RETURN_STMT -> {
                BasicBlock curBlock = blockStack.getLast();
                IRBasicType funcType = curFunction.getBasicType();
                if (!funcType.equals(IRBasicType.VOID)) {
                    assert stmtNode.returnStmtNode.hasExp();
                    parseExp(stmtNode.returnStmtNode.expNode);
                    Value expVal = valueStack.removeLast();
                    expVal = createTransInst(expVal, funcType, curBlock);
                    createStoreInst(expVal, retAllocaInst, curBlock);
                }
                createUCJumpInst(retBlock, curBlock);
            }
            case PUT_F_STMT -> {
                String str = stmtNode.putfStmtNode.strConstToken.content;
                str = str.substring(0, str.length() - 1) + "\\00\"";
                ArrayList<Integer> dim = new ArrayList<>();
                Integer actualLen = str.length() - 4;
                dim.add(actualLen);

                GlobalVar strcon = createGlobalVar(IRBasicType.I8, ".str" + strconNumber++, true, true, dim, program);

                LiteralConst strLC = createLiteralConst(str);
                strcon.addUsee(strLC);

                SymbolTable curTable = program.getSymbolTable();
                FuncSymbol funcSymbol = (FuncSymbol) curTable.getSymbol("putf");
                BasicBlock curBlock = blockStack.getLast();

                Declare declare = (Declare)funcSymbol.getValue();
                IRBasicType IRBasicType = declare.getBasicType();

                List<Value> expValues = new ArrayList<>();
                if(stmtNode.putfStmtNode.expParamNodeList != null){
                    for (ExpNode expNode : stmtNode.putfStmtNode.expParamNodeList) {
                        parseExp(expNode);
                        Value expVal = valueStack.removeLast();
                        expValues.add(expVal);
                    }
                }

                LiteralConst zero = createLiteralConst(0, IRBasicType.I32);
                Value getElemPtrInst = createGetElemPtrInst(IRBasicType.I8, strcon, List.of(actualLen), List.of(zero), curBlock);
                CallInst callInst = createCallInst(IRBasicType, declare, curBlock);
                callInst.addUsee(getElemPtrInst);
                for (Value expValue : expValues) {
                    callInst.addUsee(expValue);
                }
            }
        }
    }

    private void parseExp(ExpNode expNode) {
        parseAddExp(expNode.addExpNode);
    }

    private void parseCond(CondNode condNode, BasicBlock trueBlock, BasicBlock falseBlock) {
        parseLOrExp(condNode.lOrExpNode, trueBlock, falseBlock);
    }
    private void parseNumber(NumberNode numberNode) {
        String content = numberNode.numberTypeToken.content;
        if (numberNode.isFloat()) {
            LiteralConst val = createLiteralConst(Float.parseFloat(content), IRBasicType.FLOAT);
            valueStack.add(val);
        } else if (numberNode.isInt()) {
            LiteralConst val = new LiteralConst(Integer.decode(content), IRBasicType.I32);
            valueStack.add(val);
        }
    }
    private void parsePrimaryExp(PrimaryExpNode primaryExpNode) {
        if (primaryExpNode.nodeType.equals(PrimaryExpNode.expNodeType)) {
            parseExp(primaryExpNode.expNode);
        } else if (primaryExpNode.nodeType.equals(PrimaryExpNode.lValNodeType)) {
            parseLVal(primaryExpNode.lValNode, LVAL_TYPE.VALUE);
        } else if (primaryExpNode.nodeType.equals(PrimaryExpNode.numberNodeType)) {
            parseNumber(primaryExpNode.numberNode);
        }
    }
    private void parseUnaryExp(UnaryExpNode unaryExpNode) {
        if (unaryExpNode.nodeType.equals(UnaryExpNode.primaryExpNodeType)) {
            parsePrimaryExp(unaryExpNode.primaryExpNode);
        }
        // 函数调用一定会出现在一个基本块中。
        else if (unaryExpNode.nodeType.equals(UnaryExpNode.identExpNodeType)) {
            BasicBlock curBlock = blockStack.getLast();
            SymbolTable curTable = curBlock.getSymbolTable();

            String funcName = unaryExpNode.identToken.content;
            FuncSymbol symbol = (FuncSymbol)curTable.getSymbol(funcName);
            IRBasicType IRBasicType = symbol.getType();
            CallInst callInst = null;

            Value func = symbol.getValue();
            List<Argument> arguments = null;
            if(func instanceof Function function){
                arguments = function.getArguments();

            }
            else if(func instanceof Declare declare){
                arguments = declare.getArguments();
            }
            assert arguments != null;
            assert arguments.size() == unaryExpNode.expParamNodeList.size();

            List<Value> expValues = new ArrayList<>();
            int argSize = arguments.size();
            for (int i = 0; i < argSize; i++) {
                ExpNode expNode = unaryExpNode.expParamNodeList.get(i);
                Argument argument = arguments.get(i);

                parseExp(expNode);
                Value expVal = valueStack.removeLast();
                IRBasicType argType = argument.getBasicType();

                expVal = createTransInst(expVal, argType, curBlock);
                expValues.add(expVal);
            }

            callInst = createCallInst(IRBasicType, func, curBlock);
            for (Value expValue : expValues) {
                callInst.addUsee(expValue);
            }

            if (callInst != null) {
                valueStack.add(callInst);
            }

        } else if (unaryExpNode.nodeType.equals(UnaryExpNode.unaryExpNodeType)) {
            parseUnaryExp(unaryExpNode.unaryExpNode);
            Value value = valueStack.removeLast();
            TokenType opTope = unaryExpNode.opToken.wordType;

            LiteralConst lc = doUniInstConstSpread(value, opTope);
            if(lc != null){
                valueStack.add(lc);
                return;
            }

            IRBasicType IRBasicType = value.getBasicType();
            BasicBlock curBlock = blockStack.getLast();

            if (opTope == TokenType.ADD) {
                valueStack.add(value);
            } else if (opTope == TokenType.SUB) {
                LiteralConst zero = createLiteralConst(0, IRBasicType);
                BinOpInst binOpInst = createBinOpInst(IRBasicType, TokenType.SUB, zero, value, curBlock);
                valueStack.add(binOpInst);
            } else if (opTope == TokenType.NOT) {
                // 如果进行!操作，一个简单的方法是，判断其是否和0相等。
                if(IRBasicType != IRBasicType.I1){
                    LiteralConst zero = createLiteralConst(0, IRBasicType);
                    CmpInst cmpInst = createCmpInst(IRBasicType, TokenType.EQ, zero, value, curBlock);
                    valueStack.add(cmpInst);
                }
                else{
                    LiteralConst one = createLiteralConst(1, IRBasicType);
                    BinOpInst binOpInst = createBinOpInst(IRBasicType, TokenType.XOR, value, one, curBlock);
                    valueStack.add(binOpInst);
                }
            }

        }
    }
    private void parseAddExp(AddExpNode addExpNode) {
        int size = addExpNode.mulExpNodeArrayList.size();

        List<Value> tempValueStack = new ArrayList<>();
        List<Token> tempTokenStack = new ArrayList<>();
        for (int i = 0; i < size; i++) {
            MulExpNode mulExpNode = addExpNode.mulExpNodeArrayList.get(i);
            parseMulExp(mulExpNode);
            tempValueStack.add(valueStack.removeLast());
            if (i >= 1) {
                Token opToken = addExpNode.opList.get(i - 1);
                tempTokenStack.add(opToken);
            }
        }
        doBinInstOp(tempValueStack, tempTokenStack);
        valueStack.add(tempValueStack.getFirst());
    }

    private void parseMulExp(MulExpNode mulExpNode) {
        int size = mulExpNode.unaryExpNodeArrayList.size();

        List<Value> tempValueStack = new LinkedList<>();
        List<Token> tempTokenStack = new LinkedList<>();
        for (int i = 0; i < size; i++) {
            UnaryExpNode unaryExpNode = mulExpNode.unaryExpNodeArrayList.get(i);
            parseUnaryExp(unaryExpNode);
            tempValueStack.add(valueStack.removeLast());
            if(i >= 1){
                Token opToken = mulExpNode.opList.get(i - 1);
                tempTokenStack.add(opToken);
            }
        }
        doBinInstOp(tempValueStack, tempTokenStack);
        valueStack.add(tempValueStack.getFirst());
    }

    private void parseRelExp(RelExpNode relExpNode) {
        int size = relExpNode.addExpNodeArrayList.size();

        List<Value> tempValueStack = new LinkedList<>();
        List<Token> tempTokenStack = new LinkedList<>();
        for(int i = 0; i < size; i++){
            AddExpNode addExpNode = relExpNode.addExpNodeArrayList.get(i);
            parseAddExp(addExpNode);
            tempValueStack.add(valueStack.removeLast());
            if(i >= 1){
                Token opToken = relExpNode.opList.get(i - 1);
                tempTokenStack.add(opToken);
            }
        }
        doCmpInstOp(tempValueStack, tempTokenStack);
        valueStack.add(tempValueStack.removeFirst());
    }

    private void parseEqExp(EqExpNode eqExpNode) {
        int size = eqExpNode.relExpNodeArrayList.size();

        List<Value> tempValueStack = new LinkedList<>();
        List<Token> tempTokenStack = new LinkedList<>();
        for(int i = 0; i < size; i++){
            RelExpNode relExpNode = eqExpNode.relExpNodeArrayList.get(i);
            parseRelExp(relExpNode);
            tempValueStack.add(valueStack.removeLast());
            if(i >= 1){
                Token opToken = eqExpNode.opList.get(i - 1);
                tempTokenStack.add(opToken);
            }
        }
        doCmpInstOp(tempValueStack, tempTokenStack);
        valueStack.add(tempValueStack.removeFirst());
    }

    // 这里的trueBlock和falseBlock是整个LAndExp分别为真和为假时，需要跳转到的基本块。
    // 由于是且运算的操作，所以如果一个EqExp为真，那么就需要跳转到 **下一个EqExp** 所在的基本块，否则
    // 就跳转到falseBlock。最后一个EqExp为真时，就直接跳转到trueBlock.
    private void parseLAndExp(LAndExpNode lAndExpNode, BasicBlock trueBlock, BasicBlock falseBlock) {
        int size = lAndExpNode.eqExpNodeArrayList.size();

        BasicBlock oriBlock = blockStack.removeLast();
        BasicBlock currblock = oriBlock;
        SymbolTable currTable = currblock.getSymbolTable();
        BasicBlock fatherBlock = currblock.getFatherBlock();

        for (int i = 0; i < size; i++) {
            EqExpNode eqExpNode = lAndExpNode.eqExpNodeArrayList.get(i);
            BasicBlock eqExpTrueBB = trueBlock;
            BasicBlock eqExpFalseBB = falseBlock;
            if (i != size - 1) {
                eqExpTrueBB = createEmptyBB(currTable, fatherBlock);
            }
            // 设置当前基本块
            blockStack.add(currblock);
            parseEqExp(eqExpNode);
            blockStack.removeLast();

            Value eqValue = valueStack.removeLast();
            IRBasicType IRBasicType = eqValue.getBasicType();
            if (!IRBasicType.equals(IRBasicType.I1)) {
                LiteralConst zero = createLiteralConst(0, IRBasicType);
                CmpInst cmpInst = createCmpInst(IRBasicType, TokenType.NE, eqValue, zero, currblock);
                eqValue = cmpInst;
            }
            createBranchInst(eqValue, eqExpTrueBB, eqExpFalseBB, currblock);

            if (!eqExpTrueBB.equals(trueBlock)) {
                setBBNameLater(eqExpTrueBB);
            }
            currblock = eqExpTrueBB;
            if (!currblock.equals(oriBlock) && !currblock.equals(trueBlock) && !currblock.equals(falseBlock)) {
                fatherBlock.addUsee(currblock);
            }
        }
        blockStack.add(currblock);
    }

    // 一个LOrExp又有需要的LAndExp构成，那么对于这些LAndExp而言，只要任意一个为真，LOrExp为真，所以
    // 它们需要跳转到的就是trueBlock；如果某一个LAndExp为假，那么其应该跳转到的基本块是 **下一个LAndExp**
    // 所在的基本块，而不是falseBlock。只有最后一个LAndExp为假，需要跳转的基本块才是falseBlock。
    private void parseLOrExp(LOrExpNode lOrExpNode, BasicBlock trueBlock, BasicBlock falseBlock) {
        int size = lOrExpNode.lAndExpNodeArrayList.size();

        BasicBlock oriBlock = blockStack.removeLast();
        BasicBlock currBlock = oriBlock;
        SymbolTable currTable = currBlock.getSymbolTable();
        BasicBlock fatherBlock = currBlock.getFatherBlock();

        for (int i = 0; i < size; i++) {
            LAndExpNode lAndExpNode = lOrExpNode.lAndExpNodeArrayList.get(i);
            BasicBlock lAndTrueBB = trueBlock;
            BasicBlock lAndFalseBB = falseBlock;
            // 后面还有LAndExp
            if (i != size - 1) {
                lAndFalseBB = createEmptyBB(currTable, fatherBlock);
            }
            // 设置当前基本块
            blockStack.add(currBlock);
            parseLAndExp(lAndExpNode, lAndTrueBB, lAndFalseBB);
            blockStack.removeLast();

            // 下一条指令所在的基本块就是lAndFalseBB.
            if (!lAndFalseBB.equals(falseBlock)) {
                setBBNameLater(lAndFalseBB);
            }
            currBlock = lAndFalseBB;
            if (!currBlock.equals(oriBlock) && !currBlock.equals(trueBlock) && !currBlock.equals(falseBlock)) {
                fatherBlock.addUsee(currBlock);
            }
        }
        blockStack.add(currBlock);
    }
    // 注意，这里的trueBlock代表的是整个LOrExp为真时，应该跳转到的基本块，而falseBlock则代表的是当
    // 整个LOrExp为假时应该跳转到的基本块。

    private void parseConstExp(ConstExpNode constExpNode) {
        parseAddExp(constExpNode.addExpNode);
        assert valueStack.getLast() instanceof LiteralConst;
    }

    private void parseLVal(LValNode lValNode, LVAL_TYPE lValType) {
        BasicBlock curBlock = blockStack.isEmpty() ? null : blockStack.getLast();
        SymbolTable symbolTable = curBlock == null ? program.getSymbolTable() : curBlock.getSymbolTable();
        String lValName = lValNode.identToken.content;
        Symbol lVal = symbolTable.getSymbol(lValName);
        Value declValue = lVal.getValue();
        IRBasicType IRBasicType = declValue.getBasicType();

        if (lValType == LVAL_TYPE.VALUE) {
            if (lVal instanceof ArraySymbol arraySymbol) {
                ArrayList<Value> indexExpList = new ArrayList<>();
                boolean isAllConst = true;
                for (ExpNode expNode : lValNode.expNodeArrayList) {
                    parseExp(expNode);
                    Value idxValue = valueStack.removeLast();
                    isAllConst &= idxValue instanceof LiteralConst;
                    idxValue = createTransInst(idxValue, IRBasicType.I32, curBlock);
                    indexExpList.add(idxValue);
                }
                // 此时：
                // 1. 数组变量的初始化是常数；
                // 2. 每一个下标也是常数；
                // 3. 下标的维数和数组的维数相等；
                // 所以可以进行常量传播。
                if (isAllConst && arraySymbol.isArrayConst()) {
                    LiteralConst literalConst = getLiteralConst(arraySymbol, indexExpList);
                    valueStack.add(literalConst);
                } else {
                    List<Integer> dimList = new LinkedList<>(arraySymbol.getDim());
                    // 首先，指针也是一个维度，所以将指针（-1）存储在dimList是没问题的；
                    // 那么只有当总的idx的大小等同于维度的时候，才可以load一个具体的值，否则
                    // 都是指针。
                    // 所以，这里完成了形式的统一：
                    // 1. 如果取到值，那么在末尾需要有一次Load;
                    // 2. 如果取到地址，那么GEP的末尾也需要有一个i32 0;
                    // 其本质上都是“多”走了一步。
                    boolean isPtr = indexExpList.size() < dimList.size();
                    if(isPtr){
                        LiteralConst zero = createLiteralConst(0, IRBasicType.I32);
                        indexExpList.add(zero);
                    }
                    Inst finalLVal = createGetElemPtrInst(arraySymbol.getType(), declValue, dimList, indexExpList, curBlock);
                    if(!isPtr){
                        finalLVal = createLoadInst(IRBasicType, finalLVal, curBlock);
                    }
                    valueStack.add(finalLVal);
                }
            } else if (lVal instanceof VarSymbol var) {
                if (var.isConst()) {
                    valueStack.add(var.getConstInit());
                } else {
                    LoadInst loadInst = createLoadInst(IRBasicType, declValue, curBlock);
                    valueStack.add(loadInst);
                }
            }
        } else if (lValType == LVAL_TYPE.ADDRESS) {
            if (lVal instanceof ArraySymbol arraySymbol) {
                ArrayList<Value> indexExpList = new ArrayList<>();
                for (ExpNode expNode : lValNode.expNodeArrayList) {
                    parseExp(expNode);
                    Value idxValue = valueStack.removeLast();
                    idxValue = createTransInst(idxValue, IRBasicType.I32, curBlock);
                    indexExpList.add(idxValue);
                }
                List<Integer> dimList = arraySymbol.getDim();
                Value finalLVal = createGetElemPtrInst(arraySymbol.getType(), declValue, dimList, indexExpList, curBlock);
                valueStack.add(finalLVal);
            } else if (lVal instanceof VarSymbol var) {
                valueStack.add(declValue);
            }
        }
    }

    private LiteralConst getLiteralConst(ArraySymbol arraySymbol, ArrayList<Value> indexExpList) {
        int expSize = indexExpList.size();
        int dimSize = arraySymbol.getDim().size();
        assert expSize == dimSize;
        LiteralConst literalConst = null;
        ArrayValue currValue = arraySymbol.getArrayInitValue();
        for (int i = 0; i < expSize; i++) {
            LiteralConst lc = (LiteralConst) indexExpList.get(i);
            if (i != expSize - 1) {
                currValue = (ArrayValue) currValue.arrayValueUseList.get(lc.getIntValue());
            } else {
                literalConst = (LiteralConst) currValue.arrayValueUseList.get(lc.getIntValue());
            }
        }
        return literalConst;
    }
    AllocateInst createAllocateInst(IRBasicType IRBasicType, Boolean isConst, List<Integer> dim, BasicBlock bb) {
        AllocateInst allocateInst = new AllocateInst("%var_" + indexInFunction, IRBasicType, isConst, dim != null, dim,  bb);
        indexInFunction++;
        funcAllocaInsts.add(allocateInst);
        return allocateInst;
    }

    BinOpInst createBinOpInst(IRBasicType IRBasicType, TokenType opType, Value v1, Value v2, BasicBlock bb) {
        BinOpInst binOpInst = new BinOpInst("%var_" + indexInFunction, IRBasicType, opType, bb);
        indexInFunction++;
        binOpInst.addUsee(v1);
        binOpInst.addUsee(v2);
        bb.addUsee(binOpInst);
        return binOpInst;
    }

    CmpInst createCmpInst(IRBasicType IRBasicType, TokenType opType, Value v1, Value v2, BasicBlock bb) {
        CmpInst cmpInst = new CmpInst("%var_" + indexInFunction, IRBasicType, opType, bb);
        indexInFunction++;
        cmpInst.addUsee(v1);
        cmpInst.addUsee(v2);
        bb.addUsee(cmpInst);
        return cmpInst;
    }

    BranchInst createBranchInst(Value v1, BasicBlock trueBB, BasicBlock falseBB, BasicBlock bb) {
        if (endBlock.contains(bb)) return null;
        BranchInst branchInst = new BranchInst("%var_" + indexInFunction, IRBasicType.I1, bb);
        branchInst.addUsee(v1);
        branchInst.addUsee(trueBB);
        branchInst.addUsee(falseBB);
        bb.addUsee(branchInst);
        endBlock.add(bb);
        return branchInst;
    }

    CallInst createCallInst(IRBasicType IRBasicType, Value value, BasicBlock bb) {
        String name = "";
        if (!IRBasicType.equals(IRBasicType.VOID)) {
            name = "%var_" + indexInFunction;
            indexInFunction++;
        }
        CallInst callInst = new CallInst(name, IRBasicType, bb);
        callInst.addUsee(value);
        bb.addUsee(callInst);
        return callInst;
    }

    StoreInst createStoreInst(Value value, Value addr, BasicBlock bb) {
        StoreInst storeInst = new StoreInst(addr.getBasicType(), bb);
        storeInst.addUsee(value);
        storeInst.addUsee(addr);
        bb.addUsee(storeInst);
        return storeInst;
    }

    LoadInst createLoadInst(IRBasicType IRBasicType, Value addr, BasicBlock bb) {
        LoadInst loadInst = new LoadInst("%var_" + indexInFunction, IRBasicType, bb);
        indexInFunction++;
        loadInst.addUsee(addr);
        bb.addUsee(loadInst);
        return loadInst;
    }
    Value createTransInst(Value oriValue, List<Integer> toDimList, IRBasicType toType, BasicBlock bb) {
        IRBasicType fromType = oriValue.getBasicType();
        if (fromType == toType) return oriValue;
        if(oriValue instanceof LiteralConst lc){
            return createLiteralConst(lc.getNumber(), toType);
        }
        TransInst transInst = new TransInst("%var_" + indexInFunction, toType, toDimList, bb);
        indexInFunction++;
        transInst.addUsee(oriValue);
        bb.addUsee(transInst);
        return transInst;
    }
    Value createTransInst(Value oriValue, IRBasicType toType, BasicBlock bb) {
        IRBasicType fromType = oriValue.getBasicType();
        if (fromType == toType) return oriValue;
        if(oriValue instanceof LiteralConst lc){
            return createLiteralConst(lc.getNumber(), toType);
        }
        TransInst transInst = new TransInst("%var_" + indexInFunction, toType, null, bb);
        indexInFunction++;
        transInst.addUsee(oriValue);
        bb.addUsee(transInst);
        return transInst;
    }

    UCJumpInst createUCJumpInst(BasicBlock tarBB, BasicBlock bb) {
        if (endBlock.contains(bb)) return null;
        UCJumpInst ucJumpInst = new UCJumpInst(bb);
        ucJumpInst.addUsee(tarBB);
        bb.addUsee(ucJumpInst);
        endBlock.add(bb);
        return ucJumpInst;
    }
    Inst createGetElemPtrInst(IRBasicType IRBasicType, Value value, List<Integer> dimList, List<Value> indexValues, BasicBlock bb){
        List<Integer> getElemInstDimList = new LinkedList<>(dimList);
        // GEP 需要对处理的变量是数组还是指针进行特判。
        boolean isPtr = !getElemInstDimList.isEmpty() && getElemInstDimList.getLast() == -1;
        if(!isPtr){
            getElemInstDimList.add(-1);
        }
        // 注意！！对于GetElementInst本身的类型，其不仅和数组原有的类型有关，还和下表表达式的数量也有关！
        int idxSize = indexValues.size() - (isPtr ? 1 : 0);
        for(int i = 0; i < idxSize; i++){
            getElemInstDimList.removeFirst();
        }

        GetElemPtrInst getElemPtrInst = new GetElemPtrInst("%var_" + indexInFunction, IRBasicType, getElemInstDimList, bb);
        indexInFunction++;
        bb.addUsee(getElemPtrInst);
        assert value != null;

        getElemPtrInst.addUsee(value);
        for(Value idxValue : indexValues){
            getElemPtrInst.addUsee(idxValue);
        }

        // 左值并不是一个指针，那么就需要额外添加一个0，表示偏移。
        if(dimList.isEmpty() || dimList.getLast() != -1){
            LiteralConst zero = createLiteralConst(0, IRBasicType.I32);
            getElemPtrInst.addUseeAt(zero, 1);
        }

        return getElemPtrInst;
    }

    // 注意，在创建一个返回指令时，我们没有将该返回指令加入到RetBlock中！
    RetInst createRetInst(IRBasicType IRBasicType, BasicBlock bb) {
        return new RetInst("retInst", IRBasicType, bb);
    }
    private LiteralConst createLiteralConst(String str){
        return new LiteralConst(str);
    }
    private LiteralConst createLiteralConst(Number val, IRBasicType IRBasicType){
        if(IRBasicType == IRBasicType.I32 || IRBasicType == IRBasicType.I64 || IRBasicType == IRBasicType.I1 || IRBasicType == IRBasicType.I8){
            return new LiteralConst(val.intValue(), IRBasicType);
        }
        else if(IRBasicType == IRBasicType.FLOAT){
            return new LiteralConst(val.floatValue(), IRBasicType);
        }
        return null;
    }
    ArraySymbol createArraySymbol(IRBasicType IRBasicType, String varName, Boolean isConst, Boolean isGlobal, List<Integer> dimensionList, SymbolTable table) {
        ArraySymbol arraySymbol = new ArraySymbol(varName, IRBasicType, isConst, isGlobal, dimensionList);
        table.addSymbol(arraySymbol);
        return arraySymbol;
    }

    VarSymbol createVarSymbol(IRBasicType IRBasicType, String varName, Boolean isConst, Boolean isGlobal, SymbolTable table) {
        VarSymbol varSymbol = new VarSymbol(varName, IRBasicType, isConst, isGlobal);
        table.addSymbol(varSymbol);
        return varSymbol;
    }

    GlobalVar createGlobalVar(IRBasicType IRBasicType, String name, Boolean isConst, Boolean isArray, List<Integer> dim, Program program) {
        GlobalVar globalVar = new GlobalVar(IRBasicType, name, isConst, isArray, dim);
        int declareIndex = program.getDeclareIndex();
        program.addUseeAt(globalVar, declareIndex);
        program.setDeclareIndex(declareIndex + 1);
        return globalVar;
    }

    Function createFunc(Token identToken, IRBasicType IRBasicType, Program program) {
        Function function = new Function(identToken.content, IRBasicType, program);
        FuncSymbol symbol = new FuncSymbol(identToken.content, IRBasicType, function);
        program.getSymbolTable().addSymbol(symbol);
        program.addUsee(function);
        return function;
    }

    Declare createDeclare(Token identToken, IRBasicType IRBasicType, Program program) {
        Declare declare = new Declare(identToken.content, IRBasicType);
        FuncSymbol symbol = new FuncSymbol(identToken.content, IRBasicType, declare);
        program.getSymbolTable().addSymbol(symbol);
        program.addUsee(declare);
        return declare;
    }

    BasicBlock createBB(SymbolTable table, BasicBlock fatherBlock) {
        BasicBlock block = new BasicBlock("Block_"+Integer.toString(indexInFunction), table, fatherBlock);
        indexInFunction++;
        return block;
    }
    BasicBlock createBB(SymbolTable table, Function function) {
        BasicBlock block = new BasicBlock("Block_"+Integer.toString(indexInFunction), table, function, BLOCK_TYPE.SUPER_BLOCK);
        indexInFunction++;
        return block;
    }
    BasicBlock createEmptyBB(SymbolTable table, BasicBlock fatherBlock) {
        return new BasicBlock("undefined", table, fatherBlock);
    }
    BasicBlock createEmptyBB(SymbolTable table, Function function, BLOCK_TYPE blockType){
        return new BasicBlock("undefined", table, function, blockType);
    }

    void setBBNameLater(BasicBlock block) {
        block.setName("Block_" + Integer.toString(indexInFunction));
        indexInFunction++;
    }
    private Integer getTotalSpace(List<Integer> dimList){
        Integer ans = 1;
        for(Integer dim: dimList){
            ans *= dim;
        }
        return ans;
    }
    private void transArrayValueToTarBT(ArrayValue arrayValue, IRBasicType IRBasicType, BasicBlock basicBlock){
        int usedValueSize = arrayValue.arrayValueUseList.size();
        for(int i = 0; i < usedValueSize; i++){
            if(arrayValue.arrayValueUseList.get(i) instanceof ArrayValue arrayValue1){
                transArrayValueToTarBT(arrayValue1, IRBasicType, basicBlock);
            }
            else{
                arrayValue.arrayValueUseList.set(i, createTransInst(arrayValue.arrayValueUseList.get(i), IRBasicType, basicBlock));
            }
        }
    }
    private boolean isEmptyArrayValue(ArrayValue arrayValue){
        if(arrayValue.arrayValueUseList.isEmpty()){
            return true;
        }
        int size = arrayValue.arrayValueUseList.size();
        for(int i = 0; i < size; i++){
            if(!(arrayValue.arrayValueUseList.get(i) instanceof LiteralConst lc) || lc.getIntValue() != 0){
                return false;
            }
        }
        return true;
    }
    private void createLocalZeroInitializeInst(AllocateInst allocateInst, int totSize){
        BasicBlock currBlock = blockStack.getLast();
        Value transInst = createTransInst(allocateInst, List.of(-1), IRBasicType.I8, currBlock);
        Value func = program.getSymbolTable().getSymbol("llvm.memset.p0i8.i64").getValue();
        CallInst callInst = createCallInst(IRBasicType.VOID, func, currBlock);

        callInst.addUsee(transInst);
        callInst.addUsee(createLiteralConst(0, IRBasicType.I8));
        callInst.addUsee(createLiteralConst(totSize, IRBasicType.I64));
        callInst.addUsee(createLiteralConst(0, IRBasicType.I1));
    }
    private ArrayValue getExpectArrayValue(IRBasicType IRBasicType, int curSize, List<Integer> dimList, ArrayValue oriArrayValue){
        int curDim = dimList.getFirst();
        int elemSize = curSize / curDim;
        int usedValueSize = oriArrayValue.arrayValueUseList.size();

        if(dimList.size() == 1){
            assert usedValueSize <= curDim;
            for(int i = 0; i < usedValueSize; i++){
                assert !(oriArrayValue.arrayValueUseList.get(i) instanceof ArrayValue);
            }
            for(int i = usedValueSize; i < curDim; i++){
                LiteralConst zero = createLiteralConst(0, IRBasicType);
                oriArrayValue.arrayValueUseList.add(zero);
            }
            oriArrayValue.dimList = List.of(curDim);

            return oriArrayValue;
        }

        ArrayValue retArrayValue = new ArrayValue("JB array value", IRBasicType);
        retArrayValue.dimList = new ArrayList<>();
        retArrayValue.dimList.add(curDim);
        int curInd = 0;
        for(int i = 0; i < curDim; i++){
            ArrayValue childValue = null;
            if(curInd < usedValueSize && oriArrayValue.arrayValueUseList.get(curInd) instanceof ArrayValue arrayValue){
                childValue = getExpectArrayValue(IRBasicType, elemSize, dimList.subList(1, dimList.size()), arrayValue);
                curInd++;
            }
            else{
                childValue = new ArrayValue("JB array value", IRBasicType);
                for(int j = 0; j < elemSize; j++){
                    if(curInd == usedValueSize ||  oriArrayValue.arrayValueUseList.get(curInd) instanceof ArrayValue){
                        while(j != elemSize){
                            LiteralConst zero = createLiteralConst(0, IRBasicType);
                            childValue.arrayValueUseList.add(zero);
                            j++;
                        }
                        break;
                    }
                    else{
                        childValue.arrayValueUseList.add(oriArrayValue.arrayValueUseList.get(curInd));
                        curInd++;
                    }
                }
                childValue = getExpectArrayValue(IRBasicType, elemSize, dimList.subList(1, dimList.size()), childValue);
            }
            retArrayValue.arrayValueUseList.add(childValue);
        }
        retArrayValue.dimList.addAll(((ArrayValue)retArrayValue.arrayValueUseList.get(0)).dimList);
        return retArrayValue;
    }
    private void parseArrayInit(BasicBlock basicBlock, AllocateInst allocateInst, ArrayValue arrayValue, List<Integer> dimList, IRBasicType IRBasicType) {
        Integer totSize = 1;
        for(Integer dim : dimList){
            totSize *= dim;
        }

        for(int i = 0; i < totSize; i++){
            int curSize = totSize;
            int ci = i;
            Value value = arrayValue;

            List<Value> idxExpList = new ArrayList<>();
            int curIdxIdx = 0;
            while(value instanceof ArrayValue arrayValue1){
                curSize /= dimList.get(curIdxIdx);
                curIdxIdx++;
                value = arrayValue1.arrayValueUseList.get(ci / curSize);
                LiteralConst nextInd = createLiteralConst(ci / curSize, IRBasicType.I32);
                idxExpList.add(nextInd);
                ci %= curSize;
            }
            Value getElemPtrInst = createGetElemPtrInst(IRBasicType, allocateInst, dimList, idxExpList, basicBlock);
            value = createTransInst(value, getElemPtrInst.getBasicType(), basicBlock);
            createStoreInst(value, getElemPtrInst, basicBlock);
        }
    }
    private void doBinInstOp(List<Value> tempValueStack, List<Token> tempTokenStack){
        assert tempValueStack.size() == tempTokenStack.size() + 1;
        IRBasicType opType = getInstOperandBasicType(tempValueStack);
        doBinInstConstSpread(opType, tempValueStack, tempTokenStack);
        // 如果获得的值的数量超过了1，那么一定会进行二元运算，那么这个二元运算
        // 就一定会出现在BasicBlock中，所以必须要先判断值得个数才取BasicBlock。
        if (tempValueStack.size() != 1) {
            BasicBlock curBlock = blockStack.getLast();
            while (tempValueStack.size() != 1) {
                Value v1 = tempValueStack.removeFirst();
                Value v2 = tempValueStack.removeFirst();
                Token opToken = tempTokenStack.removeFirst();

                v1 = createTransInst(v1, opType, curBlock);
                v2 = createTransInst(v2, opType, curBlock);

                BinOpInst binOpInst = createBinOpInst(opType, opToken.wordType, v1, v2, curBlock);
                tempValueStack.addFirst(binOpInst);
            }
        }
    }
    private void doCmpInstOp(List<Value> tempValueStack, List<Token> tempTokenStack){
        assert tempValueStack.size() == tempTokenStack.size() + 1;
        IRBasicType opType = getInstOperandBasicType(tempValueStack);
        doCmpInstConstSpread(opType, tempValueStack, tempTokenStack);
        if (tempValueStack.size() != 1) {
            BasicBlock curBlock = blockStack.getLast();
            while (tempValueStack.size() != 1) {
                Value v1 = tempValueStack.removeFirst();
                Value v2 = tempValueStack.removeFirst();
                Token opToken = tempTokenStack.removeFirst();

                v1 = createTransInst(v1, opType, curBlock);
                v2 = createTransInst(v2, opType, curBlock);

                CmpInst cmpInst = createCmpInst(opType, opToken.wordType, v1, v2, curBlock);
                tempValueStack.addFirst(cmpInst);
            }
        }
    }
    private IRBasicType getInstOperandBasicType(List<Value> values){
        int vsize = values.size();
        for(int i = 0; i < vsize; i++){
            Value value = values.get(i);
            if(value.getBasicType() == IRBasicType.FLOAT){
                return IRBasicType.FLOAT;
            }
        }
        return IRBasicType.I32;
    }
    private TokenType getOpTypeForCS(TokenType tokenType){
        if(tokenType == TokenType.SUB){
            return TokenType.ADD;
        }
        if(tokenType == TokenType.DIV){
            return TokenType.MUL;
        }
        return tokenType;
    }
    private LiteralConst doUniInstConstSpread(Value unaryExp, TokenType opType){
        if(unaryExp instanceof LiteralConst lc){
            return doLCOp(lc, null, opType);
        }
        return null;
    }
    // 重点！！常量传播！！
    // op k1 op k2 op k3 op k4，
    // 其中的op的类型是一样的，那么我们的常量传播会改成如下的形式：
    // op k'， 其中的k'是与k1 op k2 op k3 op k4等价的一个值。
    // 这也就代表着最后化简成的k1' op1 k2' op2 k3' op3 k4'仍然可以做常量传播，
    // 但是可以保证的是，任意的opi和opj都不是一样的。（%除外）。
    private void doBinInstConstSpread(IRBasicType tarType, List<Value> values, List<Token> opTokens) {
        if(values.size() == 1){
            return;
        }
        TokenType opLevel = null;
        switch (opTokens.get(0).wordType){
            case ADD :
            case SUB :
                opLevel = TokenType.ADD;
                break;
            case MUL:
            case DIV:
            case MOD:
                opLevel = TokenType.MUL;
                break;
            default:
        }

        // 我们的常量传播有5轮：
        // 第一轮：
        // 如果全部都是常量，那么很显然是可以直接传播的，计算便可。
        boolean isAllConst = true;
        for(Value value: values){
            isAllConst &= value instanceof LiteralConst;
        }
        if(isAllConst){
            while(values.size() != 1){
                Token opToken = opTokens.removeFirst();
                LiteralConst lc1 = (LiteralConst) values.removeFirst();
                lc1 = (LiteralConst) createTransInst(lc1, tarType, null);
                LiteralConst lc2 = (LiteralConst) values.removeFirst();
                lc2 = (LiteralConst) createTransInst(lc2, tarType, null);

                LiteralConst lc3 = doLCOp(lc1, lc2, opToken.wordType);
                lc3 = (LiteralConst) createTransInst(lc3, tarType, null);
                values.addFirst(lc3);
            }
            return;
        }
        // 这两个是进行了常量传播之后应该保留的那些内容。
        List<Value> restValues = new ArrayList<>();
        List<Token> restOpTokens = new ArrayList<>();
        // 第二轮：把+0, -0, *1,*0, /1, %1的操作全部传播了。
        int vsize = values.size(), osize = opTokens.size();
        assert osize == vsize - 1;

        BasicBlock currBlock = blockStack.getLast();
        for(int i = 0; i < vsize; i++){
            values.set(i, createTransInst(values.get(i), tarType, currBlock));
        }


        restValues.add(values.getFirst());
        for(int i = 1; i < vsize; i++){
            Value currValue = values.get(i);
            Token currOp = opTokens.get(i - 1);
            if(currValue instanceof LiteralConst lc){
                if((currOp.wordType == TokenType.ADD || currOp.wordType == TokenType.SUB) && lc.isInt() && lc.getIntValue() == 0){
                    continue;
                }
                if((currOp.wordType == TokenType.MUL || currOp.wordType == TokenType.DIV) && lc.isInt() && lc.getIntValue() == 1){
                    continue;
                }
                if((currOp.wordType == TokenType.MUL && lc.isInt() && lc.getIntValue() == 0)){
                    restValues.clear();
                    LiteralConst zero = createLiteralConst(0, IRBasicType.I32);
                    restValues.add(zero);
                    continue;
                }
                if(currOp.wordType == TokenType.MOD && lc.isInt() && lc.getIntValue() == 1){
                    restValues.clear();
                    LiteralConst zero = createLiteralConst(0, IRBasicType.I32);
                    restValues.add(zero);
                    continue;
                }
            }
            restValues.add(currValue);
            restOpTokens.add(currOp);
        }
        values.clear();
        values.addAll(restValues);
        opTokens.clear();
        opTokens.addAll(restOpTokens);
        restValues.clear();
        restOpTokens.clear();

        // 第三轮：对于+，-，我们可以任意改变参与运算的值的顺序；
        // 对于*，/, 我们可以改变连续的乘法参与运算的值的顺序.

        vsize = values.size();
        osize = opTokens.size();
        assert osize == vsize - 1;

        List<ValueTokenPair> valueTokenPairs = new ArrayList<>();
        valueTokenPairs.add(new ValueTokenPair(values.get(0), opLevel));
        for(int i = 1; i < vsize; i++){
            valueTokenPairs.add(new ValueTokenPair(values.get(i), opTokens.get(i - 1).wordType));
        }
        if(opLevel == TokenType.ADD){
            valueTokenPairs.sort(new ValueTokenPairComparator());
        }
        else if(opLevel == TokenType.MUL){
            List<ValueTokenPair> tempValueTokenPairs = new ArrayList<>();
            List<ValueTokenPair> mulValueTokenPairs = new ArrayList<>();
            List<ValueTokenPair> divValueTokenPairs = new ArrayList<>();
            for(int i = 0; i < vsize; i++){
                ValueTokenPair vtp = valueTokenPairs.get(i);
                if(vtp.getTokenType() == TokenType.MUL){
                    if(!divValueTokenPairs.isEmpty()){
                        divValueTokenPairs.sort(new ValueTokenPairComparator());
                        tempValueTokenPairs.addAll(divValueTokenPairs);
                        divValueTokenPairs.clear();
                    }
                    mulValueTokenPairs.add(vtp);
                }
                else if(vtp.getTokenType() == TokenType.DIV){
                    if(!mulValueTokenPairs.isEmpty()){
                        mulValueTokenPairs.sort(new ValueTokenPairComparator());
                        tempValueTokenPairs.addAll(mulValueTokenPairs);
                        mulValueTokenPairs.clear();
                    }
                    divValueTokenPairs.add(vtp);
                }
                else{
                    if(!mulValueTokenPairs.isEmpty()){
                        mulValueTokenPairs.sort(new ValueTokenPairComparator());
                        tempValueTokenPairs.addAll(mulValueTokenPairs);
                        mulValueTokenPairs.clear();
                    }
                    if(!divValueTokenPairs.isEmpty()){
                        divValueTokenPairs.sort(new ValueTokenPairComparator());
                        tempValueTokenPairs.addAll(divValueTokenPairs);
                        divValueTokenPairs.clear();
                    }
                    tempValueTokenPairs.add(vtp);
                }
            }
            if(!mulValueTokenPairs.isEmpty()){
                mulValueTokenPairs.sort(new ValueTokenPairComparator());
                tempValueTokenPairs.addAll(mulValueTokenPairs);
                mulValueTokenPairs.clear();
            }
            if(!divValueTokenPairs.isEmpty()){
                divValueTokenPairs.sort(new ValueTokenPairComparator());
                tempValueTokenPairs.addAll(divValueTokenPairs);
                divValueTokenPairs.clear();
            }
            valueTokenPairs.clear();
            valueTokenPairs.addAll(tempValueTokenPairs);
        }

        // 第四轮：基于第三轮的结果，可以保证对于任何一类运算符，其连续的区域的常量
        // 都在前面。
        List<ValueTokenPair> tempValueTokenPairs = new ArrayList<>();
        tempValueTokenPairs.add(valueTokenPairs.get(0));
        vsize = valueTokenPairs.size();
        for(int i = 1; i < vsize; i++){
            ValueTokenPair curVTP = valueTokenPairs.get(i);
            ValueTokenPair prevVTP = tempValueTokenPairs.getLast();
            if(prevVTP.getTokenType() != TokenType.MOD && prevVTP.getTokenType() == curVTP.getTokenType() && prevVTP.getValue() instanceof  LiteralConst lc1 && curVTP.getValue() instanceof LiteralConst lc2){
                TokenType csType = getOpTypeForCS(prevVTP.getTokenType());
                LiteralConst lc3 = doLCOp(lc1, lc2, csType);
                prevVTP.setValue(lc3);
            }
            else{
                tempValueTokenPairs.add(curVTP);
            }
        }
        valueTokenPairs.clear();
        valueTokenPairs.addAll(tempValueTokenPairs);
        tempValueTokenPairs.clear();

        // 第五轮：对于+和-可以直接进行运算，在减少一次。
        vsize = valueTokenPairs.size();
        tempValueTokenPairs.add(valueTokenPairs.get(0));
        for(int i = 1; i < vsize; i++){
            ValueTokenPair curVTP = valueTokenPairs.get(i);
            ValueTokenPair prevVTP = tempValueTokenPairs.getLast();
            if(opLevel == TokenType.ADD && prevVTP.getValue() instanceof  LiteralConst lc1 && curVTP.getValue() instanceof LiteralConst lc2){
                LiteralConst lc3 = doLCOp(lc1, lc2, TokenType.SUB);
                prevVTP.setValue(lc3);
            }
            else{
                tempValueTokenPairs.add(curVTP);
            }
        }
        valueTokenPairs.clear();
        valueTokenPairs.addAll(tempValueTokenPairs);
        tempValueTokenPairs.clear();
        // 第六轮：把所有可能的运算后为0的值给剔除掉
        vsize = valueTokenPairs.size();
        for(int i = 0; i < vsize; i++) {
            ValueTokenPair curVTP = valueTokenPairs.get(i);
            if (!(curVTP.getValue() instanceof LiteralConst lc && lc.isInt() && lc.getIntValue() == 0)) {
                tempValueTokenPairs.add(curVTP);
            }
        }

        values.clear();
        opTokens.clear();
        vsize = tempValueTokenPairs.size();
        ValueTokenPair fvtp = tempValueTokenPairs.get(0);
        if(opLevel == TokenType.ADD && fvtp.getTokenType() == TokenType.SUB){
            assert fvtp.getValue() instanceof LiteralConst;
            LiteralConst flc = (LiteralConst) fvtp.getValue();
            if(flc.isInt()){
                fvtp.setValue(createLiteralConst(-flc.getIntValue(), flc.getBasicType()));
            }
            else if(flc.isFloat()){
                fvtp.setValue(createLiteralConst(-flc.getFloatValue(), flc.getBasicType()));
            }
        }
        values.add(tempValueTokenPairs.get(0).getValue());
        for(int i = 1; i < vsize; i++){
            ValueTokenPair vtp = tempValueTokenPairs.get(i);
            values.add(vtp.getValue());
            opTokens.add(new Token(vtp.getTokenType(), ""));
        }
    }
    private void doCmpInstConstSpread(IRBasicType tarType, List<Value> values, List<Token> opTokens){
        if(values.size() == 1){
            return;
        }

        boolean isAllConst = true;
        for(Value value: values){
            isAllConst &= value instanceof LiteralConst;
        }
        if(isAllConst){
            while(values.size() != 1){
                Token opToken = opTokens.removeFirst();
                LiteralConst lc1 = (LiteralConst) values.removeFirst();
                lc1 = (LiteralConst) createTransInst(lc1, tarType, null);
                LiteralConst lc2 = (LiteralConst) values.removeFirst();
                lc2 = (LiteralConst) createTransInst(lc2, tarType, null);

                LiteralConst lc3 = doLCOp(lc1, lc2, opToken.wordType);
                lc3 = (LiteralConst) createTransInst(lc3, tarType, null);
                values.addFirst(lc3);
            }
            return;
        }

        List<Value> restValues = new ArrayList<>();
        List<Token> restOpTokens = new ArrayList<>();

        int vsize = values.size(), osize = opTokens.size();
        assert vsize == osize + 1;

        restValues.add(values.getFirst());
        for(int i = 1; i < vsize; i++){
            Value prevVal = restValues.getLast();
            Value currVal = values.get(i);
            Token opToken = opTokens.get(i - 1);
            if(prevVal instanceof LiteralConst lc1 && currVal instanceof LiteralConst lc2){
                LiteralConst lc3 = doLCOp(lc1, lc2, opToken.wordType);
                restValues.removeLast();
                restValues.add(lc3);
            }
            else{
                restValues.add(currVal);
                restOpTokens.add(opToken);
            }
        }

        values.clear();
        opTokens.clear();
        values.addAll(restValues);
        opTokens.addAll(restOpTokens);
    }

    LiteralConst doLCOp(LiteralConst v1, LiteralConst v2, TokenType tokenType) {
        assert v1.getBasicType() == v2.getBasicType();
        IRBasicType IRBasicType = v1.getBasicType();
        if (v2 == null) {
            Double val = v1.getNumber().doubleValue();
            if (tokenType == TokenType.ADD) return v1;
            else if (tokenType == TokenType.SUB) {
                return createLiteralConst(-val, IRBasicType);
            } else if (tokenType == TokenType.NOT) {
                Integer res = val == 0 ? 1 : 0;
                return createLiteralConst(res, IRBasicType);
            }
        } else {
            double val1 = v1.getNumber().doubleValue();
            double val2 = v2.getNumber().doubleValue();
            switch (tokenType) {
                case TokenType.ADD -> {
                    return createLiteralConst(val1 + val2, IRBasicType);
                }
                case TokenType.SUB -> {
                    return createLiteralConst(val1 - val2, IRBasicType);
                }
                case TokenType.MUL -> {
                    return createLiteralConst(val1 * val2, IRBasicType);
                }
                case TokenType.DIV -> {
                    return createLiteralConst(val1 / val2, IRBasicType);
                }
                case TokenType.MOD -> {
                    return createLiteralConst((int)val1 % (int)val2, IRBasicType);
                }
                case TokenType.LE -> {
                    Integer res = val1 <= val2 ? 1 : 0;
                    return createLiteralConst(res, IRBasicType);
                }
                case TokenType.LT -> {
                    Integer res = val1 < val2 ? 1 : 0;
                    return createLiteralConst(res, IRBasicType);
                }
                case TokenType.GE -> {
                    Integer res = val1 >= val2 ? 1 : 0;
                    return createLiteralConst(res, IRBasicType);
                }
                case TokenType.GT -> {
                    Integer res = val1 > val2 ? 1 : 0;
                    return createLiteralConst(res, IRBasicType);
                }
                case TokenType.NE -> {
                    Integer res = val1 != val2 ? 1 : 0;
                    return createLiteralConst(res, IRBasicType);
                }
                case TokenType.EQ -> {
                    Integer res = val1 == val2 ? 1 : 0;
                    return createLiteralConst(res, IRBasicType);
                }
            }
        }
        return null;
    }
}
