package ir.traversal;

import frontend.ast.AbstractSyntaxTree;
import frontend.ast.ExprWithLeading;
import frontend.lexer.TokenType;
import ir.traversal.SymbolTable.SymbolTableEntry;
import ir.traversal.SymbolTable.SymbolId;
import ir.type.ArrayType;
import ir.type.IrType;
import ir.type.PointerType;
import ir.type.Ty;
import ir.value.*;
import utils.Calculator;
import utils.GlobalCounter;
import utils.Panic;

import java.util.*;

public class ScopeSpecifier {
    private final List<AbstractSyntaxTree.CompileUnit> compileUnits;
    private SymbolTable currentTable = null;

    public ScopeSpecifier(List<AbstractSyntaxTree.CompileUnit> compileUnits) {
        this.compileUnits = compileUnits;
    }

    /**
     * 遍历 AST，构建符号表，返回全局符号表
     *
     * @return 全局符号表，递归地包含所有子符号表
     */
    public SymbolTable traverse() {
        SymbolTable globalTable = new SymbolTable(null);
        currentTable = globalTable;
        for (AbstractSyntaxTree.CompileUnit compileUnit : compileUnits) {
            if (compileUnit instanceof AbstractSyntaxTree.FuncDecl) {
                globalTable.put(traverse((AbstractSyntaxTree.FuncDecl) compileUnit));
            } else if (compileUnit instanceof AbstractSyntaxTree.VarDecl) {
                traverse((AbstractSyntaxTree.VarDecl) compileUnit, true);
            } else {
                throw new RuntimeException("Invalid CompileUnit At Top Level: " + compileUnit);
            }
        }
        return globalTable;
    }

    /**
     * 遍历 Block，构建子符号表
     *
     * @param block  待遍历的 Block
     * @param params 函数参数列表，如果为空，则不是函数的最大的块
     * @param func   函数对象，如果为空，则不是函数的最大的块
     */
    private void traverse(AbstractSyntaxTree.Block block, List<AbstractSyntaxTree.FuncParam> params, Function func) {
        currentTable = new SymbolTable(currentTable, block);
        block.symbols().set(currentTable);

        for (AbstractSyntaxTree.FuncParam param : params) {
            Value value;
            IrType type;
            if (param.type() == TokenType.INT) {
                value = new Variable(param.ident(), 0);
                type = Ty.I32;
            } else if (param.type() == TokenType.FLOAT) {
                value = new Variable(param.ident(), 0f);
                type = Ty.F32;
            } else {
                throw new RuntimeException("Invalid Type: " + param.type());
            }
            if (!param.indexes().isEmpty()) {
                // Array
                if (param.indexes().get(0) != null) {
                    throw new RuntimeException("The first index must be empty!");
                }
                for (int i = 1; i < param.indexes().size(); i++) {
                    type = new ArrayType(type, (int) calculate(param.indexes().get(i), 0));
                }
                type = new PointerType(type);
                value = new Variable(param.ident(), type, null);
            }
            SymbolTableEntry entry = new SymbolTableEntry(
                GlobalCounter.gen("p"), value, false, param
            );
            func.addArgument(new Argument(entry.emitValue()));
            currentTable.put(entry);
            param.redirection().setTargetSymbol(entry.id());
        }

        for (AbstractSyntaxTree.BlockItem item : block.items()) {
            if (item instanceof AbstractSyntaxTree.VarDecl) {
                traverse((AbstractSyntaxTree.VarDecl) item, false);
            } else if (item instanceof AbstractSyntaxTree.Statement stmt) {
                traverse(stmt);
            } else {
                throw new RuntimeException("Invalid BlockItem: " + item);
            }
        }

        currentTable = currentTable.getParent();  // Restore currentTable
    }

