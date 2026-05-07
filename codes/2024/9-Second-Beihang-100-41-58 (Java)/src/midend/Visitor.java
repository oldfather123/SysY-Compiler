package midend;

import Utils.LibFunction;
import Utils.SymbolTable;
import frontend.AST.*;
import frontend.AST.Number;
import midend.LLVMType.*;
import midend.instr.*;

import java.util.*;

public class Visitor {
    private Module module;
    private Function curFunction;
    private BasicBlock curBasicBlock;
    private Value curValue;
    private IRBuilder builder = new IRBuilder();
    private SymbolTable symbolTable = new SymbolTable();
    private Stack<BasicBlock> whileEntryBlock = new Stack<>();
    private Stack<BasicBlock> whileOutBlock = new Stack<>();
    private ArrayList<GlobalVar> globalVars = new ArrayList<>();

    public Visitor() {

    }

    public Module getModule() {
        return this.module;
    }


    public void visitAST(AST ast) {
        module = new Module(globalVars);
        ArrayList<CompUnit> units = ast.getUnits();

        LibFunction.initLib();
        symbolTable.pushsymbol();

        curBasicBlock = new BasicBlock(UndefinedType.undefined, null); //临时块
        for(CompUnit unit : units) {
            if (unit instanceof FuncDef) {
                visitFuncDef((FuncDef) unit);
            } else if (unit instanceof Decl) {
                visitDecl((Decl) unit, true);
            } else {
                throw new RuntimeException("IRgen: AST has problems");
            }
        }
    }

    public void visitDecl(Decl decl, boolean isGlobal) {
        String bType = decl.getbType();
        Type type = null;
        if (bType.equals("int")) {
            type = IntegerType.i32;
        } else if (bType.equals("float")) {
            type = FloatType.f32;
        }
        for (Def def : decl.getDefs()) {
            if (decl.getIsConst()) {
                visitConstDef(def, type, isGlobal);
            } else {
                visitVarDef(def, type, isGlobal);
            }
        }
    }

    public void visitConstDef(Def def, Type type, boolean isGlobal) {
        String name = def.getIdent();
        ArrayList<Exp> array = def.getArray();
        if (array.isEmpty()) {
//            AllocaInstr allocaInstr = builder.buildAllocaInstr(type, curBasicBlock);
//            if (!isGlobal) {
//                //curBasicBlock.addInstr(allocaInstr);
//                visitExp((AST.Exp) def.getInitVal());
//                //builder.buildStoreInstr(curValue, allocaInstr, curBasicBlock);
//                pushsymbol(name, curValue);
//            } else {
//                visitExp((AST.Exp) def.getInitVal());
//                GlobalVar globalVar = null;
//                PointerType pointerType = new PointerType(type);
//                if (type == IntegerType.i32) {
//                    globalVar = new GlobalVar(pointerType, name, new ConstInt(IntegerType.i32, ((ConstInt) curValue).getValue()));
//                } else if (type == FloatType.f32) {
//                    globalVar = new GlobalVar(pointerType, name, new ConstFloat(FloatType.f32, ((ConstFloat) curValue).getValue()));
//                }
//                pushsymbol(name, curValue);
//                //globalVars.add(globalVar);
            visitExp((Exp) def.getInitVal());
            if (curValue.getType().isFloat() && type == IntegerType.i32) {
                curValue = new ConstInt(IntegerType.i32, (int) ((ConstFloat) curValue).getValue());
            } else if (curValue.getType().isInteger() && type == FloatType.f32) {
                curValue = new ConstFloat(FloatType.f32, (float) ((ConstInt) curValue).getValue());
            }
            symbolTable.pushsymbol(name, curValue);

        } else {
            //TODO: 数组操作
            visitArray(array, type, name, isGlobal, def.getInitVal(), true);
        }
    }

