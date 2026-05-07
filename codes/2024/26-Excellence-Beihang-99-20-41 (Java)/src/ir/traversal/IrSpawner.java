package ir.traversal;

import frontend.ast.AbstractSyntaxTree;
import frontend.ast.ExprWithLeading;
import frontend.lexer.TokenType;
import ir.IrFactory;
import ir.IrInstr;
import ir.instr.*;
import ir.traversal.SymbolTable.SymbolTableEntry;
import ir.traversal.SymbolTable.SymbolId;
import ir.type.ArrayType;
import ir.type.IrType;
import ir.type.PointerType;
import ir.type.Ty;
import ir.value.*;
import utils.GlobalCounter;
import utils.Pair;

import java.util.*;
import java.util.stream.Collectors;

import static utils.Panic.panicIfFalse;

public class IrSpawner {
    private final SymbolTable symbolTable;
    private final IrFactory builder;
    // Loop Control
    private final LinkedList<String> afterBlocks;
    private final LinkedList<String> condBlocks;
    // Variable that is not changed
    private final HashMap<String, Intermediate> notChangedVars = new HashMap<>();

    public IrSpawner(SymbolTable symbolTable, String moduleName) {
        this.builder = new IrFactory(moduleName);
        Intermediate.builder = builder;
        this.symbolTable = symbolTable;
        this.afterBlocks = new LinkedList<>();
        this.condBlocks = new LinkedList<>();
    }

    public IrFactory spawn() {
        addLibFunc();
        declareGlobalVars();
        for (SymbolTableEntry entry : symbolTable.getTable()) {
            if (entry.id().id() == -2) {
                declareFunction(entry);
            }
        }
        return builder;
    }

    public void addLibFunc() {
        Function function;
        List<Function> libs = new LinkedList<>();

        // void memset(void *, int, int);
        function = new Function("memset", Ty.VOID);
        function.addArgument(new Argument("_1", new PointerType(Ty.VOID)));
        function.addArgument(new Argument("_2", Ty.I32));
        function.addArgument(new Argument("_3", Ty.I32));
        libs.add(function);

        // int getint();
        libs.add(new Function("getint", Ty.I32));

        // int getch();
        libs.add(new Function("getch", Ty.I32));

        // float getfloat();
        libs.add(new Function("getfloat", Ty.F32));

        // int getarray(int[]);
        function = new Function("getarray", Ty.I32);
        function.addArgument(new Argument("_1", new PointerType(Ty.I32)));
        libs.add(function);

        // int getfarray(float[]);
        function = new Function("getfarray", Ty.I32);
        function.addArgument(new Argument("_1", new PointerType(Ty.F32)));
        libs.add(function);

        // void putint(int);
        function = new Function("putint", Ty.VOID);
        function.addArgument(new Argument("_1", Ty.I32));
        libs.add(function);

        // void putch(int);
        function = new Function("putch", Ty.VOID);
        function.addArgument(new Argument("_1", Ty.I32));
        libs.add(function);

        // void putfloat(float);
        function = new Function("putfloat", Ty.VOID);
        function.addArgument(new Argument("_1", Ty.F32));
        libs.add(function);

        // void putarray(int, int[]);
        function = new Function("putarray", Ty.VOID);
        function.addArgument(new Argument("_1", Ty.I32));
        function.addArgument(new Argument("_2", new PointerType(Ty.I32)));
        libs.add(function);

        // void putfarray(int, float[]);
        function = new Function("putfarray", Ty.VOID);
        function.addArgument(new Argument("_1", Ty.I32));
        function.addArgument(new Argument("_2", new PointerType(Ty.F32)));
        libs.add(function);

        // void putf("<string>", int, ...);
        // Skip!

        // Special!
        // void starttime();
        libs.add(new Function("starttime", Ty.VOID));
        function = new Function("_sysy_starttime", Ty.VOID);
        function.addArgument(new Argument("_1", Ty.I32));
        libs.add(function);

        // Special!
        // void stoptime();
        libs.add(new Function("stoptime", Ty.VOID));
        function = new Function("_sysy_stoptime", Ty.VOID);
        function.addArgument(new Argument("_1", Ty.I32));
        libs.add(function);

        for (Function lib : libs) {
            symbolTable.put(new SymbolTableEntry(new SymbolId(lib.getName(), -1), lib, true, null));
        }

        for (SymbolTableEntry entry : symbolTable.getTable()) {
            if (entry.id().id() == -1) {
                builder.externFunction(entry);
            }
        }
    }