    private void traverse(AbstractSyntaxTree.Statement stmt) {
        if (stmt instanceof AbstractSyntaxTree.Block subBlock) {
            traverse(subBlock, List.of(), null);
        } else if (stmt instanceof AbstractSyntaxTree.ExprStmt expr) {
            if (expr.expr() != null) {
                calculate(expr.expr(), 0);
            }
        } else if (stmt instanceof AbstractSyntaxTree.AssignStmt assign) {
            calculate(assign.lvalue(), true);
            calculate(assign.rvalue(), 0);
        } else if (stmt instanceof AbstractSyntaxTree.BranchStmt branch) {
            traverse(branch.condition());
            traverse(branch.thenBlock());
            if (branch.elseBlock() != null) {
                traverse(branch.elseBlock());
            }
        } else if (stmt instanceof AbstractSyntaxTree.WhileStmt loop) {
            traverse(loop.condition());
            traverse(loop.body());
        } else if (stmt instanceof AbstractSyntaxTree.LoopControlStmt ctrl) {
            Panic.panicIfFalse(ctrl.type() == TokenType.BREAK || ctrl.type() == TokenType.CONTINUE, "loop control error");
        } else if (stmt instanceof AbstractSyntaxTree.ReturnStmt ret) {
            if (ret.returnExpr() != null) {
                calculate(ret.returnExpr(), 0);
            }
        }
    }

    /**
     * 仅仅遍历条件表达式，找出里面涉及的变量，进行重定向
     * @param expr 条件表达式的 AST 节点
     */
    private void traverse(AbstractSyntaxTree.OrExpr expr) {
        for (AbstractSyntaxTree.AndExpr andExpr : expr.items()) {
            for (AbstractSyntaxTree.EqExpr eqExpr : andExpr.items()) {
                traverse(eqExpr.leading());
                for (ExprWithLeading<AbstractSyntaxTree.RelExpr> follow : eqExpr.follows()) {
                    traverse(follow.expr());
                }
            }
        }
    }

    private void traverse(AbstractSyntaxTree.RelExpr expr) {
        calculate(expr.leading(), 0);
        for (ExprWithLeading<AbstractSyntaxTree.AddExpr> follow : expr.follows()) {
            calculate(follow.expr(), 0);
        }
    }

    /**
     * 处理变量声明，构建符号表项
     *
     * @param varDecl AST 节点
     * @param global  是否为全局变量
     */
    private void traverse(AbstractSyntaxTree.VarDecl varDecl, boolean global) {
        boolean isConst = varDecl.isConst();
        String symbolType = isConst ? "c" : "v";
        IrType type = switch (varDecl.type()) {
            case INT -> Ty.I32;
            case FLOAT -> Ty.F32;
            default -> throw new RuntimeException("Invalid Type: " + varDecl.type());
        };
        for (AbstractSyntaxTree.VarDef varDef : varDecl.decls()) {
            if (varDef.indexes().isEmpty()) {
                // Just a normal variable
                Value value;
                if (varDef.init() == null) {
                    value = getDefaultViaType(varDef.ident(), type, isConst);
                } else {
                    value = calculate(varDef.init().expr(), getBaseViaType(type), varDef.ident(), isConst);
                }
                SymbolTableEntry entry = new SymbolTableEntry(
                    GlobalCounter.gen(symbolType), value, isConst || global, varDef.init()
                );
                currentTable.put(entry);
                varDef.redirection().setTargetSymbol(entry.id());
                String prefix = entry.global() ? "@" : "%";
                ((Constant) entry.value()).mangle(prefix + entry.id());
            } else {
                // Array
                IrType arrayType = type;
                for (int i = varDef.indexes().size() - 1; i >= 0; i--) {
                    AbstractSyntaxTree.AddExpr size = varDef.indexes().get(i);
                    int result = (Integer) calculate(size, 0);
                    arrayType = new ArrayType(arrayType, result);
                }
                TreeMap<Integer, Object> values = new TreeMap<>();
                if (varDef.init() == null) {
                    arrayFill(values, ((ArrayType) arrayType).getDimension(), 0, 0, type, new LinkedList<>());
                } else {
                    arrayFill(values, ((ArrayType) arrayType).getDimension(), 0, 0, type, varDef.init().values());
                }
                Value value;
                if (isConst) {
                    value = new Constant(varDef.ident(), arrayType, values);
                } else {
                    value = new Variable(varDef.ident(), arrayType, values);
                }
                SymbolTableEntry entry = new SymbolTableEntry(
                    GlobalCounter.gen(symbolType), value, isConst || global, varDef.init()
                );
                currentTable.put(entry);
                varDef.redirection().setTargetSymbol(entry.id());
                String prefix = entry.global() ? "@" : "%";
                ((Constant) entry.value()).mangle(prefix + entry.id());
            }
        }
    }