    public void visitArray(ArrayList<Exp> array, Type type, String name, boolean isGlobal, InitVal initVal, boolean isConst) {
        ArrayList<Value> indexs = new ArrayList<>(); //数组值
        ArrayType arrayType = null;
        for (int count = array.size() - 1; count >= 0; count--) {
            visitExp(array.get(count));
            indexs.add(curValue);
            if (count == array.size() - 1) {
                arrayType = new ArrayType(type, ((ConstInt) curValue).getValue());
            } else {
                arrayType = new ArrayType(arrayType, ((ConstInt) curValue).getValue());
            }
        }
        Collections.reverse(indexs);
        Value defaultValue = (type == IntegerType.i32)? new ConstInt(IntegerType.i32, 0) : new ConstFloat(FloatType.f32, 0.0F);
        if (!isGlobal) {
            if (type == IntegerType.i32) {
                ((ConstInt) defaultValue).setDafult();
            } else if (type == FloatType.f32) {
                ((ConstFloat) defaultValue).setDafult();
            }
        }
        ArrayList<Value> initials = new ArrayList<>();

        initials = visitInitArray((InitArray) initVal, indexs, defaultValue, type);


        if (!isGlobal) {
            AllocaInstr allocaInstr = builder.buildAllocaInstr(arrayType, curBasicBlock);
            symbolTable.pushsymbol(name, allocaInstr);
            curBasicBlock.addInstr(allocaInstr);
            //visitInitArray((AST.InitArray) def.getInitVal(), allocaInstr);
            ArrayList<Value> gepIndex = new ArrayList<>();
            for (int count = 0; count <= array.size(); count++) {
                gepIndex.add(new ConstInt(IntegerType.i32, 0));
            }
            curValue = builder.buildGepInstr(allocaInstr, gepIndex, curBasicBlock);
            //memset
            ArrayList<Value> funcValues = new ArrayList<>();
            if (type.isFloat()) {
                curValue = builder.buildCvsInstr(InstrType.BITCAST, curValue, curBasicBlock);
            }
            funcValues.add(curValue);
            funcValues.add(new ConstInt(IntegerType.i32, 0));
            funcValues.add(new ConstInt(IntegerType.i32, initials.size() * 4));
            builder.buildCallInstr(LibFunction.findFunc("memset"), funcValues, curBasicBlock);

            ArrayList<Value> initValues = new ArrayList<>();
            gepIndex.remove(array.size());
            for (int count = 0; count < initials.size(); count++) {
                Value nowValue = initials.get(count);
                if (nowValue instanceof Constant && ((type == IntegerType.i32 && ((ConstInt) nowValue).getIsDafult()) || (type == FloatType.f32 && ((ConstFloat) nowValue).getIsDafult()))) {
                    continue;
                }
                gepIndex.add(new ConstInt(IntegerType.i32, count));
                curValue = builder.buildGepInstr(allocaInstr, gepIndex, curBasicBlock);
                builder.buildStoreInstr(nowValue, curValue, curBasicBlock);
                gepIndex.remove(array.size());
            }
            allocaInstr.setArrayValues(initials);
        } else {
            //TODO:@var = global []
            GlobalVar globalVar = new GlobalVar(new PointerType(arrayType), name, initials);
            if (isConst) {
                globalVar.setConst();
            }
            symbolTable.pushsymbol(name, globalVar);
            globalVars.add(globalVar);
        }
    }

