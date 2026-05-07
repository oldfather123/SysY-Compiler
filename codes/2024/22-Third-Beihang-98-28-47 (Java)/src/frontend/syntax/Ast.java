//TODO: 暂用于parser
package frontend.syntax;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.symbols.ArrayInitVal;
import frontend.ir.symbols.SymTab;
import frontend.ir.symbols.Symbol;
import frontend.ir.constvalue.ConstInt;
import frontend.lexer.Token;
import frontend.lexer.TokenType;

import java.util.ArrayList;
import java.util.List;

/**
 * 所有的语法树节点
 * 为简化编译器实现难度, 对文法进行了改写(不影响语义)
 */
public class Ast {

    public ArrayList<CompUnit> nodes;
    boolean printPermission;

    // CompUnit -> Decl | FuncDef
    public interface CompUnit {
    }

    // Decl -> ['const'] 'int' Def {',' Def} ';'
    public static class Decl implements CompUnit, BlockItem {

        private final boolean constant;
        private Token type;
        private ArrayList<Def> defs;

        public Decl(boolean constant, Token bType, ArrayList<Def> defs) {
            this.constant = constant;
            this.type = bType;
            this.defs = defs;
            assert bType != null;
            assert defs != null;
        }

        public boolean isConst() {
            return constant;
        }

        public Token getType() {
            return type;
        }

        public List<Def> getDefList() {
            return this.defs;
        }
    }

    // Def -> Ident {'[' Exp ']'} ['=' Init]
    public static class Def {

        private TokenType type;
        private Token ident;
        private ArrayList<Exp> indexList;
        private Init init;

        public Def(TokenType type, Token ident, ArrayList<Exp> indexList, Init init) {
            this.type = type;
            this.ident = ident;
            this.indexList = indexList;
            this.init = init;
            assert type != null;
            assert ident != null;
            assert indexList != null;
        }

        public TokenType getType() {
            return type;
        }

        public Token getIdent() {
            return ident;
        }

        public List<Exp> getIndexList() {
            return indexList;
        }

        public Init getInit() {
            return init;
        }
    }

    // Init -> Exp | InitArray
    public interface Init {
    }

    // InitArray -> '{' [ Init { ',' Init } ] '}'
    public static class InitArray implements Init {
        public ArrayList<Init> initList;

        public InitArray(ArrayList<Init> initList) {
            this.initList = initList;
            assert initList != null;
        }

        public List<Init> getInitList() {
            return initList;
        }
    }

    // FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
    // FuncFParams -> FuncFParam {',' FuncFParam}
    public static class FuncDef implements CompUnit {

        public Token type; // FuncType
        public Token ident; // name
        public ArrayList<FuncFParam> params;
        public Block body;

        public FuncDef(Token type, Token ident, ArrayList<FuncFParam> params, Block body) {
            this.type = type;
            this.ident = ident;
            this.params = params;
            this.body = body;
            assert type != null;
            assert ident != null;
            assert params != null;
            assert body != null;
            // 为了保证函数的最后有一条 return 语句
            body.ensureReturn(type.getType());
        }

        public Token getIdent() {
            return ident;
        }

        public Token getType() {
            return type;
        }

        public List<FuncFParam> getFParams() {
            return params;
        }

        public Block getBody() {
            return body;
        }
    }

    // FuncFParam -> BType Ident ['[' ']' { '[' Exp ']' }]
    public static class FuncFParam {

        private final Token type;
        private final Token ident;
        private final boolean array; // whether it is an array
        private final ArrayList<Exp> arrayItemList; // array sizes of each dim
        public static final FuncFParam INT_PARAM = new FuncFParam(
                new Token(TokenType.INT, "0"),
                new Token(TokenType.IDENT, ""),
                false, null
        );
        public static final FuncFParam FLOAT_PARAM = new FuncFParam(
                new Token(TokenType.FLOAT, "0"),
                new Token(TokenType.IDENT, ""),
                false, null
        );

        public FuncFParam(Token type, Token ident, boolean array, ArrayList<Exp> arrayItemList) {
            this.type = type;
            this.ident = ident;
            this.array = array;
            this.arrayItemList = arrayItemList;
            assert type != null;
            assert ident != null;
            assert arrayItemList != null;
        }


        public Token getType() {
            return type;
        }

