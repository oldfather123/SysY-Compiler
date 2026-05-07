// Generated from org/systemf/compiler/parser/SysYParser.g4 by ANTLR 4.13.2
package org.systemf.compiler.parser;
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
	 * Enter a parse tree produced by {@link SysYParser#basicType}.
	 * @param ctx the parse tree
	 */
	void enterBasicType(SysYParser.BasicTypeContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#basicType}.
	 * @param ctx the parse tree
	 */
	void exitBasicType(SysYParser.BasicTypeContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#retType}.
	 * @param ctx the parse tree
	 */
	void enterRetType(SysYParser.RetTypeContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#retType}.
	 * @param ctx the parse tree
	 */
	void exitRetType(SysYParser.RetTypeContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#constPrefix}.
	 * @param ctx the parse tree
	 */
	void enterConstPrefix(SysYParser.ConstPrefixContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#constPrefix}.
	 * @param ctx the parse tree
	 */
	void exitConstPrefix(SysYParser.ConstPrefixContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#incompleteArray}.
	 * @param ctx the parse tree
	 */
	void enterIncompleteArray(SysYParser.IncompleteArrayContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#incompleteArray}.
	 * @param ctx the parse tree
	 */
	void exitIncompleteArray(SysYParser.IncompleteArrayContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#arrayPostfixSingle}.
	 * @param ctx the parse tree
	 */
	void enterArrayPostfixSingle(SysYParser.ArrayPostfixSingleContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#arrayPostfixSingle}.
	 * @param ctx the parse tree
	 */
	void exitArrayPostfixSingle(SysYParser.ArrayPostfixSingleContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#arrayPostfix}.
	 * @param ctx the parse tree
	 */
	void enterArrayPostfix(SysYParser.ArrayPostfixContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#arrayPostfix}.
	 * @param ctx the parse tree
	 */
	void exitArrayPostfix(SysYParser.ArrayPostfixContext ctx);
	/**
	 * Enter a parse tree produced by the {@code single}
	 * labeled alternative in {@link SysYParser#eqInitializeVal}.
	 * @param ctx the parse tree
	 */
	void enterSingle(SysYParser.SingleContext ctx);
	/**
	 * Exit a parse tree produced by the {@code single}
	 * labeled alternative in {@link SysYParser#eqInitializeVal}.
	 * @param ctx the parse tree
	 */
	void exitSingle(SysYParser.SingleContext ctx);
	/**
	 * Enter a parse tree produced by the {@code array}
	 * labeled alternative in {@link SysYParser#eqInitializeVal}.
	 * @param ctx the parse tree
	 */
	void enterArray(SysYParser.ArrayContext ctx);
	/**
	 * Exit a parse tree produced by the {@code array}
	 * labeled alternative in {@link SysYParser#eqInitializeVal}.
	 * @param ctx the parse tree
	 */
	void exitArray(SysYParser.ArrayContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#initializer}.
	 * @param ctx the parse tree
	 */
	void enterInitializer(SysYParser.InitializerContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#initializer}.
	 * @param ctx the parse tree
	 */
	void exitInitializer(SysYParser.InitializerContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#varDefEntry}.
	 * @param ctx the parse tree
	 */
	void enterVarDefEntry(SysYParser.VarDefEntryContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#varDefEntry}.
	 * @param ctx the parse tree
	 */
	void exitVarDefEntry(SysYParser.VarDefEntryContext ctx);
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
	 * Enter a parse tree produced by {@link SysYParser#funcParam}.
	 * @param ctx the parse tree
	 */
	void enterFuncParam(SysYParser.FuncParamContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#funcParam}.
	 * @param ctx the parse tree
	 */
	void exitFuncParam(SysYParser.FuncParamContext ctx);
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
	 * Enter a parse tree produced by {@link SysYParser#varAccess}.
	 * @param ctx the parse tree
	 */
	void enterVarAccess(SysYParser.VarAccessContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#varAccess}.
	 * @param ctx the parse tree
	 */
	void exitVarAccess(SysYParser.VarAccessContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#funcRealParam}.
	 * @param ctx the parse tree
	 */
	void enterFuncRealParam(SysYParser.FuncRealParamContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#funcRealParam}.
	 * @param ctx the parse tree
	 */
	void exitFuncRealParam(SysYParser.FuncRealParamContext ctx);
	/**
	 * Enter a parse tree produced by the {@code paren}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterParen(SysYParser.ParenContext ctx);
	/**
	 * Exit a parse tree produced by the {@code paren}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitParen(SysYParser.ParenContext ctx);
	/**
	 * Enter a parse tree produced by the {@code access}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterAccess(SysYParser.AccessContext ctx);
	/**
	 * Exit a parse tree produced by the {@code access}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitAccess(SysYParser.AccessContext ctx);
	/**
	 * Enter a parse tree produced by the {@code or}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterOr(SysYParser.OrContext ctx);
	/**
	 * Exit a parse tree produced by the {@code or}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitOr(SysYParser.OrContext ctx);
	/**
	 * Enter a parse tree produced by the {@code eqs}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterEqs(SysYParser.EqsContext ctx);
	/**
	 * Exit a parse tree produced by the {@code eqs}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitEqs(SysYParser.EqsContext ctx);
	/**
	 * Enter a parse tree produced by the {@code and}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterAnd(SysYParser.AndContext ctx);
	/**
	 * Exit a parse tree produced by the {@code and}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitAnd(SysYParser.AndContext ctx);
	/**
	 * Enter a parse tree produced by the {@code functionCall}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterFunctionCall(SysYParser.FunctionCallContext ctx);
	/**
	 * Exit a parse tree produced by the {@code functionCall}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitFunctionCall(SysYParser.FunctionCallContext ctx);
	/**
	 * Enter a parse tree produced by the {@code constFloat}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterConstFloat(SysYParser.ConstFloatContext ctx);
	/**
	 * Exit a parse tree produced by the {@code constFloat}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitConstFloat(SysYParser.ConstFloatContext ctx);
	/**
	 * Enter a parse tree produced by the {@code constInt}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterConstInt(SysYParser.ConstIntContext ctx);
	/**
	 * Exit a parse tree produced by the {@code constInt}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitConstInt(SysYParser.ConstIntContext ctx);
	/**
	 * Enter a parse tree produced by the {@code unary}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterUnary(SysYParser.UnaryContext ctx);
	/**
	 * Exit a parse tree produced by the {@code unary}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitUnary(SysYParser.UnaryContext ctx);
	/**
	 * Enter a parse tree produced by the {@code muls}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterMuls(SysYParser.MulsContext ctx);
	/**
	 * Exit a parse tree produced by the {@code muls}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitMuls(SysYParser.MulsContext ctx);
	/**
	 * Enter a parse tree produced by the {@code adds}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterAdds(SysYParser.AddsContext ctx);
	/**
	 * Exit a parse tree produced by the {@code adds}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitAdds(SysYParser.AddsContext ctx);
	/**
	 * Enter a parse tree produced by the {@code rels}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void enterRels(SysYParser.RelsContext ctx);
	/**
	 * Exit a parse tree produced by the {@code rels}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 */
	void exitRels(SysYParser.RelsContext ctx);
	/**
	 * Enter a parse tree produced by {@link SysYParser#stmtBlock}.
	 * @param ctx the parse tree
	 */
	void enterStmtBlock(SysYParser.StmtBlockContext ctx);
	/**
	 * Exit a parse tree produced by {@link SysYParser#stmtBlock}.
	 * @param ctx the parse tree
	 */
	void exitStmtBlock(SysYParser.StmtBlockContext ctx);
	/**
	 * Enter a parse tree produced by the {@code expression}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterExpression(SysYParser.ExpressionContext ctx);
	/**
	 * Exit a parse tree produced by the {@code expression}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitExpression(SysYParser.ExpressionContext ctx);
	/**
	 * Enter a parse tree produced by the {@code assignment}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterAssignment(SysYParser.AssignmentContext ctx);
	/**
	 * Exit a parse tree produced by the {@code assignment}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitAssignment(SysYParser.AssignmentContext ctx);
	/**
	 * Enter a parse tree produced by the {@code block}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterBlock(SysYParser.BlockContext ctx);
	/**
	 * Exit a parse tree produced by the {@code block}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitBlock(SysYParser.BlockContext ctx);
	/**
	 * Enter a parse tree produced by the {@code if}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterIf(SysYParser.IfContext ctx);
	/**
	 * Exit a parse tree produced by the {@code if}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitIf(SysYParser.IfContext ctx);
	/**
	 * Enter a parse tree produced by the {@code while}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterWhile(SysYParser.WhileContext ctx);
	/**
	 * Exit a parse tree produced by the {@code while}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitWhile(SysYParser.WhileContext ctx);
	/**
	 * Enter a parse tree produced by the {@code break}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterBreak(SysYParser.BreakContext ctx);
	/**
	 * Exit a parse tree produced by the {@code break}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitBreak(SysYParser.BreakContext ctx);
	/**
	 * Enter a parse tree produced by the {@code continue}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterContinue(SysYParser.ContinueContext ctx);
	/**
	 * Exit a parse tree produced by the {@code continue}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitContinue(SysYParser.ContinueContext ctx);
	/**
	 * Enter a parse tree produced by the {@code return}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void enterReturn(SysYParser.ReturnContext ctx);
	/**
	 * Exit a parse tree produced by the {@code return}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 */
	void exitReturn(SysYParser.ReturnContext ctx);
}