    public ArrayList<Value> visitInitArray(InitArray initArray, ArrayList<Value> indexs, Value defaultValue, Type type) {
        int nowidx = 0;
        ArrayList<Value> values = new ArrayList<>();
        if (initArray == null) {
            return values;
//            int arraySize = 1;
//            for (Value index : indexs) {
//                arraySize *= ((ConstInt) index).getValue();
//            }
//            for (int count = nowidx; count < arraySize; count++) {
//                values.add(defaultValue);
//            }
//            return values;
        }
        for (InitVal initVal : initArray.getValArrayList()) {
            if (initVal instanceof Exp) {
                visitExp((Exp) initVal);
                if (curValue.getType().isFloat() && type == IntegerType.i32) {
                    curValue = new ConstInt(IntegerType.i32, (int) ((ConstFloat) curValue).getValue());
                } else if (curValue.getType().isInteger() && type == FloatType.f32) {
                    curValue = new ConstFloat(FloatType.f32, (float) ((ConstInt) curValue).getValue());
                }
                values.add(curValue);
                nowidx++;
            } else if (initVal instanceof InitArray) {
                //int start = 1;
                ///
                int start = 0;
                if(nowidx == 0){
                    start = 1;
                }
                else{
                    int tmpMul = 1;
                    for(int i = indexs.size() - 1; i >= 0; i--){
                        tmpMul *= ((ConstInt) indexs.get(i)).getValue();
                        if(nowidx % tmpMul != 0){
                            start = i + 1;
                            break;
                        }
                    }
                }
                ///
                ArrayList<Value> newIndex = new ArrayList<>();
                for (int count = start; count < indexs.size(); count++) {
                    newIndex.add(indexs.get(count));
                }
                ArrayList<Value> newValues = visitInitArray((InitArray) initVal, newIndex, defaultValue, type);
                values.addAll(newValues);
                nowidx += newValues.size();
            } else {
                throw new RuntimeException("bad InitVal type in visitInitArray\n");
            }
        }
        int arraySize = 1;
        for (Value index : indexs) {
            arraySize *= ((ConstInt) index).getValue();
        }
        for (int count = nowidx; count < arraySize; count++) {
            values.add(defaultValue);
        }
        return values;
    }

//    public void visitInitArray(AST.InitArray initArray, Value ptrval) {
//        ArrayList<Value> indexs = new ArrayList<>();
//        indexs.add(new ConstInt(IntegerType.i32, 0));
//        indexs.add(new ConstInt(IntegerType.i32, 0));
//        for (int count = 0; count < initArray.getInitVals().size(); count++) {
//            indexs.set(1, new ConstInt(IntegerType.i32, count));
//            if (initArray.getInitVals().get(count) instanceof AST.Exp) {
//                Value GepValue = builder.buildGepInstr(ptrval, indexs, curBasicBlock);
//                visitExp((AST.Exp) initArray.getInitVals().get(count));
//                builder.buildStoreInstr(curValue, GepValue, curBasicBlock);
//            } else if (initArray.getInitVals().get(count) instanceof AST.InitArray) {
//                Value nowValue = builder.buildGepInstr(ptrval, indexs, curBasicBlock);
//                visitInitArray((AST.InitArray) initArray.getInitVals().get(count), nowValue);
//            } else {
//                throw new RuntimeException("wrong InitVal\n");
//            }
//        }
//    }

    public void visitVarDef(Def def, Type type, boolean isGlobal) {
        String name = def.getIdent();
        ArrayList<Exp> array = def.getArray();
        InitVal initVal = def.getInitVal();
        if (array.isEmpty()) {
            AllocaInstr allocaInstr = builder.buildAllocaInstr(type, curBasicBlock);
            if (!isGlobal) {
                curBasicBlock.addInstr(allocaInstr);
                if (initVal != null) {
                    visitExp((Exp) initVal);
                    if (curValue instanceof Constant && curValue.getType().isFloat() && type == IntegerType.i32) {
                        curValue = new ConstInt(IntegerType.i32, (int) ((ConstFloat) curValue).getValue());
                    } else if (curValue instanceof Constant && curValue.getType().isInteger() && type == FloatType.f32) {
                        curValue = new ConstFloat(FloatType.f32, (float) ((ConstInt) curValue).getValue());
                    }
                    builder.buildStoreInstr(curValue, allocaInstr, curBasicBlock);
                }
                symbolTable.pushsymbol(name, allocaInstr);
            } else {
                if (initVal != null) {
                    visitExp((Exp) initVal);
                    if (curValue.getType().isFloat() && type == IntegerType.i32) {
                        curValue = new ConstInt(IntegerType.i32, (int) ((ConstFloat) curValue).getValue());
                    } else if (curValue.getType().isInteger() && type == FloatType.f32) {
                        curValue = new ConstFloat(FloatType.f32, (float) ((ConstInt) curValue).getValue());
                    }
                    GlobalVar globalVar = null;
                    PointerType pointerType = new PointerType(type);
                    if (type == IntegerType.i32) {
                        globalVar = new GlobalVar(pointerType, name, new ConstInt(IntegerType.i32, ((ConstInt) curValue).getValue()));
                    } else if (type == FloatType.f32) {
                        globalVar = new GlobalVar(pointerType, name, new ConstFloat(FloatType.f32, ((ConstFloat) curValue).getValue()));
                    }
                    //globalVar.setConst();
                    symbolTable.pushsymbol(name, globalVar);
                    globalVars.add(globalVar);
                } else {
                    GlobalVar globalVar = null;
                    PointerType pointerType = new PointerType(type);
                    if (type == IntegerType.i32) {
                        globalVar = new GlobalVar(pointerType, name, new ConstInt(IntegerType.i32, 0));
                    } else if (type == FloatType.f32) {
                        globalVar = new GlobalVar(pointerType, name, new ConstFloat(FloatType.f32, 0.0F));
                    }
                    symbolTable.pushsymbol(name, globalVar);
                    globalVars.add(globalVar);
                }
            }
        } else {
            //TODO:数组操作
            visitArray(array, type, name, isGlobal, def.getInitVal(), false);
        }
    }