        public String getName() {
            return ident.getContent();
        }

        public boolean isArray() {
            return array;
        }

        public List<Exp> getArrayItemList() {
            return arrayItemList;
        }
    }

    // Block
    public static class Block implements Stmt {
        public ArrayList<BlockItem> items;

        public Block(ArrayList<BlockItem> items) {
            this.items = items;
            assert items != null;
        }

        public ArrayList<BlockItem> getItems() {
            return items;
        }

        public void ensureReturn(TokenType returnType) {
            if (returnType == null) {
                throw new NullPointerException();
            }
            if (!this.items.isEmpty() && this.items.get(items.size() - 1) instanceof Return) {
                return;
            }
            Return returnStmt;
            switch (returnType) {
                case INT:
                    Number returnInt = new Number(new Token(TokenType.DEC_INT, "0"));
                    returnStmt = new Return(new UnaryExp(new ArrayList<>(), returnInt));
                    break;
                case FLOAT:
                    Number returnFloat = new Number(new Token(TokenType.DEC_FLOAT, "0.0"));
                    returnStmt = new Return(new UnaryExp(new ArrayList<>(), returnFloat));
                    break;
                case VOID:
                    returnStmt = new Return(null);
                    break;
                default:
                    throw new RuntimeException("出现了意想不到的返回值类型");
            }
            this.items.add(returnStmt);
        }
    }

    // BlockItem -> Decl | Stmt
    public interface BlockItem {
    }

    // Stmt -> Assign | ExpStmt | Block | IfStmt | WhileStmt | Break | Continue | Return
    public interface Stmt extends BlockItem {
    }

    // Assign
    public static class Assign implements Stmt {

        public LVal left;
        public Exp right;

        public Assign(LVal left, Exp right) {
            this.left = left;
            this.right = right;
            assert left != null;
            assert right != null;
        }

        public LVal getLVal() {
            return left;
        }

        public Exp getExp() {
            return right;
        }
    }

    // ExpStmt
    public static class ExpStmt implements Stmt {
        private final Exp exp; // nullable, empty stmt if null

        public ExpStmt(Exp exp) {
            this.exp = exp;
        }

        public Exp getExp() {
            return exp;
        }
    }

    // IfStmt
    public static class IfStmt implements Stmt {

        public Exp condition;
        public Stmt thenStmt;
        public Stmt elseStmt;

        public IfStmt(Exp condition, Stmt thenTarget, Stmt elseTarget) {
            this.condition = condition;
            this.thenStmt = thenTarget;
            this.elseStmt = elseTarget;
            assert condition != null;
            assert thenTarget != null;
        }
    }

    // WhileStmt
    public static class WhileStmt implements Stmt {

        public Exp cond;
        public Stmt body;

        public WhileStmt(Exp cond, Stmt body) {
            this.cond = cond;
            this.body = body;
            assert cond != null;
            assert body != null;
        }
    }

    // Break
    public static class Break implements Stmt {
        public Break() {
        }
    }

    // Continue
    public static class Continue implements Stmt {
        public Continue() {
        }
    }

    // Return
    public static class Return implements Stmt {
        public Exp returnValue;

        public Return(Exp returnValue) {
            this.returnValue = returnValue;
        }

        public Exp getReturnValue() {
            return returnValue;
        }
    }

    // PrimaryExp -> Call | '(' Exp ')' | LVal | Number
    // Init -> Exp | InitArray
    // Exp -> BinaryExp | UnaryExp
    public interface Exp extends Init, PrimaryExp {
        DataType checkConstType(SymTab symTab);  // 约定：如果表达式不是常量，返回 VOID；如果是常量，则返回对应的数据类型

        Integer getConstInt(SymTab symTab);

        Float getConstFloat(SymTab symTab);
    }

    // BinaryExp: Arithmetic, Relation, Logical
    // BinaryExp -> Exp { Op Exp }, calc from left to right
    public static class BinaryExp implements Exp {

        private Exp firstExp;
        private ArrayList<Token> ops;
        private ArrayList<Exp> RestExps;

        public BinaryExp(Exp firstExp, ArrayList<Token> ops, ArrayList<Exp> RestExps) {
            this.firstExp = firstExp;
            this.ops = ops;
            this.RestExps = RestExps;
            assert firstExp != null;
            assert ops != null;
            assert RestExps != null;
        }

