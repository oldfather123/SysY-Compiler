package cn.edu.bit.newnewcc.frontend.antlr;// Generated from SysY.g4 by ANTLR 4.12.0
import org.antlr.v4.runtime.tree.ParseTreeVisitor;

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link SysYParser}.
 *
 * @param <T> The return type of the visit operation. Use {@link Void} for
 * operations with no return type.
 */
public interface SysYVisitor<T> extends ParseTreeVisitor<T> {
	/**
	 * Visit a parse tree produced by {@link SysYParser#compilationUnit}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCompilationUnit(SysYParser.CompilationUnitContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#declaration}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitDeclaration(SysYParser.DeclarationContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#constantDeclaration}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitConstantDeclaration(SysYParser.ConstantDeclarationContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#typeSpecifier}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitTypeSpecifier(SysYParser.TypeSpecifierContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#constantDefinition}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitConstantDefinition(SysYParser.ConstantDefinitionContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#initializer}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitInitializer(SysYParser.InitializerContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#variableDeclaration}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitVariableDeclaration(SysYParser.VariableDeclarationContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#variableDefinition}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitVariableDefinition(SysYParser.VariableDefinitionContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#functionDefinition}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFunctionDefinition(SysYParser.FunctionDefinitionContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#parameterList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitParameterList(SysYParser.ParameterListContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#parameterDeclaration}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitParameterDeclaration(SysYParser.ParameterDeclarationContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#compoundStatement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitCompoundStatement(SysYParser.CompoundStatementContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#blockItem}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBlockItem(SysYParser.BlockItemContext ctx);
	/**
	 * Visit a parse tree produced by the {@code assignmentStatement}
	 * labeled alternative in {@link SysYParser#statement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitAssignmentStatement(SysYParser.AssignmentStatementContext ctx);
	/**
	 * Visit a parse tree produced by the {@code expressionStatement}
	 * labeled alternative in {@link SysYParser#statement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitExpressionStatement(SysYParser.ExpressionStatementContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childCompoundStatement}
	 * labeled alternative in {@link SysYParser#statement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildCompoundStatement(SysYParser.ChildCompoundStatementContext ctx);
	/**
	 * Visit a parse tree produced by the {@code ifStatement}
	 * labeled alternative in {@link SysYParser#statement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIfStatement(SysYParser.IfStatementContext ctx);
	/**
	 * Visit a parse tree produced by the {@code whileStatement}
	 * labeled alternative in {@link SysYParser#statement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitWhileStatement(SysYParser.WhileStatementContext ctx);
	/**
	 * Visit a parse tree produced by the {@code breakStatement}
	 * labeled alternative in {@link SysYParser#statement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBreakStatement(SysYParser.BreakStatementContext ctx);
	/**
	 * Visit a parse tree produced by the {@code continueStatement}
	 * labeled alternative in {@link SysYParser#statement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitContinueStatement(SysYParser.ContinueStatementContext ctx);
	/**
	 * Visit a parse tree produced by the {@code returnStatement}
	 * labeled alternative in {@link SysYParser#statement}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitReturnStatement(SysYParser.ReturnStatementContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#expression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitExpression(SysYParser.ExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childLogicalAndExpression}
	 * labeled alternative in {@link SysYParser#logicalOrExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildLogicalAndExpression(SysYParser.ChildLogicalAndExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code binaryLogicalOrExpression}
	 * labeled alternative in {@link SysYParser#logicalOrExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBinaryLogicalOrExpression(SysYParser.BinaryLogicalOrExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code binaryLogicalAndExpression}
	 * labeled alternative in {@link SysYParser#logicalAndExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBinaryLogicalAndExpression(SysYParser.BinaryLogicalAndExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childEqualityExpression}
	 * labeled alternative in {@link SysYParser#logicalAndExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildEqualityExpression(SysYParser.ChildEqualityExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code binaryEqualityExpression}
	 * labeled alternative in {@link SysYParser#equalityExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBinaryEqualityExpression(SysYParser.BinaryEqualityExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childRelationalExoression}
	 * labeled alternative in {@link SysYParser#equalityExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildRelationalExoression(SysYParser.ChildRelationalExoressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childAdditiveExpression}
	 * labeled alternative in {@link SysYParser#relationalExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildAdditiveExpression(SysYParser.ChildAdditiveExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code binaryRelationalExpression}
	 * labeled alternative in {@link SysYParser#relationalExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBinaryRelationalExpression(SysYParser.BinaryRelationalExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childMultiplicativeExpression}
	 * labeled alternative in {@link SysYParser#additiveExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildMultiplicativeExpression(SysYParser.ChildMultiplicativeExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code binaryAdditiveExpression}
	 * labeled alternative in {@link SysYParser#additiveExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBinaryAdditiveExpression(SysYParser.BinaryAdditiveExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code binaryMultiplicativeExpression}
	 * labeled alternative in {@link SysYParser#multiplicativeExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitBinaryMultiplicativeExpression(SysYParser.BinaryMultiplicativeExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childUnaryExpression}
	 * labeled alternative in {@link SysYParser#multiplicativeExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildUnaryExpression(SysYParser.ChildUnaryExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childPrimaryExpression}
	 * labeled alternative in {@link SysYParser#unaryExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildPrimaryExpression(SysYParser.ChildPrimaryExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code functionCallExpression}
	 * labeled alternative in {@link SysYParser#unaryExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFunctionCallExpression(SysYParser.FunctionCallExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code unaryOperatorExpression}
	 * labeled alternative in {@link SysYParser#unaryExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitUnaryOperatorExpression(SysYParser.UnaryOperatorExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code parenthesizedExpression}
	 * labeled alternative in {@link SysYParser#primaryExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitParenthesizedExpression(SysYParser.ParenthesizedExpressionContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childLValue}
	 * labeled alternative in {@link SysYParser#primaryExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildLValue(SysYParser.ChildLValueContext ctx);
	/**
	 * Visit a parse tree produced by the {@code childNumber}
	 * labeled alternative in {@link SysYParser#primaryExpression}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitChildNumber(SysYParser.ChildNumberContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#lValue}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitLValue(SysYParser.LValueContext ctx);
	/**
	 * Visit a parse tree produced by the {@code integerConstant}
	 * labeled alternative in {@link SysYParser#number}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitIntegerConstant(SysYParser.IntegerConstantContext ctx);
	/**
	 * Visit a parse tree produced by the {@code floatingConstant}
	 * labeled alternative in {@link SysYParser#number}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitFloatingConstant(SysYParser.FloatingConstantContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#argumentExpressionList}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitArgumentExpressionList(SysYParser.ArgumentExpressionListContext ctx);
	/**
	 * Visit a parse tree produced by {@link SysYParser#unaryOperator}.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	T visitUnaryOperator(SysYParser.UnaryOperatorContext ctx);
}