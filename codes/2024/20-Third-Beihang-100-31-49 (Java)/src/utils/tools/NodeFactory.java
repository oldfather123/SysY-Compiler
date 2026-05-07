package utils.tools;

import front.AST.Block;
import front.AST.CompUnit;
import front.AST.Exp.*;
import front.AST.Exp.NumberNode;
import front.AST.Func.*;
import front.AST.Node;
import front.AST.TokenNode;
import front.AST.Stmt.*;

import front.AST.Var.*;
import front.lexer.Token;
import utils.type.SyntaxType;

import java.util.ArrayList;

public class NodeFactory {

    public static Node createNode(SyntaxType SType, ArrayList<Node> children) {
        Printer.printSType(SType);
        return switch (SType) {
            case PUTF -> new PutFStmt(SType,children);
            case B_TYPE -> new BType(SType, children);
            case HEX_FLOAT_CONST -> new HexFloatConst(SType, children);
            case DEC_FLOAT_CONST -> new DecFloatConst(SType, children);
            case HEX_CONST -> new HexConst(SType, children);
            case OCT_CONST -> new OctConst(SType, children);
            case DEC_CONST -> new DecConst(SType, children);
            case PUT_ARRAY -> new PutArrayStmt(SType, children);
            case PUT_F_ARRAY -> new PutFArrayStmt(SType, children);
            case GET_ARRAY_STMT -> new GetArrayStmt(SType, children);
            case GET_CH_STMT -> new GetChStmt(SType, children);
            case GET_F_ARRAY_STMT -> new GetFArrayStmt(SType, children);
            case GET_FLOAT_STMT -> new GetFloatStmt(SType, children);
            case PUTCH_STMT -> new PutChStmt(SType, children);
            case PUTFLOAT_STMT -> new PutFloatStmt(SType, children);
            case PUTINT_STMT -> new PutIntStmt(SType, children);
            case ADD_EXP -> new AddExp(SType, children);
            case MUL_EXP -> new MulExp(SType, children);
            case UNARY_EXP -> new UnaryExp(SType, children);
            case PRIMARY_EXP -> new PrimaryExp(SType, children);
            case LVAL -> new LVal(SType, children);
            case EXP -> new Exp(SType, children);
            case UNARY_OP -> new UnaryOp(SType, children);
            case NUMBER -> new NumberNode(SType, children);
            case CONST_EXP -> new ConstExp(SType, children);
            case LAND_EXP -> new LAndExp(SType, children);
            case EQ_EXP -> new EqExp(SType, children);
            case REL_EXP -> new RelExp(SType, children);
            case LOR_EXP -> new LOrExp(SType, children);
            case COND -> new Cond(SType, children);
            case FUNC_DEF -> new FuncDef(SType, children);
            case FUNC_F_PARAMS -> new FuncFParams(SType, children);
            case FUNC_F_PARAM -> new FuncFParam(SType, children);
            case FUNC_R_PARAMS -> new FuncRParams(SType, children);
            case FUNC_TYPE -> new FuncType(SType, children);
            case MAIN_FUNC_DEF -> new MainFuncDef(SType, children);
            case STMT -> new Stmt(SType, children);
            case IF_STMT -> new IfStmt(SType, children);
            case ASSIGN_STMT -> new AssignStmt(SType, children);
            case BLOCK_STMT -> new BlockStmt(SType, children);
            case BREAK_STMT -> new BreakStmt(SType, children);
            case CONTINUE_STMT -> new ContinueStmt(SType, children);
            case EXP_STMT -> new ExpStmt(SType, children);
            case GET_INT_STMT -> new GetIntStmt(SType, children);
            case RETURN_STMT -> new ReturnStmt(SType, children);
            case CONST_DECL -> new ConstDecl(SType, children);
            case CONST_DEF -> new ConstDef(SType, children);
            case CONST_INIT_VAL -> new ConstInitVal(SType, children);
            case VAR_DECL -> new VarDecl(SType, children);
            case VAR_DEF -> new VarDef(SType, children);
            case INIT_VAL -> new InitVal(SType, children);
            case BLOCK -> new Block(SType, children);
            case COMP_UNIT -> new CompUnit(SType, children);
            case STARTTIME_STMT -> new StartTimeStmt(SType, children);
            case STOPTIME_STMT -> new StopTimeStmt(SType, children);
            case WHILE_STMT -> new WhileStmt(SType, children);
            default -> null;
        };
    }

    public static Node createNode(Token token) {
        Printer.printTType(token);
        return new TokenNode(SyntaxType.TOKEN_NODE, null, token);
    }

}