        public Exp getFirstExp() {
            return firstExp;
        }

        public List<Token> getOps() {
            return ops;
        }

        public List<Exp> getRestExps() {
            return RestExps;
        }

        @Override
        public DataType checkConstType(SymTab symTab) {
            DataType constType = firstExp.checkConstType(symTab);
            if (constType == DataType.VOID) {
                return DataType.VOID;
            }
            for (Exp exp : RestExps) {
                switch (exp.checkConstType(symTab)) {
                    case VOID:
                        return DataType.VOID;
                    case FLOAT:
                        constType = DataType.FLOAT;
                }
            }
            return constType;
        }

        @Override
        public Integer getConstInt(SymTab symTab) {
            Integer res = firstExp.getConstInt(symTab);
            if (res == null) {
                return null;
            }
            for (int i = 0; i < RestExps.size(); i++) {
                Exp exp = RestExps.get(i);
                Token op = ops.get(i);
                Integer num = exp.getConstInt(symTab);
                if (num == null) {
                    return null;
                }
                switch (op.getType()) {
                    case ADD:
                        res += num;
                        break;
                    case SUB:
                        res -= num;
                        break;
                    case MUL:
                        res *= num;
                        break;
                    case DIV:
                        res /= num;
                        break;
                    case MOD:
                        res %= num;
                        break;
                    case LAND:
                        res = (res != 0 && num != 0) ? 1 : 0;
                        break;
                    case LOR:
                        res = (res != 0 || num != 0) ? 1 : 0;
                        break;
                    case LE:
                        res = (res <= num) ? 1 : 0;
                        break;
                    case LT:
                        res = (res < num) ? 1 : 0;
                        break;
                    case GE:
                        res = (res >= num) ? 1 : 0;
                        break;
                    case GT:
                        res = (res > num) ? 1 : 0;
                        break;
                    case EQ:
                        res = (res.equals(num)) ? 1 : 0;
                        break;
                    case NE:
                        res = (!res.equals(num)) ? 1 : 0;
                        break;
                    default:
                        throw new RuntimeException("整数常量表达式中出现了未曾设想的运算符");
                }
            }
            return res;
        }

        @Override
        public Float getConstFloat(SymTab symTab) {
            Float res = firstExp.getConstFloat(symTab);
            if (res == null) {
                return null;
            }
            for (int i = 0; i < RestExps.size(); i++) {
                Exp exp = RestExps.get(i);
                Token op = ops.get(i);
                Float num = exp.getConstFloat(symTab);
                if (num == null) {
                    return null;
                }
                switch (op.getType()) {
                    case ADD:
                        res += num;
                        break;
                    case SUB:
                        res -= num;
                        break;
                    case MUL:
                        res *= num;
                        break;
                    case DIV:
                        res /= num;
                        break;
                    case MOD:
                        res %= num;
                        break;
                    case LAND:
                        res = (res != 0 && num != 0) ? 1f : 0f;
                        break;
                    case LOR:
                        res = (res != 0 || num != 0) ? 1f : 0f;
                        break;
                    case LE:
                        res = (res <= num) ? 1f : 0f;
                        break;
                    case LT:
                        res = (res < num) ? 1f : 0f;
                        break;
                    case GE:
                        res = (res >= num) ? 1f : 0f;
                        break;
                    case GT:
                        res = (res > num) ? 1f : 0f;
                        break;
                    case EQ:
                        res = (res.equals(num)) ? 1f : 0f;
                        break;
                    case NE:
                        res = (!res.equals(num)) ? 1f : 0f;
                        break;
                    default:
                        throw new RuntimeException("浮点数常量表达式中出现了未曾设想的运算符");
                }
            }
            return res;
        }
    }

    // UnaryExp -> {UnaryOp} PrimaryExp
    public static class UnaryExp implements Exp {
        
        private final ArrayList<Token> ops;
        private final PrimaryExp primary;
        private final int sign;
        private final boolean not;

        public UnaryExp(ArrayList<Token> ops, PrimaryExp primary) {
            this.ops = ops;
            this.primary = primary;
            assert ops != null;
            assert primary != null;
            sign = getMySign();
            not = checkMyNot();
        }

        public List<Token> getUnaryOps() {
            return ops;
        }

        public PrimaryExp getPrimaryExp() {
            return primary;
        }
        
