package frontend.ir;

import frontend.ir.constant.*;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.*;
import frontend.ir.instr.convop.*;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.instr.unaryop.FNegInstr;
import frontend.ir.objecttype.TVoid;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.*;
import frontend.ir.objecttype.derived.TArray;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.IRModel;
import frontend.ir.symbol.*;
import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.CompUnit;
import frontend.syntax.ast.nodes.block.Block;
import frontend.syntax.ast.nodes.block.IBlockItem;
import frontend.syntax.ast.nodes.block.IStmt;
import frontend.syntax.ast.nodes.declanddef.*;
import frontend.syntax.ast.nodes.expression.*;
import frontend.syntax.ast.nodes.expression.ASTNumber;

import java.util.*;

public class Visitor {
    private static final Visitor VISITOR = new Visitor();
    private BasicBlock curBB;
    private Function curFunc;
    private BasicBlock retBB;
    private Value retPtr;
    private Loop curLoop;

    private static final List<GlbObjPtr> globalVariables = new ArrayList<>();

    private Visitor() {
        curBB    = null;
        curFunc  = null;
        retBB    = null;
        retPtr   = null;
        curLoop  = null;
    }

    public static Visitor getInstance() {
        return VISITOR;
    }

    public static List<GlbObjPtr> getGlobalVariables() {return globalVariables;}

    private int getAndAddRegIdx() {
        return curFunc.getAndAddRegIdx();
    }

    private int getAndAddBlkIdx() {
        return curFunc.getAndAddBlkIdx();
    }

    private void renewCurBB(BasicBlock newBB) {
        curBB = newBB;
        curFunc.appendBasicBlock(curBB);
    }

    public IRModel visitAST(AST ast) {
        SymTab globalSymTab = new SymTab(null);
        ArrayList<Function> functions = new ArrayList<>();

        CompUnit compUnit = ast.compUnit();
        for (Decl decl : compUnit.getDeclList()) {
            visitDecl(decl, globalSymTab);
        }
        for (FuncDef funcDef : compUnit.getFuncDefList()) {
            functions.add(visitFuncDef(funcDef, globalSymTab));
        }
        functions.add(visitFuncDef(compUnit.getMainFuncDef(), globalSymTab));

        return new IRModel(functions, globalSymTab);
    }

    private Function visitFuncDef(FuncDef funcDef, SymTab globalSymTab) {
        Type returnType = switch (funcDef.getType()) {
            case VOID  -> new TVoid();
            case INT   -> new TInt();
            case FLOAT -> new TFloat();
        };
        Function function = new Function(returnType, funcDef.getIdent());
        globalSymTab.addFunc(function, funcDef.getLineno());

        initFunction(function);

        SymTab funcFParamSymTab = new SymTab(globalSymTab);
        for (FuncFParam astFParam : funcDef.getFParams()) {
            function.appendFParam(visitFuncFParam(astFParam, funcFParamSymTab));
        }
        SymTab funcSymTab = new SymTab(funcFParamSymTab);

        visitCodeBlk(funcDef.getBody(), funcSymTab);

        finalizeFunction(function);

        return function;
    }

    private void finalizeFunction(Function function) {
        if (curBB.isNotClosed()) {
            new JumpInstr(retBB, curBB);
        }

        renewCurBB(retBB);
        if (curFunc.getType() instanceof TVoid) {
            new ReturnInstr(null, curBB);
        } else {
            LoadInstr loadRetVal = new LoadInstr(getAndAddRegIdx(), function.getType(), retPtr, curBB);
            new ReturnInstr(loadRetVal, curBB);
        }
        function.rearrangeAlloca();

        curFunc = null;
        curBB   = null;
        retBB   = null;
        retPtr  = null;
    }

    private void initFunction(Function function) {
        curFunc = function;
        renewCurBB(new BasicBlock(getAndAddBlkIdx()));
        retBB = new BasicBlock(getAndAddBlkIdx());

        Type retType = function.getType();
        if (retType instanceof ArithmeticType) {
            retPtr = new AllocaInstr(retType, getAndAddRegIdx(), curBB);
            checkAndStore(IntConst.Zero, retPtr);
        } else if (retType instanceof TVoid) {
            retPtr = null;
        } else {
            throw new RuntimeException("奇怪的函数类型");
        }
    }

    private void visitCodeBlk(Block block, SymTab symTab) {
        for (IBlockItem blockItem : block.getBlockItems()) {
            if (blockItem instanceof Decl) {
                visitDecl((Decl) blockItem, symTab);
            } else if (blockItem instanceof IStmt) {
                visitStmt((IStmt) blockItem, symTab);
            } else {
                throw new RuntimeException("这是什么 item");
            }
        }
    }

    private void visitStmt(IStmt stmt, SymTab symTab) {
        if (stmt instanceof IStmt.AssignStmt) {
            visitAssignStmt((IStmt.AssignStmt) stmt, symTab);
        } else if (stmt instanceof IStmt.ExpStmt) {
            visitExpStmt((IStmt.ExpStmt) stmt, symTab);
        } else if (stmt instanceof Block) {
            visitCodeBlk((Block) stmt, new SymTab(symTab));
        } else if (stmt instanceof IStmt.IfStmt) {
            visitIfStmt((IStmt.IfStmt) stmt, symTab);
        } else if (stmt instanceof IStmt.WhileLoopStmt) {
            visitWhileLoopStmt((IStmt.WhileLoopStmt) stmt, symTab);
        } else if (stmt instanceof IStmt.BreakStmt) {
            visitBreakStmt((IStmt.BreakStmt) stmt);
        } else if (stmt instanceof IStmt.ContinueStmt) {
            visitContinueStmt((IStmt.ContinueStmt) stmt);
        } else if (stmt instanceof IStmt.ReturnStmt) {
            visitReturnStmt((IStmt.ReturnStmt) stmt, symTab);
        } else {
            throw new RuntimeException("这是什么语句");
        }
    }