    public void declareGlobalVars() {
        for (SymbolTableEntry entry : symbolTable.getTable()) {
            if (entry.value() instanceof Constant constant) {
                constant.applyMangleToName();
            }
            // 注意，Variable 继承自 Constant，所以要先判断 Variable
            if (entry.global() && entry.value() instanceof ir.value.Variable) {
                if (entry.value().getType().is("ARRAY") || ((Variable) entry.value()).isDirty()) {
                    builder.declareGlobalVariable(entry);
                } else {
                    Intermediate imm;
                    if (((Variable) entry.value()).getInitialValue() instanceof Integer) {
                        imm = new Literal((Integer) ((Variable) entry.value()).getInitialValue());
                    } else if (((Variable) entry.value()).getInitialValue() instanceof Float) {
                        imm = new Literal((Float) ((Variable) entry.value()).getInitialValue());
                    } else {
                        throw new RuntimeException("Unknown Constant Type: " + ((Variable) entry.value()).getInitialValue().getClass().getName());
                    }
                    notChangedVars.put(entry.emitValue().getName(), imm);
                }
            } else if (entry.global() && entry.value() instanceof ir.value.Constant && entry.value().getType().is("ARRAY")) {
                builder.declareGlobalConstant(entry);
            }
        }
    }

    public void declareFunction(SymbolTableEntry entry) {
        builder.declareFunction(entry);
        Function function = (Function) entry.value();
        AbstractSyntaxTree.Block funcBody = function.getBody();
        builder.setInsertPoint(builder.registerBlock());
        // Alloc Stack Space for Local Vars
        allocate(funcBody.symbols().get());
        spawnBlock(funcBody, null);
        if (!builder.hasTerminator(builder.getInsertPoint())) {
            if (function.getType().is("VOID")) {
                builder.insertInstr(new TermInstr(IrInstr.OpCode.RET));
            } else if (function.getType().is("INT")) {
                builder.insertInstr(new TermInstr(new Literal(0), IrInstr.OpCode.RET));
            } else if (function.getType().is("FLOAT")) {
                builder.insertInstr(new TermInstr(new Literal(0f), IrInstr.OpCode.RET));
            } else {
                throw new RuntimeException("Unexpected return type: " + function.getType());
            }
        }
        builder.setInsertPoint(null);
    }

    private void allocate(SymbolTable table) {
        for (SymbolTableEntry entry : table.getTable()) {
            if (entry.value() instanceof Constant constant) {
                constant.applyMangleToName();
            }
            if (entry.value() instanceof ir.value.Variable) {
                if (entry.id().type().equals("p")) {
                    builder.insertInstr(new AllocaInstr(
                        new Intermediate("%" + entry.id() + ".addr", new PointerType(entry.value().getType())),
                        new PointerType(entry.value().getType()),
                        entry.value().getType().sizeof(), Ty.I32
                    ));
                    builder.insertInstr(new StoreInstr(
                        entry.value().getType(),
                        entry.emitValue(),
                        new PointerType(entry.value().getType()),
                        new Intermediate("%" + entry.id() + ".addr", new PointerType(entry.value().getType()))
                    ));
                } else if (entry.value().getType().is("ARRAY")) {
                    builder.insertInstr(new AllocaInstr(
                        entry.emitValue(), new PointerType(entry.value().getType()),
                        entry.value().getType().sizeof(), Ty.I32
                    ));
                } else if (((Variable) entry.value()).isDirty()) {
                    builder.insertInstr(new AllocaInstr(
                        entry.emitPointer(), new PointerType(entry.value().getType()),
                        entry.value().getType().sizeof(), Ty.I32
                    ));
                }
            } else if (entry.value() instanceof ir.value.Constant && entry.value().getType().is("ARRAY")) {
                builder.declareGlobalConstant(entry);
            }
        }
        for (SymbolTable child : table.getChildren()) {
            allocate(child);
        }
    }