        public int getSign() {
            return sign;
        }
        
        public boolean checkNot() {
            return not;
        }
        
        private int getMySign() {
            int sign = 1;
            TokenType type;
            for (Token op : ops) {
                type = op.getType();
                if (type == TokenType.NOT) {
                    break;
                }
                if (type == TokenType.SUB) {
                    sign *= -1;
                }
            }
            return sign;
        }
        
        private boolean checkMyNot() {
            boolean not = false;
            for (Token op : ops) {
                if (op.getType() == TokenType.NOT) {
                    not = !not;
                }
            }
            return not;
        }

        @Override
        public DataType checkConstType(SymTab symTab) {
            if (primary instanceof Exp) {
                return ((Exp) primary).checkConstType(symTab);
            } else if (primary instanceof Call) {
                return DataType.VOID;
            } else if (primary instanceof LVal) {
                Symbol symbol = symTab.getSym(((LVal) primary).getName());
                for (Exp index : ((LVal) primary).getIndexList()) {
                    if (index.checkConstType(symTab) != DataType.INT) {
                        return DataType.VOID;
                    }
                }
                if (symbol.isConstant() || symTab.isGlobal() && symbol.isGlobal()) {
                    return symbol.getType();
                } else {
                    return DataType.VOID;
                }
            } else if (primary instanceof Number) {
                if (((Number) primary).isIntConst) {
                    return DataType.INT;
                } else if (((Number) primary).isFloatConst) {
                    return DataType.FLOAT;
                } else {
                    throw new RuntimeException("出现了未定义的数值常量类型");
                }
            } else {
                throw new RuntimeException("出现了未定义的基本表达式");
            }
        }

        @Override
        public Integer getConstInt(SymTab symTab) {
            int sign = getSign();
            int ret;
            if (primary instanceof Exp) {
                ret = ((Exp) primary).getConstInt(symTab) * sign;
            } else if (primary instanceof Call) {
                throw new RuntimeException();
            } else if (primary instanceof LVal) {
                Symbol symbol = symTab.getSym(((LVal) primary).getName());
                if (symbol.isConstant() || symTab.isGlobal() && symbol.isGlobal()) {
                    Value initVal = symbol.getInitVal();
                    if (symbol.isArray()) {
                        ArrayList<Integer> indexList = new ArrayList<>();
                        for (Exp index : ((LVal) primary).getIndexList()) {
                            indexList.add(index.getConstInt(symTab));
                        }
                        if (initVal instanceof ArrayInitVal) {
                            Value val = ((ArrayInitVal) initVal).getValueWithIndex(indexList);
                            if (val instanceof ConstInt) {
                                ret = ((ConstInt) val).getNumber() * sign;
                            } else {
                                throw new RuntimeException();
                            }
                        } else {
                            throw new RuntimeException();
                        }
                    } else {
                        if (initVal instanceof ConstInt) {
                            ret = initVal.getNumber().intValue() * sign;
                        } else {
                            throw new RuntimeException();
                        }
                    }
                } else {
                    throw new RuntimeException();
                }
            } else if (primary instanceof Number) {
                if (((Number) primary).isIntConst) {
                    ret = ((Number) primary).getIntConstValue() * sign;
                } else if (((Number) primary).isFloatConst) {
                    throw new RuntimeException();
                } else {
                    throw new RuntimeException("出现了未定义的数值常量类型");
                }
            } else {
                throw new RuntimeException("出现了未定义的基本表达式");
            }
            if (checkNot()) {
                if (getSign() > 0) {
                    ret = ret != 0 ? 0 : 1;
                } else {
                    ret = ret != 0 ? 0 : -1;
                }
            }
            return ret;
        }