    public void visitFuncDef(FuncDef funcDef) {
        String name = funcDef.getIdent();
        Type retType = null;
        if (funcDef.getFuncType().equals("int")) {
            retType = IntegerType.i32;
        } else if (funcDef.getFuncType().equals("float")) {
            retType = FloatType.f32;
        } else if (funcDef.getFuncType().equals("void")) {
            retType = VoidType.voidType;
        } else {
            throw new RuntimeException("bad FuncDef Type");
        }
        Function function = new Function(FunctionType.functionType, "@" + name, retType);

        symbolTable.pushsymbol(name, function);
        curFunction = function;
        symbolTable.pushsymbol();

        //Value.num = 0;

        BasicBlock block = new BasicBlock(UndefinedType.undefined, curFunction);
        curBasicBlock = block;
        curFunction.addBB(block);
        if (funcDef.getFuncFParams() != null && !funcDef.getFuncFParams().isEmpty()) {
            visitFuncParams(funcDef.getFuncFParams());
            BasicBlock basicBlock = new BasicBlock(UndefinedType.undefined, curFunction);
            builder.buildBrInstr(basicBlock, curBasicBlock);
            curBasicBlock = basicBlock;
            curFunction.addBB(curBasicBlock);
        }

        visitBlock(funcDef.getBlock());

        symbolTable.popsymbol();

        //加入ret void
        LinkedList<BasicBlock> basicBlocks = function.getBlockList();
        LinkedList<BasicBlock> removeBlock = new LinkedList<>();
        for (BasicBlock basicBlock : basicBlocks) {
            boolean isEnd = false;
            LinkedList<Instruction> instructions = basicBlock.getInstrList();
//            if (instructions.isEmpty()) {
//                removeBlock.add(basicBlock);
//                continue;
//            }
            ArrayList<Instruction> remove = new ArrayList<>();
            for (int count = 0; count < instructions.size(); count++) {
                if (isEnd == false && (instructions.get(count) instanceof RetInstr || instructions.get(count) instanceof BrInstr)) {
                    isEnd = true;
                    continue;
                }
                if (isEnd) {
                    remove.add(instructions.get(count));
                }
            }
            if (!isEnd) {
                if (retType == VoidType.voidType) {
                    builder.buildRetInstr(VoidType.voidType, "void", null, curBasicBlock);
                } else if (retType == IntegerType.i32) {
                    builder.buildRetInstr(IntegerType.i32, "void", new ConstInt(IntegerType.i32, 0), curBasicBlock);
                } else if (retType == FloatType.f32) {
                    builder.buildRetInstr(FloatType.f32, "void", new ConstFloat(FloatType.f32, 0.0F), curBasicBlock);
                } else {
                    throw new RuntimeException("wrong retType\n");
                }
            } else {
                for (Instruction instruction : remove) {
                    instruction.remove();
                }
            }
        }

        //alloc提前
//        BasicBlock entryBlock = function.getBlockList().get(0);
//        for (BasicBlock basicBlock : basicBlocks) {
//            ArrayList<Instruction> alloca = new ArrayList<>();
//            LinkedList<Instruction> instructions = basicBlock.getInstrList();
//            for (Instruction instruction : instructions) {
//                if (instruction instanceof AllocaInstr) {
//                    alloca.add(instruction);
//                }
//            }
//            for (int count = alloca.size() - 1; count >= 0; count--) {
//                Instruction instruction = alloca.get(count);
//                instruction.setBasicBlock(entryBlock);
//                instructions.remove(instruction);
//                entryBlock.getInstrList().addFirst(instruction);
//            }
//        }
        basicBlocks.removeAll(removeBlock);

        ArrayList<BasicBlock> exitBlocks = new ArrayList<>();
        for (BasicBlock basicBlock : basicBlocks) {
            if (basicBlock.getInstrList().getLast() instanceof RetInstr) {
                exitBlocks.add(basicBlock);
            }
        }

        //exitBlock
        boolean isVoid = false;
        if (function.getRetType().isVoid()) {
            isVoid = true;
        }
        if (exitBlocks.size() == 1) {
            function.setExitBlock(exitBlocks.get(0));
        } else {
            BasicBlock basicBlock = new BasicBlock(UndefinedType.undefined, function);
            function.addBB(basicBlock);
            //第一个块中加入alloca
            AllocaInstr ret = null;
            if (!isVoid) {
                ret = builder.buildAllocaInstr(function.getRetType(), function.getBlockList().get(0));
                function.getBlockList().get(0).getInstrList().addFirst(ret);
            }
            for (BasicBlock exitBlock : exitBlocks) {
                RetInstr retInstr = (RetInstr) exitBlock.getInstrList().getLast();
                if (!isVoid) {
                    StoreInstr storeInstr = builder.buildStoreInstr(retInstr.getValue(), ret, exitBlock);
                    //exitBlock.addInstr(storeInstr);
                }
                BrInstr brInstr = builder.buildBrInstr(basicBlock, exitBlock);
                retInstr.remove();
            }
            LoadInstr loadInstr = null;
            if (!isVoid) {
                loadInstr = builder.buildLoadInstr(ret, basicBlock);
                //basicBlock.addInstr(loadInstr);
            }
            builder.buildRetInstr(function.getRetType(), "void", loadInstr, basicBlock);
            function.setExitBlock(basicBlock);
        }
        //System.out.println(function.getExitBlock().getName());

        module.addFunction(function);
    }