    private void spawnBlock(AbstractSyntaxTree.Block block, String exit) {
        // Emit Function Body
        SymbolTable blockTable = block.symbols().get();
        for (AbstractSyntaxTree.BlockItem item : block.items()) {
            if (item instanceof AbstractSyntaxTree.Statement) {
                if (spawnStatement((AbstractSyntaxTree.Statement) item, exit, false)) {
                    return;
                }
            } else if (item instanceof AbstractSyntaxTree.Declaration declaration) {
                if (declaration instanceof AbstractSyntaxTree.VarDecl varDecl) {
                    if (varDecl.isConst()) {
                        continue;
                    }
                    for (AbstractSyntaxTree.VarDef varDef : varDecl.decls()) {
                        if (varDef.init() == null) {
                            continue;
                        }
                        SymbolTableEntry entry = blockTable.searchDown(varDef.redirection().getTargetSymbol());
                        if (varDef.init().values() != null) {
                            ArrayType arrayType = (ArrayType) entry.value().getType();
                            TreeMap<Integer, Object> initVals;
                            if (entry.value() instanceof Variable variable) {
                                initVals = (TreeMap<Integer, Object>) variable.getInitialValue();
                            } else {
                                initVals = (TreeMap<Integer, Object>) ((Constant) entry.value()).getValue();
                            }
                            arrayFill(arrayType.getDimension(), arrayType, initVals, entry.emitValue().getName());
                            continue;
                        }
                        if (((Variable) entry.value()).getInitialValue() == null) {
                            Intermediate initResult = calculate(varDef.init().expr());
                            initResult = Intermediate.doConvert(initResult, entry.value().getType());
                            if (!((Variable) entry.value()).isDirty()) {
                                notChangedVars.put(entry.emitValue().getName(), initResult);
                            } else {
                                builder.insertInstr(new StoreInstr(
                                        initResult.getType(), initResult,
                                        new PointerType(entry.value().getType()), entry.emitPointer()));
                            }
                        } else {
                            // Store the imm
                            Intermediate imm;
                            if (((Variable) entry.value()).getInitialValue() instanceof Integer) {
                                imm = new Literal((Integer) ((Variable) entry.value()).getInitialValue());
                            } else if (((Variable) entry.value()).getInitialValue() instanceof Float) {
                                imm = new Literal((Float) ((Variable) entry.value()).getInitialValue());
                            } else {
                                throw new RuntimeException("Unknown Constant Type: " + ((Variable) entry.value()).getInitialValue().getClass().getName());
                            }
                            if (!((Variable) entry.value()).isDirty()) {
                                notChangedVars.put(entry.emitValue().getName(), imm);
                            } else {
                                builder.insertInstr(new StoreInstr(
                                        entry.value().getType(), imm,
                                        new PointerType(entry.value().getType()), entry.emitPointer()));
                            }
                        }
                    }
                }
            } else {
                throw new RuntimeException("Unknown Block Item Type: " + item.getClass().getName());
            }
        }
        // Default exit block
        if (exit != null) {
            builder.insertInstr(new TermInstr(builder.getBlock(exit), IrInstr.OpCode.BR));
        }
    }

    private void arrayFillElem(PointerType pointerType, int curIndex, Intermediate val, String symbol) {
        String srcPointer = GlobalCounter.gen("%").toString();
        builder.insertInstr(new BinaInstr(
            new Intermediate(symbol, pointerType),
            new Literal(curIndex), IrInstr.OpCode.ADD,
            new Intermediate(srcPointer, pointerType)));
        builder.insertInstr(new StoreInstr(
            pointerType.getFinalBase(), val,
            pointerType, new Intermediate(srcPointer, pointerType)));
    }

    private Intermediate arrayFillInit(Object value) {
        if (value instanceof Float) {
            return new Literal((Float) value);
        } else if (value instanceof Integer) {
            return new Literal((Integer) value);
        } else if (value instanceof AbstractSyntaxTree.AddExpr expr) {
            return calculate(expr);
        } else {
            throw new RuntimeException("Unknown Constant Type: " + value.getClass().getName());
        }
    }

    private void arrayFill(List<Integer> sizes, ArrayType irType,
                          TreeMap<Integer, Object> initVals, String symbol) {
        int length = 1;
        for (int size : sizes) {
            length *= size;
        }
        int non_zero = initVals.size();
        if (non_zero * 2 > length) {
            // Fill one by one
            int curIndex = 0;
            for (Map.Entry<Integer, Object> entry : initVals.entrySet()) {
                for (int i = curIndex; i < entry.getKey(); i++) {
                    arrayFillElem(new PointerType(irType.getFinalBase()), i, Literal.zero(irType.getFinalBase()), symbol);
                }
                int index = entry.getKey();
                arrayFillElem(new PointerType(irType.getFinalBase()), index, arrayFillInit(entry.getValue()), symbol);
                curIndex = index + 1;
            }
            for (int i = curIndex; i < length; i++) {
                arrayFillElem(new PointerType(irType.getFinalBase()), i, Literal.zero(irType.getFinalBase()), symbol);
            }
        } else {
            // Only fill non-zero values
//            builder.insertInstr(new ZeroFillInstr(new Intermediate(symbol, new PointerType(irType.getFinalBase())), length));
            SymbolTableEntry func = symbolTable.searchDown(new SymbolId("@memset", -1));
            builder.insertInstr(new TermInstr(
                func.value(),
                new LinkedList<>(List.of(
                        new Intermediate(symbol, new PointerType(irType.getFinalBase())),
                        Literal.zero(Ty.I32), new Literal(length * 4))),
                IrInstr.OpCode.CALL, null)
            );
            for (Map.Entry<Integer, Object> entry : initVals.entrySet()) {
                int index = entry.getKey();
                arrayFillElem(new PointerType(irType.getFinalBase()), index, arrayFillInit(entry.getValue()), symbol);
            }
        }
    }

