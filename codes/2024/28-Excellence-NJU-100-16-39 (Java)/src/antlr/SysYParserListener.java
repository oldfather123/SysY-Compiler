// Generated from ./src/antlr/SysYParser.g4 by ANTLR 4.12.0

package antlr;

import org.antlr.v4.runtime.tree.ParseTreeListener;

/**
 * This interface defines a complete listener for a parse tree produced by
 * {@link SysYParser}.
 */
public interface SysYParserListener extends ParseTreeListener {
	/**
	 * Enter a parse tree produced by {@link SysYParser#program}.
	 * @param ctx the parse tree
	 */
	void enterProgram(SysYParser.ProgramContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#program}.
	 * @param ctx the parse tree
	 */
	void exitProgram(SysYParser.ProgramContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#compUnit}.
	 * @param ctx the parse tree
	 */
	void enterCompUnit(SysYParser.CompUnitContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#compUnit}.
	 * @param ctx the parse tree
	 */
	void exitCompUnit(SysYParser.CompUnitContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#decl}.
	 * @param ctx the parse tree
	 */
	void enterDecl(SysYParser.DeclContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#decl}.
	 * @param ctx the parse tree
	 */
	void exitDecl(SysYParser.DeclContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#constDecl}.
	 * @param ctx the parse tree
	 */
	void enterConstDecl(SysYParser.ConstDeclContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#constDecl}.
	 * @param ctx the parse tree
	 */
	void exitConstDecl(SysYParser.ConstDeclContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#bType}.
	 * @param ctx the parse tree
	 */
	void enterBType(SysYParser.BTypeContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#bType}.
	 * @param ctx the parse tree
	 */
	void exitBType(SysYParser.BTypeContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#constDef}.
	 * @param ctx the parse tree
	 */
	void enterConstDef(SysYParser.ConstDefContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#constDef}.
	 * @param ctx the parse tree
	 */
	void exitConstDef(SysYParser.ConstDefContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#constInitVal}.
	 * @param ctx the parse tree
	 */
	void enterConstInitVal(SysYParser.ConstInitValContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#constInitVal}.
	 * @param ctx the parse tree
	 */
	void exitConstInitVal(SysYParser.ConstInitValContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#varDecl}.
	 * @param ctx the parse tree
	 */
	void enterVarDecl(SysYParser.VarDeclContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#varDecl}.
	 * @param ctx the parse tree
	 */
	void exitVarDecl(SysYParser.VarDeclContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#varDef}.
	 * @param ctx the parse tree
	 */
	void enterVarDef(SysYParser.VarDefContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#varDef}.
	 * @param ctx the parse tree
	 */
	void exitVarDef(SysYParser.VarDefContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#initVal}.
	 * @param ctx the parse tree
	 */
	void enterInitVal(SysYParser.InitValContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#initVal}.
	 * @param ctx the parse tree
	 */
	void exitInitVal(SysYParser.InitValContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#funcDef}.
	 * @param ctx the parse tree
	 */
	void enterFuncDef(SysYParser.FuncDefContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#funcDef}.
	 * @param ctx the parse tree
	 */
	void exitFuncDef(SysYParser.FuncDefContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#funcType}.
	 * @param ctx the parse tree
	 */
	void enterFuncType(SysYParser.FuncTypeContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#funcType}.
	 * @param ctx the parse tree
	 */
	void exitFuncType(SysYParser.FuncTypeContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#funcFParams}.
	 * @param ctx the parse tree
	 */
	void enterFuncFParams(SysYParser.FuncFParamsContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#funcFParams}.
	 * @param ctx the parse tree
	 */
	void exitFuncFParams(SysYParser.FuncFParamsContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#funcFParam}.
	 * @param ctx the parse tree
	 */
	void enterFuncFParam(SysYParser.FuncFParamContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#funcFParam}.
	 * @param ctx the parse tree
	 */
	void exitFuncFParam(SysYParser.FuncFParamContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#block}.
	 * @param ctx the parse tree
	 */
	void enterBlock(SysYParser.BlockContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#block}.
	 * @param ctx the parse tree
	 */
	void exitBlock(SysYParser.BlockContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#blockItem}.
	 * @param ctx the parse tree
	 */
	void enterBlockItem(SysYParser.BlockItemContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#blockItem}.
	 * @param ctx the parse tree
	 */
	void exitBlockItem(SysYParser.BlockItemContext ctx);
	/**
	 * Enter a parse tree produced by the {@code stmt_with_assign}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterStmt_with_assign(SysYParser.Stmt_with_assignContext ctx);
	/**
	 * Exit a parse tree produced by the {@code stmt_with_assign}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitStmt_with_assign(SysYParser.Stmt_with_assignContext ctx);
	/**
	 * Enter a parse tree produced by the {@code stmt_with_exp}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterStmt_with_exp(SysYParser.Stmt_with_expContext ctx);
	/**
	 * Exit a parse tree produced by the {@code stmt_with_exp}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitStmt_with_exp(SysYParser.Stmt_with_expContext ctx);
	/**
	 * Enter a parse tree produced by the {@code stmt_with_block}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterStmt_with_block(SysYParser.Stmt_with_blockContext ctx);
	/**
	 * Exit a parse tree produced by the {@code stmt_with_block}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitStmt_with_block(SysYParser.Stmt_with_blockContext ctx);
	/**
	 * Enter a parse tree produced by the {@code stmt_with_if}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterStmt_with_if(SysYParser.Stmt_with_ifContext ctx);
	/**
	 * Exit a parse tree produced by the {@code stmt_with_if}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitStmt_with_if(SysYParser.Stmt_with_ifContext ctx);
	/**
	 * Enter a parse tree produced by the {@code stmt_with_while}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterStmt_with_while(SysYParser.Stmt_with_whileContext ctx);
	/**
	 * Exit a parse tree produced by the {@code stmt_with_while}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitStmt_with_while(SysYParser.Stmt_with_whileContext ctx);
	/**
	 * Enter a parse tree produced by the {@code stmt_with_break}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterStmt_with_break(SysYParser.Stmt_with_breakContext ctx);
	/**
	 * Exit a parse tree produced by the {@code stmt_with_break}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitStmt_with_break(SysYParser.Stmt_with_breakContext ctx);
	/**
	 * Enter a parse tree produced by the {@code stmt_with_continue}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterStmt_with_continue(SysYParser.Stmt_with_continueContext ctx);
	/**
	 * Exit a parse tree produced by the {@code stmt_with_continue}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitStmt_with_continue(SysYParser.Stmt_with_continueContext ctx);
	/**
	 * Enter a parse tree produced by the {@code stmt_with_return}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterStmt_with_return(SysYParser.Stmt_with_returnContext ctx);
	/**
	 * Exit a parse tree produced by the {@code stmt_with_return}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitStmt_with_return(SysYParser.Stmt_with_returnContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#returnStmt}.
	 * @param ctx the parse tree
	 */
	void enterReturnStmt(SysYParser.ReturnStmtContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#returnStmt}.
	 * @param ctx the parse tree
	 */
	void exitReturnStmt(SysYParser.ReturnStmtContext ctx);
	/**
	 * Enter a parse tree produced by the {@code exp_with_plus_and_mius}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void enterExp_with_plus_and_mius(SysYParser.Exp_with_plus_and_miusContext ctx);
	/**
	 * Exit a parse tree produced by the {@code exp_with_plus_and_mius}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void exitExp_with_plus_and_mius(SysYParser.Exp_with_plus_and_miusContext ctx);
	/**
	 * Enter a parse tree produced by the {@code exp_with_unaryOp}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void enterExp_with_unaryOp(SysYParser.Exp_with_unaryOpContext ctx);
	/**
	 * Exit a parse tree produced by the {@code exp_with_unaryOp}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void exitExp_with_unaryOp(SysYParser.Exp_with_unaryOpContext ctx);
	/**
	 * Enter a parse tree produced by the {@code exp_with_symbol}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void enterExp_with_symbol(SysYParser.Exp_with_symbolContext ctx);
	/**
	 * Exit a parse tree produced by the {@code exp_with_symbol}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void exitExp_with_symbol(SysYParser.Exp_with_symbolContext ctx);
	/**
	 * Enter a parse tree produced by the {@code exp_with_lval}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void enterExp_with_lval(SysYParser.Exp_with_lvalContext ctx);
	/**
	 * Exit a parse tree produced by the {@code exp_with_lval}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void exitExp_with_lval(SysYParser.Exp_with_lvalContext ctx);
	/**
	 * Enter a parse tree produced by the {@code exp_with_Paren}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void enterExp_with_Paren(SysYParser.Exp_with_ParenContext ctx);
	/**
	 * Exit a parse tree produced by the {@code exp_with_Paren}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void exitExp_with_Paren(SysYParser.Exp_with_ParenContext ctx);
	/**
	 * Enter a parse tree produced by the {@code exp_with_num}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void enterExp_with_num(SysYParser.Exp_with_numContext ctx);
	/**
	 * Exit a parse tree produced by the {@code exp_with_num}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void exitExp_with_num(SysYParser.Exp_with_numContext ctx);
	/**
	 * Enter a parse tree produced by the {@code exp_with_funcRParams}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void enterExp_with_funcRParams(SysYParser.Exp_with_funcRParamsContext ctx);
	/**
	 * Exit a parse tree produced by the {@code exp_with_funcRParams}
	 * labeled alternative in {@link SysYParser#exp}.
	 * @param ctx the parse tree
	 */
	void exitExp_with_funcRParams(SysYParser.Exp_with_funcRParamsContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#cond}.
	 * @param ctx the parse tree
	 */
	void enterCond(SysYParser.CondContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#cond}.
	 * @param ctx the parse tree
	 */
	void exitCond(SysYParser.CondContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#lVal}.
	 * @param ctx the parse tree
	 */
	void enterLVal(SysYParser.LValContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#lVal}.
	 * @param ctx the parse tree
	 */
	void exitLVal(SysYParser.LValContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#number}.
	 * @param ctx the parse tree
	 */
	void enterNumber(SysYParser.NumberContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#number}.
	 * @param ctx the parse tree
	 */
	void exitNumber(SysYParser.NumberContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#unaryOp}.
	 * @param ctx the parse tree
	 */
	void enterUnaryOp(SysYParser.UnaryOpContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#unaryOp}.
	 * @param ctx the parse tree
	 */
	void exitUnaryOp(SysYParser.UnaryOpContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#funcRParams}.
	 * @param ctx the parse tree
	 */
	void enterFuncRParams(SysYParser.FuncRParamsContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#funcRParams}.
	 * @param ctx the parse tree
	 */
	void exitFuncRParams(SysYParser.FuncRParamsContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#param}.
	 * @param ctx the parse tree
	 */
	void enterParam(SysYParser.ParamContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#param}.
	 * @param ctx the parse tree
	 */
	void exitParam(SysYParser.ParamContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#constExp}.
	 * @param ctx the parse tree
	 */
	void enterConstExp(SysYParser.ConstExpContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#constExp}.
	 * @param ctx the parse tree
	 */
	void exitConstExp(SysYParser.ConstExpContext ctx);
}