    public void visitFuncParams(ArrayList<FuncFParam> params) {
        ArrayList<Param> params1 = new ArrayList<>();
        for (FuncFParam param : params) {
            params1.add(visitFuncFParam(param));
        }
        //Value.num++;
        for (int count = 0; count < params1.size(); count++) {
            AllocaInstr instr = builder.buildAllocaInstr(params1.get(count).getType(), curBasicBlock);
            curBasicBlock.addInstr(instr);
            builder.buildStoreInstr(params1.get(count), instr, curBasicBlock);
            symbolTable.pushsymbol(params.get(count).getIdent(), instr);
        }
    }

    public Param visitFuncFParam(FuncFParam param) {
        String bType = param.getbType();
        String ident = param.getIdent();
        Param param1 = null;
        Type type = null;
        if (bType.equals("void")) {
            type = VoidType.voidType;
        } else if (bType.equals("int")) {
            type = IntegerType.i32;
        } else if (bType.equals("float")) {
            type = FloatType.f32;
        } else {
            throw new RuntimeException("bad Param type");
        }
        if (!param.getIsArray()) {
            param1 = new Param(type);
            curFunction.addParam(param1);
            //pushsymbol(ident, param1);
        } else {
            //TODO:数组参数
            ArrayList<Exp> array = param.getArray();
            for (int count = array.size() - 1; count >= 0; count--) {
                visitExp(array.get(count));
                type = new ArrayType(type, ((ConstInt) curValue).getValue());
            }
            type = new PointerType(type);
            param1 = new Param(type);
            curFunction.addParam(param1);
            //pushsymbol(ident, param1);
        }
        return param1;
    }

    public void visitBlock(Block block) {
        ArrayList<BlockItem> blockItems = block.getBlockItems();
        for(BlockItem blockItem : blockItems) {
            visitBlockItem(blockItem);
        }
    }

    public void visitBlockItem(BlockItem blockItem) {
        if (blockItem instanceof Stmt) {
            visitStmt((Stmt) blockItem);
        } else if (blockItem instanceof Decl) {
            visitDecl((Decl) blockItem, false);
        } else {
            throw new RuntimeException("BlockItem is bad");
        }
    }