    /**
     * 生成语句
     *
     * @param stmt 语句的 AST 节点
     * @param exit 退出后，需要跳转到的块，null 表示默认处理
     * @return 是否提前结束块
     */
    public boolean spawnStatement(AbstractSyntaxTree.Statement stmt, String exit, boolean terminate) {
        if (stmt instanceof AbstractSyntaxTree.AssignStmt assign) {
            SymbolTableEntry entry = symbolTable.searchDown(assign.lvalue().redirection().getTargetSymbol());
            IrType targetType = entry.value().getType();
            // 这里没有统一变量在 Java 中的类型，需要注意！
            String targetName = entry.emitValue().getName();
            if (!assign.lvalue().indexes().isEmpty()) {
                List<Intermediate> indexes = new LinkedList<>();
                List<Integer> sizes;
                if (targetType.is("POINTER")) {
                    LinkedList<Integer> list = new LinkedList<>();
                    list.add(0);
                    if (((PointerType) targetType).getBaseType() instanceof ArrayType arrayType) {
                        list.addAll(arrayType.getDimension());
                    }
                    sizes = list;
                } else {
                    sizes = ((ArrayType) targetType).getDimension();
                }
                for (AbstractSyntaxTree.AddExpr addExpr : assign.lvalue().indexes()) {
                    Intermediate index = calculate(addExpr);
                    indexes.add(Intermediate.doConvert(index, Ty.I32));
                }
                Intermediate loadedIndex = indexes.get(0);
                for (int i = 1; i < indexes.size(); i++) {
                    // Current Index *= dimension[i]
                    int curSize = sizes.get(i);
                    Intermediate offsetBase = new Intermediate(Ty.I32);
                    builder.insertInstr(new BinaInstr(
                        loadedIndex, new Literal(curSize), IrInstr.OpCode.MUL, offsetBase
                    ));
                    Intermediate offsetIndex = new Intermediate(Ty.I32);
                    // Current Index += size[i]
                    Intermediate index = indexes.get(i);
                    builder.insertInstr(new BinaInstr(offsetBase, index, IrInstr.OpCode.ADD, offsetIndex));
                    loadedIndex = offsetIndex;
                }
                if (targetType instanceof ArrayType arrayType) {
                    targetType = arrayType.getFinalBase();
                } else {
                    targetType = ((PointerType) targetType).getBaseType();
                    if (targetType instanceof ArrayType arrayType) {
                        targetType = arrayType.getFinalBase();
                    }
                }
                PointerType pointerType = new PointerType(targetType);
                Intermediate srcPointer = new Intermediate(pointerType);
                builder.insertInstr(new BinaInstr(new Intermediate(targetName, pointerType), loadedIndex, IrInstr.OpCode.ADD, srcPointer));
                targetName = srcPointer.name;
            }
            Intermediate rvalue = calculate(assign.rvalue());
            rvalue = Intermediate.doConvert(rvalue, targetType);
            if (targetName.startsWith("%p")) {
                // How dare it
                targetName = targetName + ".addr";
            }
            builder.insertInstr(new StoreInstr(
                targetType, rvalue,
                new PointerType(targetType), new Intermediate(targetName, new PointerType(targetType))
            ));
        } else if (stmt instanceof AbstractSyntaxTree.ExprStmt expr) {
            if (expr.expr() != null) {
                calculate(expr.expr());
            }
        } else if (stmt instanceof AbstractSyntaxTree.BranchStmt branch) {
            Intermediate ret = calculate(branch.condition());

            String thenBlock = builder.registerBlock(new CommentInstr("if.then"));
            String elseBlock = null;
            if (branch.elseBlock() != null) {
                elseBlock = builder.registerBlock(new CommentInstr("if.else"));
            }
            String afterBlock = builder.registerBlock(new CommentInstr("if.after"));

            if (branch.elseBlock() == null) {
                builder.insertInstr(new TermInstr(Intermediate.doConvert(ret, Ty.I1), builder.getBlock(thenBlock), builder.getBlock(afterBlock), IrInstr.OpCode.BR));
            } else {
                builder.insertInstr(new TermInstr(Intermediate.doConvert(ret, Ty.I1), builder.getBlock(thenBlock), builder.getBlock(elseBlock), IrInstr.OpCode.BR));
            }

            builder.setInsertPoint(thenBlock);
            spawnStatement(branch.thenBlock(), afterBlock, true);

            if (branch.elseBlock() != null) {
                builder.setInsertPoint(elseBlock);
                spawnStatement(branch.elseBlock(), afterBlock, true);
            }

            builder.setInsertPoint(afterBlock);
        } else if (stmt instanceof AbstractSyntaxTree.LoopControlStmt ctrl) {
            if (ctrl.type() == TokenType.BREAK) {
                builder.insertInstr(new TermInstr(builder.getBlock(afterBlocks.getLast()), IrInstr.OpCode.BR));
            } else if (ctrl.type() == TokenType.CONTINUE) {
                builder.insertInstr(new TermInstr(builder.getBlock(condBlocks.getLast()), IrInstr.OpCode.BR));
            } else {
                throw new RuntimeException("Invalid loop control type: " + ctrl.type());
            }
            return true;
        } else if (stmt instanceof AbstractSyntaxTree.WhileStmt loop) {
            String condBlock = builder.registerBlock(new CommentInstr("while.cond"));
            builder.insertInstr(new TermInstr(builder.getBlock(condBlock), IrInstr.OpCode.BR));

            builder.setInsertPoint(condBlock);
            Intermediate cond = calculate(loop.condition());

            String bodyBlock = builder.registerBlock(new CommentInstr("while.body"));
            String afterBlock = builder.registerBlock(new CommentInstr("while.after"));
            afterBlocks.add(afterBlock);
            condBlocks.add(condBlock);

            builder.insertInstr(new TermInstr(Intermediate.doConvert(cond, Ty.I1), builder.getBlock(bodyBlock), builder.getBlock(afterBlock), IrInstr.OpCode.BR));
            builder.setInsertPoint(bodyBlock);
            spawnStatement(loop.body(), condBlock, true);

            builder.setInsertPoint(afterBlock);
            afterBlocks.pollLast();
            condBlocks.pollLast();
        } else if (stmt instanceof AbstractSyntaxTree.ReturnStmt returnStmt) {
            if (returnStmt.returnExpr() == null) {
                builder.insertInstr(new TermInstr(IrInstr.OpCode.RET));
            } else {
                Intermediate res = calculate(returnStmt.returnExpr());
                builder.insertInstr(new TermInstr(Intermediate.doConvert(res, builder.getLastFunction().getType()), IrInstr.OpCode.RET));
            }
            return true;
        } else if (stmt instanceof AbstractSyntaxTree.Block block) {
            String newBlock = builder.registerBlock(new CommentInstr("block.new"));
            builder.insertInstr(new TermInstr(builder.getBlock(newBlock), IrInstr.OpCode.BR));
            String afterBlock = builder.registerBlock(new CommentInstr("block.after"));

            builder.setInsertPoint(newBlock);
            spawnBlock(block, afterBlock);

            builder.setInsertPoint(afterBlock);
        } else {
            throw new RuntimeException("Unknown Statement Type: " + stmt.getClass().getName());
        }
        if (terminate) {
            if (exit == null) {
                throw new RuntimeException("Terminate statement without an exit block");
            }
            builder.insertInstr(new TermInstr(builder.getBlock(exit), IrInstr.OpCode.BR));
            return true;
        }
        return false;
    }

