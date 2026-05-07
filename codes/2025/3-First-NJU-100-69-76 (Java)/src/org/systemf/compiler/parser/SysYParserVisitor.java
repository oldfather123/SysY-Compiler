// Generated from org/systemf/compiler/parser/SysYParser.g4 by ANTLR 4.13.2
package org.systemf.compiler.parser;
import org.antlr.v4.runtime.tree.ParseTreeVisitor;

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link SysYParser}.
 *
 * @param <T> The return type of the visit operation. Use {@link Void} for
 * operations with no return type.
 */
public interface SysYParserVisitor<T> extends ParseTreeVisitor<T> {
	/**
	 * Visit a parse tree produced by {@link SysYParser#program}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitProgram(SysYParser.ProgramContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#basicType}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBasicType(SysYParser.BasicTypeContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#retType}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitRetType(SysYParser.RetTypeContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#constPrefix}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitConstPrefix(SysYParser.ConstPrefixContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#incompleteArray}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIncompleteArray(SysYParser.IncompleteArrayContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#arrayPostfixSingle}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitArrayPostfixSingle(SysYParser.ArrayPostfixSingleContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#arrayPostfix}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitArrayPostfix(SysYParser.ArrayPostfixContext ctx);
	/**
	 * Visit a parse tree produced by the {@code single}
	 * labeled alternative in {@link SysYParser#eqInitializeVal}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitSingle(SysYParser.SingleContext ctx);
	/**
	 * Visit a parse tree produced by the {@code array}
	 * labeled alternative in {@link SysYParser#eqInitializeVal}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitArray(SysYParser.ArrayContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#initializer}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInitializer(SysYParser.InitializerContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#varDefEntry}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitVarDefEntry(SysYParser.VarDefEntryContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#varDef}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitVarDef(SysYParser.VarDefContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#funcParam}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFuncParam(SysYParser.FuncParamContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#funcDef}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFuncDef(SysYParser.FuncDefContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#varAccess}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitVarAccess(SysYParser.VarAccessContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#funcRealParam}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFuncRealParam(SysYParser.FuncRealParamContext ctx);
	/**
	 * Visit a parse tree produced by the {@code paren}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitParen(SysYParser.ParenContext ctx);
	/**
	 * Visit a parse tree produced by the {@code access}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAccess(SysYParser.AccessContext ctx);
	/**
	 * Visit a parse tree produced by the {@code or}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitOr(SysYParser.OrContext ctx);
	/**
	 * Visit a parse tree produced by the {@code eqs}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitEqs(SysYParser.EqsContext ctx);
	/**
	 * Visit a parse tree produced by the {@code and}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAnd(SysYParser.AndContext ctx);
	/**
	 * Visit a parse tree produced by the {@code functionCall}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFunctionCall(SysYParser.FunctionCallContext ctx);
	/**
	 * Visit a parse tree produced by the {@code constFloat}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitConstFloat(SysYParser.ConstFloatContext ctx);
	/**
	 * Visit a parse tree produced by the {@code constInt}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitConstInt(SysYParser.ConstIntContext ctx);
	/**
	 * Visit a parse tree produced by the {@code unary}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitUnary(SysYParser.UnaryContext ctx);
	/**
	 * Visit a parse tree produced by the {@code muls}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitMuls(SysYParser.MulsContext ctx);
	/**
	 * Visit a parse tree produced by the {@code adds}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAdds(SysYParser.AddsContext ctx);
	/**
	 * Visit a parse tree produced by the {@code rels}
	 * labeled alternative in {@link SysYParser#expr}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitRels(SysYParser.RelsContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#stmtBlock}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitStmtBlock(SysYParser.StmtBlockContext ctx);
	/**
	 * Visit a parse tree produced by the {@code expression}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitExpression(SysYParser.ExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code assignment}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAssignment(SysYParser.AssignmentContext ctx);
	/**
	 * Visit a parse tree produced by the {@code block}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBlock(SysYParser.BlockContext ctx);
	/**
	 * Visit a parse tree produced by the {@code if}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIf(SysYParser.IfContext ctx);
	/**
	 * Visit a parse tree produced by the {@code while}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitWhile(SysYParser.WhileContext ctx);
	/**
	 * Visit a parse tree produced by the {@code break}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBreak(SysYParser.BreakContext ctx);
	/**
	 * Visit a parse tree produced by the {@code continue}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitContinue(SysYParser.ContinueContext ctx);
	/**
	 * Visit a parse tree produced by the {@code return}
	 * labeled alternative in {@link SysYParser#stmt}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitReturn(SysYParser.ReturnContext ctx);
}