    public void visitStmt(Stmt stmt) {
        if (stmt instanceof Return) {
            RetInstr retInstr = null;

            if (((Return) stmt).getExp() == null) {
                Value value = new Value(VoidType.voidType, "void");
                builder.buildRetInstr(VoidType.voidType, "void", value, curBasicBlock);
            } else {
                visitExp(((Return) stmt).getExp());
                if (curValue.getType() == IntegerType.i32 && curFunction.getRetType() == FloatType.f32) {
                    curValue = builder.buildCvsInstr(InstrType.SITOFP, curValue, curBasicBlock);
                } else if (curValue.getType() == FloatType.f32 && curFunction.getRetType() == IntegerType.i32) {
                    curValue = builder.buildCvsInstr(InstrType.FPTOSI, curValue, curBasicBlock);
                }
//                if (curFunction.getName().equals("@main")) {
//                    ArrayList<Value> values = new ArrayList<>();
//                    values.add(curValue);
//                    builder.buildCallInstr(PUTINT, values, curBasicBlock);
//                }
                builder.buildRetInstr(curFunction.getRetType(), "void", curValue, curBasicBlock);
            }
        } else if (stmt instanceof Assign) {
            visitLval(((Assign) stmt).getLval());
            Value pointer = curValue;
            visitExp(((Assign) stmt).getExp());
            builder.buildStoreInstr(curValue, pointer, curBasicBlock);
        } else if (stmt instanceof If) {
            visitIf((If) stmt);
        } else if (stmt instanceof While){
            visitWhile((While) stmt);
        } else if (stmt instanceof Break){
            if (whileOutBlock.size() == 0) {
                return;
            } else {
                //builder.buildBrInstr(whileOutBlock.peek(), curBasicBlock);
                BasicBlock block = new BasicBlock(UndefinedType.undefined, curFunction);
                curFunction.addBB(block);
                builder.buildBrInstr(block, curBasicBlock);
                curBasicBlock = block;
                builder.buildBrInstr(whileOutBlock.peek(), curBasicBlock);
            }
        } else if (stmt instanceof Continue) {
            if (whileEntryBlock.size() == 0) {
                return;
            } else {
                BasicBlock block = new BasicBlock(UndefinedType.undefined, curFunction);
                curFunction.addBB(block);
                builder.buildBrInstr(block, curBasicBlock);
                curBasicBlock = block;
                builder.buildBrInstr(whileEntryBlock.peek(), curBasicBlock);
            }
        } else if (stmt instanceof ExpStmt) {
            visitExp(((ExpStmt) stmt).getExp());
        } else if (stmt instanceof Block) {
            symbolTable.pushsymbol();
            visitBlock((Block) stmt);
            symbolTable.popsymbol();
        } else {
            throw new RuntimeException("stmt is bad\n");
        }
    }

    public void visitWhile(While whileStmt) {
        BasicBlock condBlock = new BasicBlock(UndefinedType.undefined, curFunction);
        curFunction.addBB(condBlock);
        BasicBlock whileBlock = new BasicBlock(UndefinedType.undefined, curFunction);
        curFunction.addBB(whileBlock);
        BasicBlock nextBlock = new BasicBlock(UndefinedType.undefined, curFunction);
        curFunction.addBB(nextBlock);

        whileEntryBlock.push(condBlock);
        whileOutBlock.push(nextBlock);

        builder.buildBrInstr(condBlock, curBasicBlock);
        curBasicBlock = condBlock;
        visitLorExp(whileStmt.getCond(), nextBlock, whileBlock);
        //builder.buildBrInstr(whileBlock, curBasicBlock);

        curBasicBlock = whileBlock;
        visitStmt(whileStmt.getWhileStmt());
        builder.buildBrInstr(condBlock, curBasicBlock);

        curBasicBlock = nextBlock;

        whileOutBlock.pop();
        whileEntryBlock.pop();
    }

    public void visitIf(If ifStmt) {
        BasicBlock ifBlock = new BasicBlock(UndefinedType.undefined, curFunction);
        curFunction.addBB(ifBlock);
        BasicBlock nextBlock = new BasicBlock(UndefinedType.undefined, curFunction);
        curFunction.addBB(nextBlock);
        BasicBlock elseBlock = null;
        if (ifStmt.getHaveElse()) {
            elseBlock = new BasicBlock(UndefinedType.undefined, curFunction);
            curFunction.addBB(elseBlock);
            visitLorExp(ifStmt.getCond(), elseBlock, ifBlock);
        } else {
            visitLorExp(ifStmt.getCond(), nextBlock, ifBlock);
        }

        curBasicBlock = ifBlock;
        visitStmt(ifStmt.getIfStmt());
        builder.buildBrInstr(nextBlock, curBasicBlock);

        if (elseBlock != null) {
            curBasicBlock = elseBlock;
            visitStmt(ifStmt.getElseStmt());
            builder.buildBrInstr(nextBlock, curBasicBlock);
        }

        curBasicBlock = nextBlock;
    }