    private int arrayFill(Map<Integer, Object> values, List<Integer> sizes, int curIndex, int curDepth, IrType irType,
                          List<AbstractSyntaxTree.InitVal> initVals) {
        int needToFill = sizes.get(curDepth);
        for (int i = curDepth + 1; i < sizes.size(); i++) {
            needToFill *= sizes.get(i);
        }
        needToFill += curIndex;
        for (AbstractSyntaxTree.InitVal initVal : initVals) {
            if (initVal.expr() != null) {
                // Just a normal variable
                Object res = calculate(initVal.expr(), getBaseViaType(irType));
                curIndex++;
                if (irType.is("INT") && res instanceof Integer && (Integer) res == 0) {
                    continue;
                } else if (irType.is("FLOAT") && res instanceof Float && (Float) res == 0) {
                    continue;
                }
                // 能算，res 不是 null 的话，直接填入；否则，填入初始表达式 expr
                values.put(curIndex - 1, Objects.requireNonNullElseGet(res, initVal::expr));
            } else {
                // Deeper array
                curIndex = arrayFill(values, sizes, curIndex, curDepth + 1, irType, initVal.values());
            }
        }
        return needToFill;
    }

    /**
     * 处理函数声明，构建符号表项
     *
     * @param funcDecl AST 节点
     * @return 符号表项
     */
    private SymbolTableEntry traverse(AbstractSyntaxTree.FuncDecl funcDecl) {
        IrType returnType = switch (funcDecl.returnType()) {
            case INT -> Ty.I32;
            case FLOAT -> Ty.F32;
            case VOID -> Ty.VOID;
            default -> throw new RuntimeException("Invalid Type: " + funcDecl.returnType());
        };
        Function value = new Function(funcDecl.funcName(), returnType);
        SymbolTableEntry entry = new SymbolTableEntry(
            new SymbolId(funcDecl.funcName(), -2), value, true, funcDecl
        );

        traverse(funcDecl.body(), funcDecl.params(), value);
        value.setBody(funcDecl.body());

        return entry;
    }

    /**
     * 计算初始化表达式的值
     *
     * @param expr 初始化表达式的 AST 节点
     * @return 初始化表达式的值，值内部是 Integer 或 Float
     */
    private Value calculate(AbstractSyntaxTree.AddExpr expr, Object _base, String name, boolean isConst) {
        Object value = calculate(expr, _base);

        if (value instanceof Integer || (value == null && _base instanceof Integer)) {
            return isConst ? new Constant(name, (Integer) value) : new Variable(name, (Integer) value);
        } else if (value instanceof Float || (value == null && _base instanceof Float)) {
            return isConst ? new Constant(name, (Float) value) : new Variable(name, (Float) value);
        } else {
            throw new RuntimeException("Invalid Value or Type: " + value);
        }
    }

    /**
     * 尝试计算加法表达式的值
     *
     * @param expr  待计算的表达式
     * @param _base 表明类型的对象，转换链最低的一级是 Integer
     * @return 计算结果，为 Integer，Float 或 null
     */
    private Object calculate(AbstractSyntaxTree.AddExpr expr, Object _base) {
        Object value = calculate(expr.leading(), _base);

        for (ExprWithLeading<AbstractSyntaxTree.MulExpr> follow : expr.follows()) {
            value = switch (follow.leading()) {
                case ADD -> Calculator.Add(value, calculate(follow.expr(), _base));
                case SUB -> Calculator.Sub(value, calculate(follow.expr(), _base));
                default -> throw new RuntimeException("Invalid Operator: " + follow.leading());
            };
        }

        if (_base instanceof Integer && value instanceof Float) {
            value = ((Float) value).intValue();
        }
        if (_base instanceof Float && value instanceof Integer) {
            value = ((Integer) value).floatValue();
        }
        return value;
    }

    private Object calculate(AbstractSyntaxTree.MulExpr expr, Object _base) {
        Object value = calculate(expr.leading(), _base);

        for (ExprWithLeading<AbstractSyntaxTree.UnaryExpr> follow : expr.follows()) {
            value = switch (follow.leading()) {
                case MUL -> Calculator.Mul(value, calculate(follow.expr(), _base));
                case DIV -> Calculator.Div(value, calculate(follow.expr(), _base));
                case MOD -> Calculator.Mod(value, calculate(follow.expr(), _base));
                default -> throw new RuntimeException("Invalid Operator: " + follow.leading());
            };
        }

        return value;
    }