        @Override
        public Float getConstFloat(SymTab symTab) {
            int sign = getSign();
            float ret;
            if (primary instanceof Exp) {
                ret = ((Exp) primary).getConstFloat(symTab) * sign;
            } else if (primary instanceof Call) {
                throw new RuntimeException();
            } else if (primary instanceof LVal) {
                Symbol symbol = symTab.getSym(((LVal) primary).getName());
                if (symbol.isConstant() || symTab.isGlobal() && symbol.isGlobal()) {
                    Value initVal = symbol.getInitVal();
                    if (symbol.isArray()) {
                        ArrayList<Integer> indexList = new ArrayList<>();
                        for (Exp index : ((LVal) primary).getIndexList()) {
                            indexList.add(index.getConstInt(symTab));
                        }
                        if (initVal instanceof ArrayInitVal) {
                            Value val = ((ArrayInitVal) initVal).getValueWithIndex(indexList);
                            if (val instanceof ConstValue) {
                                ret = val.getNumber().floatValue() * sign;
                            } else {
                                throw new RuntimeException();
                            }
                        } else {
                            throw new RuntimeException();
                        }
                    } else {
                        if (initVal instanceof ConstValue) {
                            ret = initVal.getNumber().floatValue() * sign;
                        } else {
                            throw new RuntimeException();
                        }
                    }
                } else {
                    throw new RuntimeException();
                }
            } else if (primary instanceof Number) {
                if (((Number) primary).isIntConst) {
                    ret = (float) ((Number) primary).getIntConstValue() * sign;
                } else if (((Number) primary).isFloatConst) {
                    ret = ((Number) primary).getFloatConstValue() * sign;
                } else {
                    throw new RuntimeException("出现了未定义的数值常量类型");
                }
            } else {
                throw new RuntimeException("出现了未定义的基本表达式");
            }
            if (checkNot()) {
                if (getSign() > 0) {
                    ret = ret != 0 ? 0 : 1;
                } else {
                    ret = ret != 0 ? 0 : -1;
                }
            }
            return ret;
        }
    }

    // PrimaryExp -> Call | '(' Exp ')' | LVal | Number
    public interface PrimaryExp {
    }

    // LVal -> Ident {'[' Exp ']'}
    public static class LVal implements PrimaryExp {

        private final Token ident;
        private final ArrayList<Exp> indexList;

        public LVal(Token ident, ArrayList<Exp> indexList) {
            this.ident = ident;
            this.indexList = indexList;
            assert ident != null;
            assert indexList != null;
        }

        public String getName() {
            return ident.getContent();
        }

        public List<Exp> getIndexList() {
            return indexList;
        }
    }

    // Number
    public static class Number implements PrimaryExp {

        private Token number;
        private boolean isIntConst = false;
        private boolean isFloatConst = false;
        private int intConstValue = 0;
        private float floatConstValue = (float) 0.0;

        public Number(Token number) {
            assert number != null;
            this.number = number;

            if (number.isIntConst()) {
                isIntConst = true;
                // todo: 这里原本是高级 switch
                switch (number.getType()) {
                    case HEX_INT:
                        intConstValue = Integer.parseInt(number.getContent().substring(2), 16);
                        break;
                    case OCT_INT:
                        intConstValue = Integer.parseInt(number.getContent().substring(1), 8);
                        break;
                    case DEC_INT:
                        intConstValue = Integer.parseInt(number.getContent());
                        break;
                    default:
                        throw new AssertionError("Bad Number!");
                }
                floatConstValue = (float) intConstValue;
            } else if (number.isFloatConst()) {
                isFloatConst = true;
                floatConstValue = Float.parseFloat(number.getContent());
                intConstValue = (int) floatConstValue;
            } else {
                assert isIntConst || isFloatConst;
            }
        }

        public boolean isIntConst() {
            return isIntConst;
        }

        public boolean isFloatConst() {
            return isFloatConst;
        }

        public int getIntConstValue() {
            return intConstValue;
        }

        public float getFloatConstValue() {
            return floatConstValue;
        }
    }

    // Call -> Ident '(' [ Exp {',' Exp} ] ')'
    // FuncRParams -> Exp {',' Exp}, already inlined in Call
    public static class Call implements PrimaryExp {

        private final Token ident;
        private final ArrayList<Exp> params;
        private final int lineno;

        public Call(Token ident, ArrayList<Exp> params, int lineno) {
            assert ident != null;
            assert params != null;
            this.ident = ident;
            this.params = params;
            this.lineno = lineno;
        }

        public String getName() {
            return ident.getContent();
        }

        public List<Exp> getParams() {
            return params;
        }
        
        public int getLineno() {
            return lineno;
        }
    }

    public Ast(ArrayList<CompUnit> nodes) {
        assert nodes != null;
        this.nodes = nodes;
    }

    public List<CompUnit> getUnits() {
        return this.nodes;
    }
}