    private void visitContinueStmt(IStmt.ContinueStmt stmt) {
        if (curLoop == null) {
            throw new RuntimeException("USING CONTINUE IN NON LOOP BLOCKS at line" + stmt.getLineno());
        } else {
            new JumpInstr(curLoop.getCondBegin(), curBB);
        }
    }

    private void visitBreakStmt(IStmt.BreakStmt stmt) {
        if (curLoop == null) {
            throw new RuntimeException("USING BREAK IN NON LOOP BLOCKS at line" + stmt.getLineno());
        } else {
            new JumpInstr(curLoop.getExiting(), curBB);
        }
    }

    private void visitWhileLoopStmt(IStmt.WhileLoopStmt stmt, SymTab symTab) {
        BasicBlock condBegin = new BasicBlock(getAndAddBlkIdx());
        BasicBlock bodyBegin = new BasicBlock(getAndAddBlkIdx());
        BasicBlock exiting   = new BasicBlock(getAndAddBlkIdx());

        final Loop outerLoop = curLoop;
        curLoop = new Loop(exiting);
        new JumpInstr(condBegin, curBB);

        renewCurBB(condBegin);
        IExp cond = stmt.getCond();
        if (cond == null) {
            new BranchInstr(BoolConst.One, bodyBegin, exiting, curBB);  // 这个对于 while 循环似乎是不可能触发的
        } else {
            Value condition = evaluateLOR(cond, bodyBegin, exiting, symTab);
            new BranchInstr(condition, bodyBegin, exiting, curBB);  // 这里 curBB 不一定是 condBegin 了
        }
        curLoop.renewCondition(condBegin.getSubListTo(curBB));    // 从 condBegin 到 curBB

        renewCurBB(bodyBegin);
        visitStmt(stmt.getBody(), symTab);
        new JumpInstr(condBegin, curBB);

        renewCurBB(exiting);
        BasicBlock succeed = new BasicBlock(getAndAddBlkIdx());
        new JumpInstr(succeed, curBB);
        renewCurBB(succeed);

        curLoop = outerLoop;
    }

    /**
     * 定位对象，准备对它赋值或者求值，因此可能出现未定义符号或者对常量赋值的情况
     */
    private Value locateObject(LVal lVal, SymTab symTab, boolean toWrite) {
        Symbol symbol;
        symbol = symTab.getObject(lVal.getIdent(), lVal.getLineno());

        if (toWrite && symbol.isConst()) {
            throw new RuntimeException("MODIFYING CONST VALUE at line " + lVal.getLineno());
        }

        Value pointer = symbol.getPointer();
        Type type = symbol.getType();
        if (type instanceof TArray) {
            List<IExp> idxExpList = lVal.getIndexList();
            List<Value> idxList = new ArrayList<>();
            idxList.add(IntConst.Zero);
            if (idxExpList.isEmpty()) {
                idxList.add(IntConst.Zero);
            } else {
                for (IExp exp : idxExpList) {
                    idxList.add(evaluate(exp, symTab));
                }
            }

            pointer = new GEPInstr(getAndAddRegIdx(), idxList, pointer, curBB);
        } else if (type instanceof TPointer) {
            List<IExp> idxExpList = lVal.getIndexList();
            List<Value> idxList = new ArrayList<>();
            if (idxExpList.isEmpty()) {
                idxList.add(IntConst.Zero);
            } else {
                for (IExp exp : idxExpList) {
                    idxList.add(evaluate(exp, symTab));
                }
            }

            Type pointerType = pointer.getType();
            if (!(pointerType instanceof TPointer ptr)) {
                throw new RuntimeException("指针的类型必须是指针类型");
            }

            Type ref = ptr.getReferencedType();
            pointer = new LoadInstr(getAndAddRegIdx(), ref, pointer, curBB);
            pointer = new GEPInstr(getAndAddRegIdx(), idxList, pointer, curBB);
        }

        return pointer;
    }

    private void visitIfStmt(IStmt.IfStmt stmt, SymTab symTab) {
        boolean hasElse = stmt.getElseStmt() != null;

        BasicBlock thenBlk = new BasicBlock(getAndAddBlkIdx());
        BasicBlock elseBlk = new BasicBlock(getAndAddBlkIdx());
        BasicBlock succeed = hasElse ? new BasicBlock(getAndAddBlkIdx()) : elseBlk;

        Value condition = evaluateLOR(stmt.getCond(), thenBlk, elseBlk, symTab);
        new BranchInstr(condition, thenBlk, elseBlk, curBB);

        renewCurBB(thenBlk);
        visitStmt(stmt.getThenStmt(), symTab);
        new JumpInstr(succeed, curBB);

        if (hasElse) {
            renewCurBB(elseBlk);
            visitStmt(stmt.getElseStmt(), symTab);
            new JumpInstr(succeed, curBB);
        }

        renewCurBB(succeed);
    }