    public Intermediate calculate(AbstractSyntaxTree.RelExpr expr) {
        Intermediate result = calculate(expr.leading());
        for (ExprWithLeading<AbstractSyntaxTree.AddExpr> addExpr : expr.follows()) {
            Intermediate itemResult = calculate(addExpr.expr());
            Intermediate[] r = Intermediate.doComparison(result, itemResult);
            Intermediate newResult = r[0];
            result = r[1];
            itemResult = r[2];
            builder.insertInstr(IrInstr.spawnComparisonOp(
                result, itemResult, addExpr.leading(), newResult, TokenType.LT, TokenType.GT, TokenType.LE, TokenType.GE
            ));
            result = newResult;
        }
        return result;
    }

    public Intermediate calculate(AbstractSyntaxTree.EqExpr expr) {
        Intermediate result = calculate(expr.leading());
        for (ExprWithLeading<AbstractSyntaxTree.RelExpr> relExpr : expr.follows()) {
            Intermediate itemResult = calculate(relExpr.expr());
            Intermediate[] r = Intermediate.doComparison(result, itemResult);
            Intermediate newResult = r[0];
            result = r[1];
            itemResult = r[2];
            builder.insertInstr(IrInstr.spawnComparisonOp(
                result, itemResult, relExpr.leading(), newResult, TokenType.EQ, TokenType.NEQ
            ));
            result = newResult;
        }
        return result;
    }