    public void visitLorExp(Exp cond, BasicBlock endBlock, BasicBlock stmtBlock) {
        BinaryExp binaryExp = (BinaryExp) cond;
        Exp first = binaryExp.getFirst();
        ArrayList<String> ops = binaryExp.getBinaryOps();
        ArrayList<Exp> exps = binaryExp.getExps();
        BasicBlock nextBlock;
        for (int count = 0; count < ops.size() + 1; count++) {
            if (count != 0) {
                first = exps.get(count - 1);
            }

            if (count < ops.size()) {
                nextBlock = new BasicBlock(UndefinedType.undefined, curFunction);
                curFunction.addBB(nextBlock);
            } else {
                nextBlock = endBlock;
            }
            visitLandExp(first, nextBlock, stmtBlock);
//            builder.buildBrInstr(curValue, endBlock, nextBlock, curBasicBlock);
//            curFunction.addBB(nextBlock);
            curBasicBlock = nextBlock;
        }
    }

    public void visitLandExp(Exp cond, BasicBlock endBlock, BasicBlock stmtBlock) {
        BinaryExp binaryExp = (BinaryExp) cond;
        Exp first = binaryExp.getFirst();
        ArrayList<String> ops = binaryExp.getBinaryOps();
        ArrayList<Exp> exps = binaryExp.getExps();
        BasicBlock nextBlock;
        for (int count = 0; count < ops.size() + 1; count++) {
            if (count != 0) {
                first = exps.get(count - 1);
            }

            if (count < ops.size()) {
                nextBlock = new BasicBlock(UndefinedType.undefined, curFunction);
                curFunction.addBB(nextBlock);
            } else {
                nextBlock = stmtBlock;
            }

            visitExp(first);

            if (!(curValue instanceof CmpInstr)) {
                if (curValue.getType() == IntegerType.i32) {
                    curValue = builder.buildALUInstr(curValue, new ConstInt(IntegerType.i32, 0), "!=", curBasicBlock);
                } else {
                    curValue = builder.buildALUInstr(curValue, new ConstInt(IntegerType.i1, 0), "!=", curBasicBlock);
                }
            }
            builder.buildBrInstr(curValue, nextBlock, endBlock, curBasicBlock);
            curBasicBlock = nextBlock;
        }
    }

    public void visitLval(Lval lval) {
        String ident = lval.getIdent();
        Value value = symbolTable.find(ident);

        boolean isConst = true;
        if (value instanceof GlobalVar && lval.getIsArray() && ((GlobalVar) value).isConst()) {
            int count = 0;
            ArrayType arrayType = (ArrayType) ((PointerType) ((GlobalVar) value).getType()).getElementType();
            ArrayList<Integer> numList = arrayType.getNumList();
            ArrayList<Integer> number = new ArrayList<>();
            number.add(1);
            for (int count1 = numList.size() - 1; count1 > 0; count1--) {
                int num = number.get(0);
                number.add(0, num * numList.get(count1));
            }
            ArrayList<Exp> indexs = lval.getIndexs();

            for (int count2 = 0; count2 < indexs.size(); count2++) {
                visitExp(indexs.get(count2));
                if (!(curValue instanceof ConstInt)) {
                    isConst = false;
                    break;
                }
                count += ((ConstInt) curValue).getValue() * number.get(count2);
            }
            if (isConst){
                if (((GlobalVar) value).isAllZero()) {
                    if (arrayType.isIntegerArray()) {
                        curValue = new ConstInt(IntegerType.i32, 0);
                    } else {
                        curValue = new ConstFloat(FloatType.f32, 0.0F);
                    }
                    return;
                }
                curValue = ((GlobalVar) value).getValue(count);
                return;
            }
        }

        if (isConst && value instanceof GlobalVar && ((GlobalVar) value).isConst()) {
            curValue = ((GlobalVar) value).getValue();
            return;
        }

        Type eleType = null;
        if (value.getType().isPointer()) {
            eleType = ((PointerType) value.getType()).getElementType();
        }
        if (lval.getIsArray()) {
            //TODO:数组
            ArrayList<Exp> indexs = lval.getIndexs();
            ArrayList<Value> arrayIndex = new ArrayList<>();
            if (eleType instanceof PointerType) {
                value = builder.buildLoadInstr(value, curBasicBlock);
            } else {
                arrayIndex.add(new ConstInt(IntegerType.i32, 0));
            }
            for (Exp index : indexs) {
                visitExp(index);
                arrayIndex.add(curValue);
            }
            curValue = builder.buildGepInstr(value, arrayIndex, curBasicBlock);
        } else {
            curValue = value;
        }

    }

    public void visitExp(Exp exp) {
        if (exp instanceof BinaryExp) {
            visitBinaryExp((BinaryExp) exp);
        } else if (exp instanceof UnaryExp) {
            visitUnaryExp((UnaryExp) exp);
        }
    }