    /**
     * 一真皆真
     */
    private Value evaluateLOR(IExp exp, BasicBlock trueTarBlk, BasicBlock falseTarBlk, SymTab symTab) {
        if (!(exp instanceof BinaryExp cond)) {
            throw new RuntimeException("条件一定是多元表达式");
        }

        BasicBlock nextBlk;
        Value condValue;

        if (cond.getOps().isEmpty()) {
            nextBlk = falseTarBlk;
        } else {
            nextBlk = new BasicBlock(getAndAddBlkIdx());
        }
        condValue = anything2bool(evaluateLAND(cond.getFirstExp(), nextBlk, symTab));

        List<AST.OperatorName> ops = cond.getOps();
        List<IExp> restExps = cond.getRestExps();
        for (int i = 0; i < ops.size(); i++) {
            if (ops.get(i) != AST.OperatorName.LOR) {
                throw new RuntimeException("这里只能有 ||");
            }

            IExp nextExp = restExps.get(i);
            new BranchInstr(condValue, trueTarBlk, nextBlk, curBB);
            renewCurBB(nextBlk);

            if (i == ops.size() - 1) {
                nextBlk = falseTarBlk;
            } else {
                nextBlk = new BasicBlock(getAndAddBlkIdx());
            }
            condValue = anything2bool(evaluateLAND(nextExp, nextBlk, symTab));
        }
        return condValue;
    }

    /**
     * 一假皆假
     */
    private Value evaluateLAND(IExp exp, BasicBlock falseTarBlk, SymTab symTab) {
        if (!(exp instanceof BinaryExp cond)) {
            throw new RuntimeException("条件一定是多元表达式");
        }

        BasicBlock nextBlk;
        Value condValue = anything2bool(evaluate(cond.getFirstExp(), symTab));

        List<AST.OperatorName> ops = cond.getOps();
        List<IExp> restExps = cond.getRestExps();
        for (int i = 0; i < ops.size(); i++) {
            if (ops.get(i) != AST.OperatorName.LAND) {
                throw new RuntimeException("这里只能有 &&");
            }
            nextBlk = new BasicBlock(getAndAddBlkIdx());
            new BranchInstr(condValue, nextBlk, falseTarBlk, curBB);
            renewCurBB(nextBlk);

            IExp nextExp = restExps.get(i);
            condValue = anything2bool(evaluate(nextExp, symTab));
        }

        return condValue;
    }