    public Intermediate orShortCircuit(List<AbstractSyntaxTree.AndExpr> items) {
        if (items.size() == 1) {
            return andShortCircuit(items.get(0).items()).getFirst();
        }
        List<String> calcBlocks = new LinkedList<>();
        calcBlocks.add(builder.getInsertPoint());
        builder.insertInstr(new CommentInstr("or.left"));
        for (int i = 1; i < items.size(); i++) {
            calcBlocks.add(builder.registerBlock(new CommentInstr("or.right." + i)));
        }
        String afterBlock = builder.registerBlock(new CommentInstr("or.after"));

        Intermediate rightestResult = null;

        for (int i = 0; i < items.size(); i++) {
            builder.setInsertPoint(calcBlocks.get(i));
            Pair<Intermediate, String> pair = andShortCircuit(items.get(i).items());
            Intermediate result = pair.getFirst();
            calcBlocks.set(i, pair.getSecond());
            if (i == items.size() - 1) {
                rightestResult = Intermediate.doConvert(result, Ty.I1);
                builder.insertInstr(new TermInstr(builder.getBlock(afterBlock), IrInstr.OpCode.BR));
            } else {
                builder.insertInstr(new TermInstr(Intermediate.doConvert(result, Ty.I1), builder.getBlock(afterBlock), builder.getBlock(calcBlocks.get(i + 1)), IrInstr.OpCode.BR));
            }
        }

        builder.setInsertPoint(afterBlock);
        Intermediate finalResult = new Intermediate(Ty.I1);

        ArrayList<Value> fromBlocks = new ArrayList<>();
        ArrayList<Value> fromValues = new ArrayList<>();
        for (int i = 0; i < items.size() - 1; i++) {
            fromBlocks.add(builder.getBlock(calcBlocks.get(i)));
            fromValues.add(new Literal(1, Ty.I1));
        }
        fromBlocks.add(builder.getBlock(calcBlocks.get(items.size() - 1)));
        fromValues.add(rightestResult);

        builder.insertInstr(new PhiInstr(finalResult, Ty.I1, fromBlocks, fromValues));

        return finalResult;
    }

    public Pair<Intermediate, String> andShortCircuit(List<AbstractSyntaxTree.EqExpr> items) {
        if (items.size() == 1) {
            return new Pair<>(calculate(items.get(0)), builder.getInsertPoint());
        }
        List<String> calcBlocks = new LinkedList<>();
        calcBlocks.add(builder.getInsertPoint());
        builder.insertInstr(new CommentInstr("and.left"));
        for (int i = 1; i < items.size(); i++) {
            calcBlocks.add(builder.registerBlock(new CommentInstr("and.right." + i)));
        }
        String afterBlock = builder.registerBlock(new CommentInstr("and.after"));

        Intermediate rightestResult = null;

        for (int i = 0; i < items.size(); i++) {
            builder.setInsertPoint(calcBlocks.get(i));
            Intermediate result = calculate(items.get(i));
            if (i == items.size() - 1) {
                rightestResult = Intermediate.doConvert(result, Ty.I1);
                builder.insertInstr(new TermInstr(builder.getBlock(afterBlock), IrInstr.OpCode.BR));
            } else {
                builder.insertInstr(new TermInstr(Intermediate.doConvert(result, Ty.I1), builder.getBlock(calcBlocks.get(i + 1)), builder.getBlock(afterBlock), IrInstr.OpCode.BR));
            }
        }

        builder.setInsertPoint(afterBlock);
        Intermediate finalResult = new Intermediate(Ty.I1);

        ArrayList<Value> fromBlocks = new ArrayList<>();
        ArrayList<Value> fromValues = new ArrayList<>();
        for (int i = 0; i < items.size() - 1; i++) {
            fromBlocks.add(builder.getBlock(calcBlocks.get(i)));
            fromValues.add(new Literal(0, Ty.I1));
        }
        fromBlocks.add(builder.getBlock(calcBlocks.get(items.size() - 1)));
        fromValues.add(rightestResult);

        builder.insertInstr(new PhiInstr(finalResult, Ty.I1, fromBlocks, fromValues));

        return new Pair<>(finalResult, afterBlock);
    }

    public Intermediate calculate(AbstractSyntaxTree.OrExpr expr) {
        return orShortCircuit(expr.items());
    }