    public void visitBinaryExp(BinaryExp binaryExp) {
        visitExp(binaryExp.getFirst()); //curValue是first
        ArrayList<String> binaryOps = binaryExp.getBinaryOps();
        ArrayList<Exp> exps = binaryExp.getExps();
        for(int count = 0; count < binaryOps.size(); count++) {
            Exp exp = exps.get(count);
            Value left = curValue;
            visitExp(exp);
            String op = binaryOps.get(count);
            if (left instanceof Constant && curValue instanceof Constant) {
                curValue = builder.buildNumber((Constant) left, (Constant) curValue, op);
            } else {
                curValue = builder.buildALUInstr(left, curValue, op, curBasicBlock);
            }
        }
    }

    public void visitUnaryExp(UnaryExp unaryExp) {
        visitPrimaryExp(unaryExp.getPrimaryExp());
        ArrayList<String> unaryOps = unaryExp.getUnaryOps();
        int count = 0;
        for(int count0 = unaryOps.size() - 1; count0 >= 0; count0--) {
            String unaryOp = unaryOps.get(count0);
            if (unaryOp.equals("-")){
                count++;
            } else if (unaryOp.equals("!")){
                curValue = builder.buildALUInstr(curValue, new ConstInt(IntegerType.i1, 0), "==", curBasicBlock);
                count = 0;
            }
        }
        if (count % 2 == 1) {
            if (curValue instanceof ConstInt) {
                curValue = new ConstInt(IntegerType.i32, -((ConstInt) curValue).getValue());
            } else if (curValue instanceof ConstFloat) {
                curValue = new ConstFloat(FloatType.f32, -((ConstFloat) curValue).getValue());
            } else {
                curValue = builder.buildALUInstr(new ConstInt(IntegerType.i32, 0), curValue, "-", curBasicBlock);
            }
        }
    }

    public void visitPrimaryExp(PrimaryExp primaryExp) {
        if (primaryExp instanceof Number) {
            visitNumber((Number) primaryExp);
        } else if (primaryExp instanceof Exp) {
            visitExp((Exp) primaryExp);
        } else if (primaryExp instanceof Lval) {
            visitLval((Lval) primaryExp);
            if (!(curValue instanceof Constant)) {
                Type type = ((PointerType) curValue.getType()).getElementType();
                if (!type.isArray()) {
                    curValue = builder.buildLoadInstr(curValue, curBasicBlock);
                } else {
                    ArrayList<Value> indexs = new ArrayList<>();
                    indexs.add(new ConstInt(IntegerType.i32, 0));
                    indexs.add(new ConstInt(IntegerType.i32, 0));
                    curValue = builder.buildGepInstr(curValue, indexs, curBasicBlock);
                }
            }
        } else if (primaryExp instanceof Func) {
            Function function = (Function) symbolTable.find(((Func) primaryExp).getIdent());
            ArrayList<Exp> params = ((Func) primaryExp).getFuncRParams();
            ArrayList<Value> values = new ArrayList<>();
            for (Exp param : params) {
                visitExp(param);
                values.add(curValue);
            }

            ArrayList<Param> params0 = function.getParams();
            for (int count = 0; count < values.size(); count++) {
                Value value = values.get(count);
                Type paramType = params0.get(count).getType();
                if (value.getType() == IntegerType.i32 && paramType == FloatType.f32) {
                    Value value1 = builder.buildCvsInstr(InstrType.SITOFP, value, curBasicBlock);
                    values.set(count, value1);
                } else if (value.getType() == FloatType.f32 && paramType == IntegerType.i32) {
                    Value value1 = builder.buildCvsInstr(InstrType.FPTOSI, value, curBasicBlock);
                    values.set(count, value1);
                }
            }

            if (function.getName().equals("@starttime") || function.getName().equals("@stoptime")) {
                values.add(new ConstInt(IntegerType.i32, ((Func) primaryExp).getLinuNum()));
            }

            curValue = builder.buildCallInstr(function, values, curBasicBlock);

        }
    }

    public void visitNumber(Number number) {
        if (number.getIsInt()) {
            curValue = new ConstInt(IntegerType.i32, number.getIntConst());
        } else if (number.getIsFloat()) {
            curValue = new ConstFloat(FloatType.f32, number.getFloatConst());
        }
    }
}