    private Value anything2bool(Value value) {
        Type inputType = value.getType();
        if (inputType instanceof TBool) {
            return value;
        } else if (inputType instanceof TChar) {
            return new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.INE, value, CharConst.Zero, curBB);
        } else if (inputType instanceof TInt) {
            return new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.INE, value, IntConst.Zero, curBB);
        } else if (inputType instanceof TFloat) {
            return new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FNE, value, FloatConst.Zero, curBB);
        } else {
            throw new RuntimeException("这是什么类型");
        }
    }

    private void visitAssignStmt(IStmt.AssignStmt stmt, SymTab symTab) {
        Value pointer = locateObject(stmt.getlVal(), symTab, true);
        if (pointer == null) {
            return;
        }

        Value value = evaluate(stmt.getExp(), symTab);

        checkAndStore(value, pointer);
    }

    private void visitReturnStmt(IStmt.ReturnStmt stmt, SymTab symTab) {
        IExp retExp = stmt.getRetVal();
        if (retExp != null) {
            if (curFunc.getType() instanceof TVoid) {
                throw new RuntimeException("UNMATCHED RET STMT IN VOID FUNC at line " + stmt.getLineno());
            } else {
                Value returnVal = evaluate(retExp, symTab);
                checkAndStore(returnVal, retPtr);
            }
        }
        new JumpInstr(retBB, curBB);
    }

    private void visitExpStmt(IStmt.ExpStmt stmt, SymTab symTab) {
        IExp exp = stmt.getExp();
        if (exp == null) {
            return;
        }
        evaluate(exp, symTab);
    }

    private void checkAndStore(Value toStore, Value pointer) {
        Type pointerType = pointer.getType();
        if (!(pointerType instanceof TPointer)) {
            throw new RuntimeException("指针的类型必须是指针类型");
        }
        Type expected = ((TPointer) pointerType).getReferencedType();
        Type provided = toStore.getType();

        if (expected instanceof TChar && provided instanceof TInt) {
            toStore = new TruncInstr(getAndAddRegIdx(), toStore, curBB);
        } else if (expected instanceof TInt && provided instanceof TChar) {
            toStore = new ZextInstr(getAndAddRegIdx(), toStore, curBB);
        } else if (expected instanceof TInt && provided instanceof TFloat) {
            toStore = new Fp2Si(getAndAddRegIdx(), toStore, curBB);
        } else if (expected instanceof TFloat && provided instanceof TInt) {
            toStore = new Si2Fp(getAndAddRegIdx(), toStore, curBB);
        } else if (!expected.equals(provided)) {
            throw new RuntimeException("Store failed!" +
                    " expected: " + expected.printIRType() + " provided: " + provided.printIRType());
        }
        new StoreInstr(toStore, pointer, curBB);
    }

    private Function.FParam visitFuncFParam(FuncFParam astFParam, SymTab symTab) {
        String name = astFParam.getIdent();
        Type type = ArithmeticType.bType2AriType(astFParam.getType());

        List<IExp> limList = astFParam.getLimList();
        if (limList != null) {
            // limList 非空表示是数组
            for (int i = limList.size() - 1; i >= 0; i--) {
                try {
                    type = new TArray(type, evaluateIntConst(limList.get(i), symTab).intValue());
                } catch (FloatOccursException e) {
                    throw new RuntimeException(e);
                }
            }
            type = new TPointer(type);
        }
        Function.FParam fParam = new Function.FParam(type, getAndAddRegIdx());

        AllocaInstr pointer = new AllocaInstr(type, getAndAddRegIdx(), curBB);
        checkAndStore(fParam, pointer);

        symTab.addObject(new Symbol(name, false, false, type, fParam, pointer), astFParam.getLineno());
        return fParam;
    }

    private void visitDecl(Decl decl, SymTab symTab) {
        boolean isConst  = decl.isConstant();
        boolean isGlobal = symTab.isGlobal();
        AST.BType bType  = decl.getbType();
        for (Def def : decl.getDefs()) {
            String ident = def.getIdent();

            List<Integer> limList = new ArrayList<>();
            for (IExp exp : def.getLimitList()) {
                Integer lim;
                try {
                    lim = evaluateIntConst(exp, symTab).intValue();
                } catch (FloatOccursException e) {
                    throw new RuntimeException(e);
                }
                limList.add(lim);
            }

            Type type = ArithmeticType.bType2AriType(bType);
            for (int i = limList.size() - 1; i >= 0; i--) {
                type = new TArray(type, limList.get(i));
            }

            IInitVal astInitVal = def.getInitVal();
            Value initVal;
            if (astInitVal == null && isGlobal) {
                if (type instanceof TArray) {
                    astInitVal = new InitArray(new ArrayList<>(), -1);
                    initVal = visitInitVal(isConst, astInitVal, limList, type, symTab);
                } else if (type instanceof TInt) {
                    initVal = IntConst.Zero;
                } else if (type instanceof TChar) {
                    initVal = CharConst.Zero;
                } else if (type instanceof TFloat) {
                    initVal = FloatConst.Zero;
                } else {
                    throw new RuntimeException("应该没有别的类型了");
                }
            } else {
                initVal = visitInitVal(isConst, astInitVal, limList, type, symTab);
            }
            Value pointer;
            if (isGlobal) {
                pointer = new GlbObjPtr(type, ident);
                this.globalVariables.add((GlbObjPtr) pointer);
            } else {
                // 如果是数组的话存一下初始化指令序列和初始值，方便后续优化（局部数组全局化）
                if (type instanceof TArray && initVal != null) {
                    pointer = new AllocaInstr.ArrayAlloca(type, getAndAddRegIdx(), curBB, initVal);
                    Instruction beforeEndIns = curBB.getLastInstr();
                    initArrayWithMemset((AllocaInstr.ArrayAlloca) pointer);
                    storeInitVal(initVal, pointer, true);
                    ((AllocaInstr.ArrayAlloca) pointer).setInitBeginIns((Instruction) beforeEndIns.getNext());
                    ((AllocaInstr.ArrayAlloca) pointer).setInitEndIns(curBB.getLastInstr());
                } else {
                    pointer = new AllocaInstr(type, getAndAddRegIdx(), curBB);
                    if (initVal != null) {
                        storeInitVal(initVal, pointer, false);
                    }
                }
            }
            symTab.addObject(new Symbol(ident, isConst, isGlobal, type, initVal, pointer), def.getLineno());
        }
    }

    private void initArrayWithMemset(AllocaInstr.ArrayAlloca pointer) {
        Type referencedType = pointer.getReferencedType();
        if (!(referencedType instanceof TArray)) {
            throw new RuntimeException("这里只能处理数组初始化");
        }
        int memSize = ((TArray) referencedType).getFullSize() * 4;  // i32 和 float 都是 4 字节

        List<Value> baseIndexList = new ArrayList<>();
        baseIndexList.add(new IntConst(0));
        while (referencedType instanceof TArray arrayType) {
            referencedType = arrayType.getElementType();
            baseIndexList.add(new IntConst(0));
        }

        GEPInstr toBase = new GEPInstr(getAndAddRegIdx(), baseIndexList, pointer, curBB);
        Bitcast toI8 = new Bitcast(getAndAddRegIdx(), toBase, curBB);

        List<Value> rParamList = Arrays.asList(toI8, new IntConst(0), new IntConst(memSize));
        new CallInstr(null, Function.LibFunc.MEMSET, rParamList, curBB);
    }

    /**
     * @param fromArray 为 true 表示是在存储数组初值，此前已经进行过 memset，不需要存储 0
     */
    private void storeInitVal(Value init, Value pointer, boolean fromArray) {
        if (init instanceof ArrayInitVal) {
            List<Value> initValues = ((ArrayInitVal) init).getInitValues();
            for (int i = 0; i < initValues.size(); i++) {
                Value value = initValues.get(i);

                if (fromArray && value instanceof IntConst intConst && intConst.getConstInt() == 0) {
                    continue;
                }
                if (fromArray && value instanceof FloatConst floatConst && Math.abs(floatConst.getConstVal().floatValue()) < 1e-6f) {
                    continue;
                }
                if (fromArray && value instanceof ArrayInitVal.ArrayZeroInitVal) {
                    continue;
                }

                List<Value> indexList = Arrays.asList(IntConst.Zero, new IntConst(i));
                GEPInstr elementPtr = new GEPInstr(getAndAddRegIdx(), indexList, pointer, curBB);
                storeInitVal(value, elementPtr, true);
            }
        } else {
            checkAndStore(init, pointer);
        }
    }

    private Value visitInitVal(boolean isConst, IInitVal initVal, List<Integer> limList, Type type, SymTab symTab) {
        if (initVal == null) {
            return null;
        }
        if (initVal instanceof IExp) {
            if (isConst || symTab.isGlobal()) {
                if (type instanceof TInt) {
                    try {
                        return new IntConst(evaluateIntConst((IExp) initVal, symTab));
                    } catch (FloatOccursException e) {
                        return new IntConst(evaluateFloatConst((IExp) initVal, symTab).intValue());
                    }
                } else if (type instanceof TFloat) {
                    return new FloatConst(evaluateFloatConst((IExp) initVal, symTab));
                } else {
                    throw new RuntimeException("处理初值时出现了不应该出现的类型：" + type.printIRType());
                }
            } else {
                return evaluate((IExp) initVal, symTab);
            }
        } else if (initVal instanceof InitArray) {
            List<Value> initValues = new ArrayList<>();

            if (!(type instanceof TArray arrayType)) {
                throw new RuntimeException("初始值列表只能用于给数组赋值");
            }

            List<IInitVal> initValList = InitArray.completeInit(arrayType.getLimList(), ((InitArray) initVal));
            for (IInitVal inner : initValList) {
                List<Integer> innerLimList = limList.subList(1, limList.size());
                Value innerVal = visitInitVal(isConst, inner, innerLimList, arrayType.getElementType(), symTab);
                initValues.add(innerVal);
            }

            if (initValues.isEmpty()) {
                return new ArrayInitVal.ArrayZeroInitVal(arrayType);
            }

            boolean isZero = true;
            for (Value val : initValues) {
                if (val instanceof ArrayInitVal.ArrayZeroInitVal) {
                    continue;
                }
                if (val instanceof IntConst intConst && intConst.getConstInt() == 0) {
                    continue;
                }
                if (val instanceof FloatConst floatConst && Math.abs(floatConst.getConstVal().floatValue()) < 1e-6f) {
                    continue;
                }
                isZero = false;
                break;
            }
            if (isZero) {
                return new ArrayInitVal.ArrayZeroInitVal(arrayType);
            }

            return new ArrayInitVal((TArray) type, initValues);
        } else {
            throw new RuntimeException("不应该出现的初始值类型");
        }
    }

    private Value evaluate(IExp exp, SymTab symTab) {
        if (exp instanceof BinaryExp) {
            Value curValue           = evaluate(((BinaryExp) exp).getFirstExp(), symTab);
            List<AST.OperatorName> ops = ((BinaryExp) exp).getOps();
            List<IExp> restExps        = ((BinaryExp) exp).getRestExps();

            for (int i = 0; i < ops.size(); i++) {
                AST.OperatorName op = ops.get(i);
                Value nxtValue = evaluate(restExps.get(i), symTab);

                curValue = integralPromotion(curValue); // 整型全变成 i32

                Type curType = curValue.getType();
                Type nxtType = nxtValue.getType();
                if (curType instanceof TFloat && nxtType instanceof TInt) {
                    nxtValue = new Si2Fp(getAndAddRegIdx(), nxtValue, curBB);
                } else if (nxtType instanceof TFloat && curType instanceof TInt) {
                    curValue = new Si2Fp(getAndAddRegIdx(), curValue, curBB);
                }

                curValue = switch (op) {
                    case ADD -> curValue.getType() instanceof TFloat ?
                            new FAddInstr(getAndAddRegIdx(), curValue, nxtValue, curBB):
                            new AddInstr(getAndAddRegIdx(), curValue, nxtValue, curBB);
                    case SUB -> curValue.getType() instanceof TFloat ?
                            new FSubInstr(getAndAddRegIdx(), curValue, nxtValue, curBB):
                            new SubInstr(getAndAddRegIdx(), curValue, nxtValue, curBB);
                    case MUL -> curValue.getType() instanceof TFloat ?
                            new FMulInstr(getAndAddRegIdx(), curValue, nxtValue, curBB):
                            new MulInstr(getAndAddRegIdx(), curValue, nxtValue, curBB);
                    case DIV -> curValue.getType() instanceof TFloat ?
                            new FDivInstr(getAndAddRegIdx(), curValue, nxtValue, curBB):
                            new SDivInstr(getAndAddRegIdx(), curValue, nxtValue, curBB);
                    case MOD -> curValue.getType() instanceof TFloat ?
                            new FRemInstr(getAndAddRegIdx(), curValue, nxtValue, curBB):
                            new SRemInstr(getAndAddRegIdx(), curValue, nxtValue, curBB);
                    case EQ  -> curValue.getType() instanceof TFloat ?
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FEQ, curValue, nxtValue, curBB):
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.IEQ, curValue, nxtValue, curBB);
                    case NE  -> curValue.getType() instanceof TFloat ?
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FNE, curValue, nxtValue, curBB):
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.INE, curValue, nxtValue, curBB);
                    case GT  -> curValue.getType() instanceof TFloat ?
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FGT, curValue, nxtValue, curBB):
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.IGT, curValue, nxtValue, curBB);
                    case GE  -> curValue.getType() instanceof TFloat ?
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FGE, curValue, nxtValue, curBB):
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.IGE, curValue, nxtValue, curBB);
                    case LT  -> curValue.getType() instanceof TFloat ?
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FLT, curValue, nxtValue, curBB):
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.ILT, curValue, nxtValue, curBB);
                    case LE  -> curValue.getType() instanceof TFloat ?
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FLE, curValue, nxtValue, curBB):
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.ILE, curValue, nxtValue, curBB);
                    default  -> throw new RuntimeException("未曾设想的运算符： " + op);
                };
            }
            return integralPromotion(curValue);
        } else if (exp instanceof UnaryExp) {
            Value res;
            int sign = ((UnaryExp) exp).getSign();
            IPrimaryExp primaryExp = ((UnaryExp) exp).getPrimaryExp();
            if (primaryExp instanceof IExp) {
                res = evaluate((IExp) primaryExp, symTab);
            } else if (primaryExp instanceof ASTNumber.IntNumber) {
                res = new IntConst(((ASTNumber.IntNumber) primaryExp).getConstNumber());
            } else if (primaryExp instanceof ASTNumber.FloatNumber) {
                res = new FloatConst(((ASTNumber.FloatNumber) primaryExp).getConstNumber());
            } else if (primaryExp instanceof Call) {
                res = dealWithCall((Call) primaryExp, symTab);
            } else if (primaryExp instanceof LVal) {
                res = evaluateLVal((LVal) primaryExp, symTab);
            } else {
                throw new RuntimeException("这是基本表达式吗？");
            }

            // 整数统统转成 int 处理
            res = integralPromotion(res);

            // 一旦存在 NOT，则这个 UnaryExp 最后的值只能是 0, 1, -1 三种情况了。
            if (((UnaryExp) exp).checkExistingNot()) {
                if (((UnaryExp) exp).checkFinalNot()) {
                    res = res.getType() instanceof TFloat ?
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FEQ, FloatConst.Zero, res, curBB) :
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.IEQ, IntConst.Zero, res, curBB);
                } else {
                    res = res.getType() instanceof TFloat ?
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.FNE, FloatConst.Zero, res, curBB) :
                            new CmpInstr(getAndAddRegIdx(), CmpInstr.CmpCond.INE, IntConst.Zero, res, curBB);
                }
                if (sign == -1) {
                    res = new ZextInstr(getAndAddRegIdx(), res, curBB);
                }
            }
            if (sign == -1) {
                if (res instanceof IntConst con) {
                    res = new IntConst(con.getConstInt() * -1);
                } else if (res instanceof FloatConst con) {
                    res = new FloatConst(con.getConstVal().floatValue() * -1);
                } else if (res.getType() instanceof TFloat) {
                    res = new FNegInstr(getAndAddRegIdx(), res, curBB);
                } else {
                    res = new SubInstr(getAndAddRegIdx(), IntConst.Zero, res, curBB);
                }
            }

            return res;
        } else {
            throw new RuntimeException("奇怪的表达式");
        }
    }

    private Value evaluateLVal(LVal lVal, SymTab symTab) {
        Value ptr = locateObject(lVal, symTab, false);

        if (ptr == null) {
            return IntConst.Zero;
        }

        Symbol symbol;
        symbol = symTab.getObject(lVal.getIdent(), lVal.getLineno());

        if (ptr.getType() instanceof TPointer ptrType) {
            if (symbol.getType() instanceof TArray arrayType && arrayType.getDim() > lVal.getIndexList().size()) {
                return ptr;
            }
            if (symbol.getType() instanceof TPointer innerPtr) {
                int dim = 1;
                if (innerPtr.getReferencedType() instanceof TArray innerArr) {
                    dim += innerArr.getDim();
                }
                if (dim > lVal.getIndexList().size()) {
                    return ptr;
                }
            }
            return new LoadInstr(getAndAddRegIdx(), ptrType.getReferencedType(), ptr, curBB);
        } else {
            throw new RuntimeException("指针必须是指针类型");
        }

    }

    private Value dealWithCall(Call call, SymTab symTab) {
        ArrayList<Value> rParams = new ArrayList<>();
        for (IExp rParam : call.getRParams()) {
            rParams.add(evaluate(rParam, symTab));
        }
        String funcName = call.getIdent();
        Function callee = Function.LibFunc.getLibFunc(funcName);

        if (callee == Function.LibFunc.PUTF) {
            // todo: 这里其实可以做一些类型检查，但是这个函数一直没用，先不做了
            return new CallInstr(null, callee, rParams, curBB);
        } else if (callee == Function.LibFunc.STARTTIME || callee == Function.LibFunc.STOPTIME) {
            return new CallInstr(null, callee, List.of(new IntConst(call.getLineno())), curBB);
        }
        if (callee == null) {
            callee = symTab.getFunction(funcName, call.getLineno());
        }

        checkAndConvParams(callee.getFParams(), rParams, call.getLineno());

        CallInstr callInstr;
        if (callee.getType() instanceof TVoid) {
            callInstr = new CallInstr(null, callee, rParams, curBB);
        } else {
            callInstr = new CallInstr(getAndAddRegIdx(), callee, rParams, curBB);
        }
        return callInstr;
    }

    private void checkAndConvParams(List<Function.FParam> fParams, List<Value> rParams, int lineno) {
        if (fParams.size() != rParams.size()) {
            throw new RuntimeException("FUNC PARAM NUM NOT MATCH at line " + lineno);
        }
        for (int i = 0; i < fParams.size(); i++) {
            Function.FParam fParam = fParams.get(i);
            Value rParam = rParams.get(i);

            if (fParam.getType() instanceof TChar && rParam.getType() instanceof TInt) {
                rParam = new TruncInstr(getAndAddRegIdx(), rParam, curBB);
                rParams.set(i, rParam);
            } else if (fParam.getType() instanceof TInt && rParam.getType() instanceof TChar) {
                rParam = new ZextInstr(getAndAddRegIdx(), rParam, curBB);
                rParams.set(i, rParam);
            } else if (fParam.getType() instanceof TInt && rParam.getType() instanceof TFloat) {
                rParam = new Fp2Si(getAndAddRegIdx(), rParam, curBB);
                rParams.set(i, rParam);
            } else if (fParam.getType() instanceof TFloat && rParam.getType() instanceof TInt) {
                rParam = new Si2Fp(getAndAddRegIdx(), rParam, curBB);
                rParams.set(i, rParam);
            } else if (fParam.getType() instanceof TPointer fPtr
                    && rParam.getType() instanceof TPointer rPtr
                    && rPtr.getReferencedType() instanceof TArray rInnerArr
                    && rInnerArr.getElementType().equals(fPtr.getReferencedType())) {
                List<Value> idxList = Arrays.asList(IntConst.Zero, IntConst.Zero);
                rParam = new GEPInstr(getAndAddRegIdx(), idxList, rParam, curBB);
                rParams.set(i, rParam);
            }

            if (!fParam.getType().equals(rParam.getType())) {
                throw new RuntimeException("FUNC PARAM NUM NOT MATCH at line " + lineno +
                        ", fType: " + fParam.getType() + "; rType: " + rParam.getType());
            }
        }
    }

    /**
     * 整型提升：将 char，bool 等全都转换成 int
     * 只对这两种类型进行转换，其它的照样返回就行
     */
    private Value integralPromotion(Value from) {
        Type type = from.getType();
        if (type instanceof TChar || type instanceof TBool) {
            return new ZextInstr(getAndAddRegIdx(), from, curBB);
        } else {
            return from;
        }
    }

    /**
     * 这个方法仅用于求常量表达式的值，包括 global object 和 const object 的初值，以及数组各维度的长度
     * 这个方法严格限制所有参与计算的元素必须全是整型，一旦出现浮点数立刻抛异常，由上级决定报错或者转为浮点计算
     * 这里不会涉及逻辑运算，返回值也一定是整型，之后按需要做截断
     */
    private Long evaluateIntConst(IExp exp, SymTab symTab) throws FloatOccursException {
        if (exp == null) {
            return null;
        }
        if (exp instanceof BinaryExp) {
            long res = evaluateIntConst(((BinaryExp) exp).getFirstExp(), symTab);
            List<AST.OperatorName> ops = ((BinaryExp) exp).getOps();
            List<IExp> restExps = ((BinaryExp) exp).getRestExps();

            for (int i = 0; i < ops.size(); i++) {
                AST.OperatorName op = ops.get(i);
                IExp  rightExp = restExps.get(i);
                long  rightNum = evaluateIntConst(rightExp, symTab);
                res = switch (op) {
                    case ADD -> res + rightNum;
                    case SUB -> res - rightNum;
                    case MUL -> res * rightNum;
                    case DIV -> res / rightNum;
                    case MOD -> res % rightNum;
                    default -> throw new RuntimeException("这个符号不应该出现在常量表达式里");
                };
            }

            return res;
        } else if (exp instanceof UnaryExp) {
            long res;
            int sign = ((UnaryExp) exp).getSign();
            IPrimaryExp primaryExp = ((UnaryExp) exp).getPrimaryExp();
            if (primaryExp instanceof IExp) {
                res = evaluateIntConst((IExp) primaryExp, symTab);
            } else if (primaryExp instanceof Call) {
                throw new RuntimeException("常量表达式计算不应该出现函数调用");
            } else if (primaryExp instanceof ASTNumber.IntNumber) {
                res = ((ASTNumber.IntNumber) primaryExp).getConstNumber();
            } else if (primaryExp instanceof ASTNumber.FloatNumber) {
                throw new FloatOccursException(((UnaryExp) exp).getLineno());
            } else if (primaryExp instanceof LVal) {
                res = evaluateConstIntLVal((LVal) primaryExp, symTab);
            } else {
                throw new RuntimeException("奇怪的基本表达式");
            }

            res *= sign;
            if (((UnaryExp) exp).checkExistingNot()) { throw new RuntimeException("我觉得这里不应该有 NOT"); }

            return res;
        } else {
            throw new RuntimeException("这什么表达式啊");
        }
    }

    private int evaluateConstIntLVal(LVal lVal, SymTab symTab) throws FloatOccursException {
        Symbol symbol;
        symbol = symTab.getObject(lVal.getIdent(), lVal.getLineno());

        Type symType = symbol.getType();
        if (symType instanceof TFloat || symType instanceof TArray arrType && arrType.getBaseElementType() instanceof TFloat) {
            throw new FloatOccursException(lVal.getLineno());
        }

        if (symbol.isConst() || symbol.isGlobal() && symTab.isGlobal()) {
            Value initVal = symbol.getInitVal();
            if (symbol.getType() instanceof TArray) {
                List<Integer> indexList = new ArrayList<>();
                for (IExp exp : lVal.getIndexList()) {
                    try {
                        indexList.add(evaluateIntConst(exp, symTab).intValue());
                    } catch (FloatOccursException e) {
                        throw new RuntimeException(e);
                    }
                }

                if (initVal instanceof ArrayInitVal) {
                    Value value = ((ArrayInitVal) initVal).getValueWithIndexList(indexList);
                    if (value instanceof IntConst intCon) {
                        return intCon.getConstInt().intValue();
                    } else {
                        throw new RuntimeException("数组里这个值好像不是整型常值");
                    }
                } else {
                    throw new RuntimeException("数组初值应该是数组初值");
                }
            } else {
                if (initVal instanceof IntConst) {
                    return ((IntConst) initVal).getConstInt().intValue();
                } else {
                    throw new RuntimeException("这个左值没有确定初始值哦~  " + lVal.getIdent());
                }
            }
        } else {
            throw new RuntimeException("这个左值没有确定值哦~  " + lVal.getIdent());
        }
    }

    private Float evaluateFloatConst(IExp exp, SymTab symTab) {
        if (exp == null) {
            return null;
        }
        if (exp instanceof BinaryExp) {
            float res = evaluateFloatConst(((BinaryExp) exp).getFirstExp(), symTab);
            List<AST.OperatorName> ops = ((BinaryExp) exp).getOps();
            List<IExp> restExps = ((BinaryExp) exp).getRestExps();

            for (int i = 0; i < ops.size(); i++) {
                AST.OperatorName op = ops.get(i);
                IExp  rightExp = restExps.get(i);
                float rightNum = evaluateFloatConst(rightExp, symTab);
                res = switch (op) {
                    case ADD -> res + rightNum;
                    case SUB -> res - rightNum;
                    case MUL -> res * rightNum;
                    case DIV -> res / rightNum;
                    case MOD -> res % rightNum;
                    default -> throw new RuntimeException("这个符号不应该出现在常量表达式里");
                };
            }

            return res;
        } else if (exp instanceof UnaryExp) {
            float res;
            int sign = ((UnaryExp) exp).getSign();
            IPrimaryExp primaryExp = ((UnaryExp) exp).getPrimaryExp();
            if (primaryExp instanceof IExp) {
                res = evaluateFloatConst((IExp) primaryExp, symTab);
            } else if (primaryExp instanceof Call) {
                throw new RuntimeException("常量表达式计算不应该出现函数调用");
            } else if (primaryExp instanceof ASTNumber.IntNumber) {
                res = ((ASTNumber.IntNumber) primaryExp).getConstNumber();
            } else if (primaryExp instanceof ASTNumber.FloatNumber) {
                res = ((ASTNumber.FloatNumber) primaryExp).getConstNumber();
            } else if (primaryExp instanceof LVal) {
                res = evaluateConstFloatLVal((LVal) primaryExp, symTab);
            } else {
                throw new RuntimeException("奇怪的基本表达式");
            }

            res *= sign;
            if (((UnaryExp) exp).checkExistingNot()) { throw new RuntimeException("我觉得这里不应该有 NOT"); }

            return res;
        } else {
            throw new RuntimeException("这什么表达式啊");
        }
    }

    private float evaluateConstFloatLVal(LVal lVal, SymTab symTab) {
        Symbol symbol;
        symbol = symTab.getObject(lVal.getIdent(), lVal.getLineno());

        if (symbol.isConst() || symbol.isGlobal() && symTab.isGlobal()) {
            Value initVal = symbol.getInitVal();
            if (symbol.getType() instanceof TArray) {
                List<Integer> indexList = new ArrayList<>();
                for (IExp exp : lVal.getIndexList()) {
                    try {
                        indexList.add(evaluateIntConst(exp, symTab).intValue());
                    } catch (FloatOccursException e) {
                        throw new RuntimeException(e);
                    }
                }

                if (initVal instanceof ArrayInitVal) {
                    Value value = ((ArrayInitVal) initVal).getValueWithIndexList(indexList);
                    if (value instanceof IntConst intCon) {
                        return intCon.getConstInt();
                    } else if (value instanceof FloatConst floatCon) {
                        return floatCon.getConstVal().floatValue();
                    } else {
                        throw new RuntimeException("数组里这个值好像不是常值");
                    }
                } else {
                    throw new RuntimeException("数组初值应该是数组初值");
                }
            } else {
                if (initVal instanceof IntConst intConst) {
                    return intConst.getConstInt();
                } else if (initVal instanceof FloatConst floatConst) {
                    return floatConst.getConstVal().floatValue();
                } else {
                    throw new RuntimeException("这个左值没有确定初始值哦~  " + lVal.getIdent());
                }
            }
        } else {
            throw new RuntimeException("这个左值没有确定值哦~  " + lVal.getIdent());
        }
    }

    private static class FloatOccursException extends Exception {
        private final int lineno;

        public FloatOccursException(int lineno) {
            this.lineno = lineno;
        }

        @Override
        public String toString() {
            return "Float number occurs in somewhere must be Integer, at line " + this.lineno;
        }
    }

    public static class Loop {
        private final List<BasicBlock> condition;
        private final BasicBlock exiting;

        public Loop(BasicBlock exiting) {
            this.condition = new ArrayList<>();
            this.exiting = exiting;
        }

        public void renewCondition(List<BasicBlock> newCond) {
            this.condition.clear();
            this.condition.addAll(newCond);
        }

        public BasicBlock getCondBegin() {
            return condition.getFirst();
        }

        public BasicBlock getExiting() {
            return exiting;
        }
    }
}