    public Intermediate calculate(AbstractSyntaxTree.LVal lvalue) {
        // Get the direct pointer
        SymbolTableEntry entry = symbolTable.searchDown(lvalue.redirection().getTargetSymbol());
        IrType stackVarBaseType = entry.value().getType();  // 本来的类型，基本类型、数组、指针（仅函数参数）
        String stackVarBaseName = entry.emitValue().getName();
        if (stackVarBaseName.startsWith("@c")) {
            // 只考虑基本类型常量
            Constant constant = (Constant) entry.value();
            if (constant.getType() == Ty.I32) {
                return new Literal((Integer) constant.getValue());
            } else if (constant.getType() == Ty.F32) {
                return new Literal((Float) constant.getValue());
            }
        }
        if (stackVarBaseType.is("POINTER")) {
            // 这里只能是函数参数，且原本表示数组
            stackVarBaseType = new ArrayType(((PointerType) stackVarBaseType).getBaseType(), 0);
            Intermediate loadedParam = new Intermediate(new PointerType(((ArrayType) stackVarBaseType).getFinalBase()));  // 数组当成指针处理，保持 stackVarBaseType 做记录
            builder.insertInstr(new LoadInstr(
                loadedParam, loadedParam.getType(), new PointerType(stackVarBaseType), new Intermediate(stackVarBaseName + ".addr", new PointerType(stackVarBaseType))
            ));
            stackVarBaseName = loadedParam.name;
        } else if (stackVarBaseName.startsWith("%p")) {
            // 这里只是普通类型的函数参数，也不会出现索引
            Intermediate loadedParam = new Intermediate(stackVarBaseType);
            builder.insertInstr(new LoadInstr(
                loadedParam, stackVarBaseType, new PointerType(stackVarBaseType), new Intermediate(stackVarBaseName + ".addr", new PointerType(stackVarBaseType))
            ));
            panicIfFalse(lvalue.indexes().isEmpty(), "Unexpected indexes for function parameter with basic type");
            return loadedParam;
        }
        if (lvalue.indexes().isEmpty()) {
            // 没有对左值进行索引
            if (stackVarBaseType instanceof ArrayType arrayType) {
                // 数组
                return new Intermediate(stackVarBaseName, new PointerType(arrayType.getFinalBase()));
            } else {
                // 普通变量
                if (notChangedVars.containsKey(stackVarBaseName)) {
                    return notChangedVars.get(stackVarBaseName);
                }
                Intermediate loadedVar = new Intermediate(stackVarBaseType);
                builder.insertInstr(new LoadInstr(loadedVar, stackVarBaseType, new PointerType(stackVarBaseType), new Intermediate(stackVarBaseName, new PointerType(stackVarBaseType))));
                return loadedVar;
            }
        } else {
            // 对左值进行了索引
            assert stackVarBaseType instanceof ArrayType;  // Just to eliminate the warnings
            IrType baseType = ((ArrayType) stackVarBaseType).getFinalBase();
            IrType remainingType = stackVarBaseType;
            Intermediate indexIntermediate = null;
            for (AbstractSyntaxTree.AddExpr index : lvalue.indexes()) {
                Intermediate indexValue = calculate(index);
                indexValue = Intermediate.doConvert(indexValue, Ty.I32);
                Intermediate newIntermediate = new Intermediate(Ty.I32);
                builder.insertInstr(new BinaInstr(
                    indexValue, new Literal(((ArrayType) remainingType).getBase().sizeof() / baseType.sizeof()), IrInstr.OpCode.MUL, newIntermediate
                ));
                remainingType = ((ArrayType) remainingType).getBase();
                indexValue = newIntermediate;
                if (indexIntermediate == null) {
                    indexIntermediate = indexValue;
                } else {
                    newIntermediate = new Intermediate(Ty.I32);
                    builder.insertInstr(new BinaInstr(
                        indexIntermediate, indexValue, IrInstr.OpCode.ADD, newIntermediate
                    ));
                    indexIntermediate = newIntermediate;
                }
            }
            Intermediate pointer = new Intermediate(new PointerType(baseType));
            builder.insertInstr(new BinaInstr(
                new Intermediate(stackVarBaseName, new PointerType(baseType)),
                indexIntermediate, IrInstr.OpCode.ADD, pointer
            ));
            if (remainingType instanceof ArrayType) {
                // Remain a pointer
                return pointer;
            } else {
                // Get the final value
                Intermediate loadedVar = new Intermediate(baseType);
                builder.insertInstr(new LoadInstr(loadedVar, baseType, new PointerType(baseType), pointer));
                return loadedVar;
            }
        }
    }

