#![allow(nonstandard_style)]
// Generated from src/frontend/antlr_dep/SysY.g4 by ANTLR 4.8
use super::sysyparser::*;
use antlr_rust::tree::{ParseTreeVisitor, ParseTreeVisitorCompat};

/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by {@link SysYParser}.
 */
pub trait SysYVisitor<'input>: ParseTreeVisitor<'input, SysYParserContextType> {
    /**
     * Visit a parse tree produced by {@link SysYParser#compUnit}.
     * @param ctx the parse tree
     */
    fn visit_compUnit(&mut self, ctx: &CompUnitContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#decl}.
     * @param ctx the parse tree
     */
    fn visit_decl(&mut self, ctx: &DeclContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constDecl}.
     * @param ctx the parse tree
     */
    fn visit_constDecl(&mut self, ctx: &ConstDeclContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#bType}.
     * @param ctx the parse tree
     */
    fn visit_bType(&mut self, ctx: &BTypeContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constDef}.
     * @param ctx the parse tree
     */
    fn visit_constDef(&mut self, ctx: &ConstDefContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code scalarConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    fn visit_scalarConstInitVal(&mut self, ctx: &ScalarConstInitValContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code listConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    fn visit_listConstInitVal(&mut self, ctx: &ListConstInitValContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#varDecl}.
     * @param ctx the parse tree
     */
    fn visit_varDecl(&mut self, ctx: &VarDeclContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code uninitVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    fn visit_uninitVarDef(&mut self, ctx: &UninitVarDefContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code initVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    fn visit_initVarDef(&mut self, ctx: &InitVarDefContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code scalarInitVal}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    fn visit_scalarInitVal(&mut self, ctx: &ScalarInitValContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code listInitval}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    fn visit_listInitval(&mut self, ctx: &ListInitvalContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcDef}.
     * @param ctx the parse tree
     */
    fn visit_funcDef(&mut self, ctx: &FuncDefContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcType}.
     * @param ctx the parse tree
     */
    fn visit_funcType(&mut self, ctx: &FuncTypeContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcFParams}.
     * @param ctx the parse tree
     */
    fn visit_funcFParams(&mut self, ctx: &FuncFParamsContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcFParam}.
     * @param ctx the parse tree
     */
    fn visit_funcFParam(&mut self, ctx: &FuncFParamContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#block}.
     * @param ctx the parse tree
     */
    fn visit_block(&mut self, ctx: &BlockContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#blockItem}.
     * @param ctx the parse tree
     */
    fn visit_blockItem(&mut self, ctx: &BlockItemContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code assignment}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_assignment(&mut self, ctx: &AssignmentContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code expStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_expStmt(&mut self, ctx: &ExpStmtContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code blockStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_blockStmt(&mut self, ctx: &BlockStmtContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code ifStmt1}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_ifStmt1(&mut self, ctx: &IfStmt1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code ifStmt2}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_ifStmt2(&mut self, ctx: &IfStmt2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code whileStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_whileStmt(&mut self, ctx: &WhileStmtContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code breakStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_breakStmt(&mut self, ctx: &BreakStmtContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code continueStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_continueStmt(&mut self, ctx: &ContinueStmtContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code returnStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_returnStmt(&mut self, ctx: &ReturnStmtContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#exp}.
     * @param ctx the parse tree
     */
    fn visit_exp(&mut self, ctx: &ExpContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#cond}.
     * @param ctx the parse tree
     */
    fn visit_cond(&mut self, ctx: &CondContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#lVal}.
     * @param ctx the parse tree
     */
    fn visit_lVal(&mut self, ctx: &LValContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp1}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn visit_primaryExp1(&mut self, ctx: &PrimaryExp1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp2}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn visit_primaryExp2(&mut self, ctx: &PrimaryExp2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp3}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn visit_primaryExp3(&mut self, ctx: &PrimaryExp3Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code intnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    fn visit_intnum(&mut self, ctx: &IntnumContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code floatnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    fn visit_floatnum(&mut self, ctx: &FloatnumContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code unary1}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn visit_unary1(&mut self, ctx: &Unary1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code unary2}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn visit_unary2(&mut self, ctx: &Unary2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code unary3}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn visit_unary3(&mut self, ctx: &Unary3Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#unaryOp}.
     * @param ctx the parse tree
     */
    fn visit_unaryOp(&mut self, ctx: &UnaryOpContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcRParams}.
     * @param ctx the parse tree
     */
    fn visit_funcRParams(&mut self, ctx: &FuncRParamsContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcRParam}.
     * @param ctx the parse tree
     */
    fn visit_funcRParam(&mut self, ctx: &FuncRParamContext<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code mul2}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    fn visit_mul2(&mut self, ctx: &Mul2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code mul1}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    fn visit_mul1(&mut self, ctx: &Mul1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code add2}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    fn visit_add2(&mut self, ctx: &Add2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code add1}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    fn visit_add1(&mut self, ctx: &Add1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code rel2}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    fn visit_rel2(&mut self, ctx: &Rel2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code rel1}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    fn visit_rel1(&mut self, ctx: &Rel1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code eq1}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    fn visit_eq1(&mut self, ctx: &Eq1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code eq2}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    fn visit_eq2(&mut self, ctx: &Eq2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code lAnd2}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    fn visit_lAnd2(&mut self, ctx: &LAnd2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code lAnd1}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    fn visit_lAnd1(&mut self, ctx: &LAnd1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code lOr1}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    fn visit_lOr1(&mut self, ctx: &LOr1Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code lOr2}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    fn visit_lOr2(&mut self, ctx: &LOr2Context<'input>) {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constExp}.
     * @param ctx the parse tree
     */
    fn visit_constExp(&mut self, ctx: &ConstExpContext<'input>) {
        self.visit_children(ctx)
    }
}

pub trait SysYVisitorCompat<'input>:
    ParseTreeVisitorCompat<'input, Node = SysYParserContextType>
{
    /**
     * Visit a parse tree produced by {@link SysYParser#compUnit}.
     * @param ctx the parse tree
     */
    fn visit_compUnit(&mut self, ctx: &CompUnitContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#decl}.
     * @param ctx the parse tree
     */
    fn visit_decl(&mut self, ctx: &DeclContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constDecl}.
     * @param ctx the parse tree
     */
    fn visit_constDecl(&mut self, ctx: &ConstDeclContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#bType}.
     * @param ctx the parse tree
     */
    fn visit_bType(&mut self, ctx: &BTypeContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constDef}.
     * @param ctx the parse tree
     */
    fn visit_constDef(&mut self, ctx: &ConstDefContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code scalarConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    fn visit_scalarConstInitVal(
        &mut self,
        ctx: &ScalarConstInitValContext<'input>,
    ) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code listConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    fn visit_listConstInitVal(&mut self, ctx: &ListConstInitValContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#varDecl}.
     * @param ctx the parse tree
     */
    fn visit_varDecl(&mut self, ctx: &VarDeclContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code uninitVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    fn visit_uninitVarDef(&mut self, ctx: &UninitVarDefContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code initVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    fn visit_initVarDef(&mut self, ctx: &InitVarDefContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code scalarInitVal}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    fn visit_scalarInitVal(&mut self, ctx: &ScalarInitValContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code listInitval}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    fn visit_listInitval(&mut self, ctx: &ListInitvalContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcDef}.
     * @param ctx the parse tree
     */
    fn visit_funcDef(&mut self, ctx: &FuncDefContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcType}.
     * @param ctx the parse tree
     */
    fn visit_funcType(&mut self, ctx: &FuncTypeContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcFParams}.
     * @param ctx the parse tree
     */
    fn visit_funcFParams(&mut self, ctx: &FuncFParamsContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcFParam}.
     * @param ctx the parse tree
     */
    fn visit_funcFParam(&mut self, ctx: &FuncFParamContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#block}.
     * @param ctx the parse tree
     */
    fn visit_block(&mut self, ctx: &BlockContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#blockItem}.
     * @param ctx the parse tree
     */
    fn visit_blockItem(&mut self, ctx: &BlockItemContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code assignment}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_assignment(&mut self, ctx: &AssignmentContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code expStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_expStmt(&mut self, ctx: &ExpStmtContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code blockStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_blockStmt(&mut self, ctx: &BlockStmtContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code ifStmt1}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_ifStmt1(&mut self, ctx: &IfStmt1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code ifStmt2}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_ifStmt2(&mut self, ctx: &IfStmt2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code whileStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_whileStmt(&mut self, ctx: &WhileStmtContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code breakStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_breakStmt(&mut self, ctx: &BreakStmtContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code continueStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_continueStmt(&mut self, ctx: &ContinueStmtContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code returnStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn visit_returnStmt(&mut self, ctx: &ReturnStmtContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#exp}.
     * @param ctx the parse tree
     */
    fn visit_exp(&mut self, ctx: &ExpContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#cond}.
     * @param ctx the parse tree
     */
    fn visit_cond(&mut self, ctx: &CondContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#lVal}.
     * @param ctx the parse tree
     */
    fn visit_lVal(&mut self, ctx: &LValContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp1}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn visit_primaryExp1(&mut self, ctx: &PrimaryExp1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp2}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn visit_primaryExp2(&mut self, ctx: &PrimaryExp2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp3}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn visit_primaryExp3(&mut self, ctx: &PrimaryExp3Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code intnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    fn visit_intnum(&mut self, ctx: &IntnumContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code floatnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    fn visit_floatnum(&mut self, ctx: &FloatnumContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code unary1}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn visit_unary1(&mut self, ctx: &Unary1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code unary2}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn visit_unary2(&mut self, ctx: &Unary2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code unary3}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn visit_unary3(&mut self, ctx: &Unary3Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#unaryOp}.
     * @param ctx the parse tree
     */
    fn visit_unaryOp(&mut self, ctx: &UnaryOpContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcRParams}.
     * @param ctx the parse tree
     */
    fn visit_funcRParams(&mut self, ctx: &FuncRParamsContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcRParam}.
     * @param ctx the parse tree
     */
    fn visit_funcRParam(&mut self, ctx: &FuncRParamContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code mul2}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    fn visit_mul2(&mut self, ctx: &Mul2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code mul1}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    fn visit_mul1(&mut self, ctx: &Mul1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code add2}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    fn visit_add2(&mut self, ctx: &Add2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code add1}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    fn visit_add1(&mut self, ctx: &Add1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code rel2}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    fn visit_rel2(&mut self, ctx: &Rel2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code rel1}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    fn visit_rel1(&mut self, ctx: &Rel1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code eq1}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    fn visit_eq1(&mut self, ctx: &Eq1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code eq2}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    fn visit_eq2(&mut self, ctx: &Eq2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code lAnd2}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    fn visit_lAnd2(&mut self, ctx: &LAnd2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code lAnd1}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    fn visit_lAnd1(&mut self, ctx: &LAnd1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code lOr1}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    fn visit_lOr1(&mut self, ctx: &LOr1Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code lOr2}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    fn visit_lOr2(&mut self, ctx: &LOr2Context<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constExp}.
     * @param ctx the parse tree
     */
    fn visit_constExp(&mut self, ctx: &ConstExpContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }
}

impl<'input, T> SysYVisitor<'input> for T
where
    T: SysYVisitorCompat<'input>,
{
    fn visit_compUnit(&mut self, ctx: &CompUnitContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_compUnit(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_decl(&mut self, ctx: &DeclContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_decl(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_constDecl(&mut self, ctx: &ConstDeclContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_constDecl(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_bType(&mut self, ctx: &BTypeContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_bType(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_constDef(&mut self, ctx: &ConstDefContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_constDef(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_scalarConstInitVal(&mut self, ctx: &ScalarConstInitValContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_scalarConstInitVal(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_listConstInitVal(&mut self, ctx: &ListConstInitValContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_listConstInitVal(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_varDecl(&mut self, ctx: &VarDeclContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_varDecl(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_uninitVarDef(&mut self, ctx: &UninitVarDefContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_uninitVarDef(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_initVarDef(&mut self, ctx: &InitVarDefContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_initVarDef(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_scalarInitVal(&mut self, ctx: &ScalarInitValContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_scalarInitVal(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_listInitval(&mut self, ctx: &ListInitvalContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_listInitval(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_funcDef(&mut self, ctx: &FuncDefContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_funcDef(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_funcType(&mut self, ctx: &FuncTypeContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_funcType(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_funcFParams(&mut self, ctx: &FuncFParamsContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_funcFParams(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_funcFParam(&mut self, ctx: &FuncFParamContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_funcFParam(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_block(&mut self, ctx: &BlockContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_block(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_blockItem(&mut self, ctx: &BlockItemContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_blockItem(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_assignment(&mut self, ctx: &AssignmentContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_assignment(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_expStmt(&mut self, ctx: &ExpStmtContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_expStmt(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_blockStmt(&mut self, ctx: &BlockStmtContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_blockStmt(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_ifStmt1(&mut self, ctx: &IfStmt1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_ifStmt1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_ifStmt2(&mut self, ctx: &IfStmt2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_ifStmt2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_whileStmt(&mut self, ctx: &WhileStmtContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_whileStmt(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_breakStmt(&mut self, ctx: &BreakStmtContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_breakStmt(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_continueStmt(&mut self, ctx: &ContinueStmtContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_continueStmt(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_returnStmt(&mut self, ctx: &ReturnStmtContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_returnStmt(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_exp(&mut self, ctx: &ExpContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_exp(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_cond(&mut self, ctx: &CondContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_cond(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_lVal(&mut self, ctx: &LValContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_lVal(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_primaryExp1(&mut self, ctx: &PrimaryExp1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_primaryExp1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_primaryExp2(&mut self, ctx: &PrimaryExp2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_primaryExp2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_primaryExp3(&mut self, ctx: &PrimaryExp3Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_primaryExp3(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_intnum(&mut self, ctx: &IntnumContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_intnum(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_floatnum(&mut self, ctx: &FloatnumContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_floatnum(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_unary1(&mut self, ctx: &Unary1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_unary1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_unary2(&mut self, ctx: &Unary2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_unary2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_unary3(&mut self, ctx: &Unary3Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_unary3(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_unaryOp(&mut self, ctx: &UnaryOpContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_unaryOp(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_funcRParams(&mut self, ctx: &FuncRParamsContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_funcRParams(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_funcRParam(&mut self, ctx: &FuncRParamContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_funcRParam(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_mul2(&mut self, ctx: &Mul2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_mul2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_mul1(&mut self, ctx: &Mul1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_mul1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_add2(&mut self, ctx: &Add2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_add2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_add1(&mut self, ctx: &Add1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_add1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_rel2(&mut self, ctx: &Rel2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_rel2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_rel1(&mut self, ctx: &Rel1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_rel1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_eq1(&mut self, ctx: &Eq1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_eq1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_eq2(&mut self, ctx: &Eq2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_eq2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_lAnd2(&mut self, ctx: &LAnd2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_lAnd2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_lAnd1(&mut self, ctx: &LAnd1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_lAnd1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_lOr1(&mut self, ctx: &LOr1Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_lOr1(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_lOr2(&mut self, ctx: &LOr2Context<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_lOr2(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }

    fn visit_constExp(&mut self, ctx: &ConstExpContext<'input>) {
        let result = <Self as SysYVisitorCompat>::visit_constExp(self, ctx);
        *<Self as ParseTreeVisitorCompat>::temp_result(self) = result;
    }
}