    private Object calculate(AbstractSyntaxTree.UnaryExpr expr, Object _base) {
        Object value;
        if (expr.body() instanceof AbstractSyntaxTree.FuncCall call) {
            for (AbstractSyntaxTree.AddExpr arg : call.args()) {
                calculate(arg, 0);
            }
            return null;
        }
        AbstractSyntaxTree.PrimaryExpr primaryExpr = (AbstractSyntaxTree.PrimaryExpr) expr.body();
        if (primaryExpr.body() instanceof AbstractSyntaxTree.LVal lvalue) {
            value = calculate(lvalue, false);
        } else if (primaryExpr.body() instanceof AbstractSyntaxTree.Number number) {
            value = number.valueInt() == null ? (Object) number.valueFloat() : (Object) number.valueInt();
        } else if (primaryExpr.body() instanceof AbstractSyntaxTree.AddExpr addExpr) {
            value = ((Variable) calculate(addExpr, _base, null, false)).getValue();
        } else {
            throw new RuntimeException("Invalid PrimaryExpr: " + primaryExpr.body());
        }

        for (int i = expr.leadingSymbols().size() - 1; i >= 0; i--) {
            TokenType token = expr.leadingSymbols().get(i);
            switch (token) {
                case ADD -> {}
                case SUB -> value = Calculator.Neg(value);
                case NOT -> value = Calculator.Not(value);
                default -> throw new RuntimeException("Invalid Unary Operator: " + token);
            }
        }

        return value;
    }

    private Object calculate(AbstractSyntaxTree.LVal lvalue, boolean isAssign) {
        SymbolTableEntry entry = currentTable.search(lvalue.ident());
        Value value = entry.value();
        lvalue.redirection().setTargetSymbol(entry.id());
        Object resultValue;
        if (value instanceof Variable) {
            if (isAssign) {
                ((Variable) value).setDirty();
            }
            resultValue = null;
        } else if (value instanceof Constant constant) {
            if (isAssign) {
                throw new RuntimeException("Assign to a constant!");
            }
            resultValue = constant.getValue();
        } else {
            throw new RuntimeException("Invalid Value: " + value);
        }
        if (lvalue.indexes().isEmpty()) {
            return resultValue;
        } else {
            List<Integer> indexes = new ArrayList<>();
            boolean hasNonCalculable = false;
            for (AbstractSyntaxTree.AddExpr expr : lvalue.indexes()) {
                Object index = calculate(expr, 1);
                if (index == null) {
                    hasNonCalculable = true;
                } else {
                    if (index instanceof Float f) {
                        index = f.intValue();
                    }
                    indexes.add((int) index);
                }
            }
            if (hasNonCalculable || resultValue == null) {
                return null;
            }
            List<Integer> dimension = ((ArrayType) value.getType()).getDimension();
            int index = 0;
            for (int i = 0; i < indexes.size(); i++) {
                if (indexes.get(i) < 0 || indexes.get(i) >= dimension.get(i)) {
                    throw new RuntimeException("Index Out Of Bound: " + indexes.get(i));
                }
                index = index * dimension.get(i) + indexes.get(i);
            }
            return ((TreeMap<Integer, Object>) resultValue).getOrDefault(index,
                getBaseViaType(((ArrayType) value.getType()).getFinalBase()));
        }
    }

    /**
     * 解决 Java 糟糕的泛型：获取基础类型的值
     *
     * @param type 类型
     * @return 基础值的制定包装类型，Integer 或 Float
     */
    private Object getBaseViaType(IrType type) {
        if (type.is("INT")) {
            return 0;
        } else if (type.is("FLOAT")) {
            return (float) 0;
        } else {
            throw new RuntimeException("Invalid Type: " + type);
        }
    }

    /**
     * 获取变量们的默认值，根据类型
     *
     * @param name    变量名
     * @param type    变量类型
     * @param isConst 是否为常量
     * @return 默认值，Value 类型（Constant 或者 Variable）
     */
    private Value getDefaultViaType(String name, IrType type, boolean isConst) {
        if (type.is("INT")) {
            return isConst ? new Constant(name, 0) : new Variable(name, 0);
        } else if (type.is("FLOAT")) {
            return isConst ? new Constant(name, 0f) : new Variable(name, 0f);
        } else {
            throw new RuntimeException("Invalid Type: " + type);
        }
    }
}