    public Intermediate calculate(AbstractSyntaxTree.PrimaryExpr expr) {
        if (expr.body() instanceof AbstractSyntaxTree.AddExpr addExpr) {
            return calculate(addExpr);
        } else if (expr.body() instanceof AbstractSyntaxTree.LVal lvalue) {
            return calculate(lvalue);
        } else if (expr.body() instanceof AbstractSyntaxTree.Number number) {
            if (number.valueInt() == null) {
                return new Literal(number.valueFloat());
            }
            return new Literal(number.valueInt());
        } else {
            throw new RuntimeException("Unexpected Primary body type: " + expr.body().getClass().getName());
        }
    }

    public Intermediate calculate(AbstractSyntaxTree.UnaryExpr expr) {
        Intermediate result;
        if (expr.body() instanceof AbstractSyntaxTree.PrimaryExpr primary) {
            result = calculate(primary);
        } else if (expr.body() instanceof AbstractSyntaxTree.FuncCall funcCall) {
            SymbolTableEntry entry;
            entry = symbolTable.searchDown(new SymbolId(funcCall.funcName(), -2));
            if (entry == null) {
                // Extern Function
                entry = symbolTable.searchDown(new SymbolId("@" + funcCall.funcName(), -1));
            }
            List<Value> args = new LinkedList<>();
            for (int i = 0; i < funcCall.args().size(); i++) {
                Intermediate arg = calculate(funcCall.args().get(i));
                args.add(Intermediate.doConvert(arg, ((Function) entry.value()).getArguments().get(i).getType()));
            }
            if (entry.value().getType().is("VOID")) {
                result = null;
            } else {
                result = new Intermediate(entry.value().getType());
            }
            builder.insertInstr(new TermInstr(entry.value(), args, IrInstr.OpCode.CALL, result));
        } else {
            throw new RuntimeException("Unexpected Unary body type: " + expr.body().getClass().getName());
        }
        for (int i = expr.leadingSymbols().size() - 1; i >= 0; i--) {
            TokenType token = expr.leadingSymbols().get(i);
            assert result != null;
            switch (token) {
                case ADD -> {}
                case SUB -> {
                    if (result.type.is("i1")) {
                        result = Intermediate.doConvert(result, Ty.I32);
                    }
                    Intermediate newResult = new Intermediate(result.type);
                    builder.insertInstr(IrInstr.spawnBinaryOp(
                        Literal.zero(result.type), result, TokenType.SUB, newResult, TokenType.SUB
                    ));
                    result = newResult;
                }
                case NOT -> {
                    if (result.type.is("i1")) {
                        Intermediate newResult = new Intermediate(result.type);
                        builder.insertInstr(new NotInstr(result, newResult));
                        result = newResult;
                    } else {
                        Intermediate newResult = new Intermediate(Ty.I1);
                        if (result.type.is("INT")) {
                            builder.insertInstr(new ICmpInstr(result, Literal.zero(result.type), ICmpInstr.Predicate.eq, newResult));
                        } else if (result.type.is("FLOAT")) {
                            builder.insertInstr(new FCmpInstr(result, Literal.zero(result.type), FCmpInstr.Predicate.oeq, newResult));
                        } else {
                            throw new RuntimeException("Unexpected type for NOT: " + result.type);
                        }
                        result = newResult;
                    }
                }
                default -> throw new RuntimeException("Unexpected op: " + token);
            }
        }
        return result;
    }

    public Intermediate calculate(AbstractSyntaxTree.MulExpr expr) {
        Intermediate result = calculate(expr.leading());
        for (ExprWithLeading<AbstractSyntaxTree.UnaryExpr> unaryExpr : expr.follows()) {
            Intermediate itemResult = calculate(unaryExpr.expr());
            Intermediate[] r = Intermediate.doArithmetic(result, itemResult);
            Intermediate newResult = r[0];
            result = r[1];
            itemResult = r[2];
            builder.insertInstr(IrInstr.spawnBinaryOp(
                result, itemResult, unaryExpr.leading(), newResult, TokenType.MUL, TokenType.DIV, TokenType.MOD
            ));
            result = newResult;
        }
        return result;
    }

    public Intermediate calculate(AbstractSyntaxTree.AddExpr expr) {
        Intermediate result = calculate(expr.leading());
        for (ExprWithLeading<AbstractSyntaxTree.MulExpr> mulExpr : expr.follows()) {
            Intermediate itemResult = calculate(mulExpr.expr());
            Intermediate[] r = Intermediate.doArithmetic(result, itemResult);
            Intermediate newResult = r[0];
            result = r[1];
            itemResult = r[2];
            builder.insertInstr(IrInstr.spawnBinaryOp(
                result, itemResult, mulExpr.leading(), newResult, TokenType.ADD, TokenType.SUB
            ));
            result = newResult;
        }
        return result;
    }
}
