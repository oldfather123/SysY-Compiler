#![allow(nonstandard_style)]
// Generated from src/frontend/antlr_dep/SysY.g4 by ANTLR 4.8
use super::sysyparser::*;
use antlr_rust::tree::ParseTreeListener;

pub trait SysYListener<'input>: ParseTreeListener<'input, SysYParserContextType> {
    /**
     * Enter a parse tree produced by {@link SysYParser#compUnit}.
     * @param ctx the parse tree
     */
    fn enter_compUnit(&mut self, _ctx: &CompUnitContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#compUnit}.
     * @param ctx the parse tree
     */
    fn exit_compUnit(&mut self, _ctx: &CompUnitContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#decl}.
     * @param ctx the parse tree
     */
    fn enter_decl(&mut self, _ctx: &DeclContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#decl}.
     * @param ctx the parse tree
     */
    fn exit_decl(&mut self, _ctx: &DeclContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#constDecl}.
     * @param ctx the parse tree
     */
    fn enter_constDecl(&mut self, _ctx: &ConstDeclContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#constDecl}.
     * @param ctx the parse tree
     */
    fn exit_constDecl(&mut self, _ctx: &ConstDeclContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#bType}.
     * @param ctx the parse tree
     */
    fn enter_bType(&mut self, _ctx: &BTypeContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#bType}.
     * @param ctx the parse tree
     */
    fn exit_bType(&mut self, _ctx: &BTypeContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#constDef}.
     * @param ctx the parse tree
     */
    fn enter_constDef(&mut self, _ctx: &ConstDefContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#constDef}.
     * @param ctx the parse tree
     */
    fn exit_constDef(&mut self, _ctx: &ConstDefContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code scalarConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    fn enter_scalarConstInitVal(&mut self, _ctx: &ScalarConstInitValContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code scalarConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    fn exit_scalarConstInitVal(&mut self, _ctx: &ScalarConstInitValContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code listConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    fn enter_listConstInitVal(&mut self, _ctx: &ListConstInitValContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code listConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    fn exit_listConstInitVal(&mut self, _ctx: &ListConstInitValContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#varDecl}.
     * @param ctx the parse tree
     */
    fn enter_varDecl(&mut self, _ctx: &VarDeclContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#varDecl}.
     * @param ctx the parse tree
     */
    fn exit_varDecl(&mut self, _ctx: &VarDeclContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code uninitVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    fn enter_uninitVarDef(&mut self, _ctx: &UninitVarDefContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code uninitVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    fn exit_uninitVarDef(&mut self, _ctx: &UninitVarDefContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code initVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    fn enter_initVarDef(&mut self, _ctx: &InitVarDefContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code initVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    fn exit_initVarDef(&mut self, _ctx: &InitVarDefContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code scalarInitVal}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    fn enter_scalarInitVal(&mut self, _ctx: &ScalarInitValContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code scalarInitVal}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    fn exit_scalarInitVal(&mut self, _ctx: &ScalarInitValContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code listInitval}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    fn enter_listInitval(&mut self, _ctx: &ListInitvalContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code listInitval}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    fn exit_listInitval(&mut self, _ctx: &ListInitvalContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#funcDef}.
     * @param ctx the parse tree
     */
    fn enter_funcDef(&mut self, _ctx: &FuncDefContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#funcDef}.
     * @param ctx the parse tree
     */
    fn exit_funcDef(&mut self, _ctx: &FuncDefContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#funcType}.
     * @param ctx the parse tree
     */
    fn enter_funcType(&mut self, _ctx: &FuncTypeContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#funcType}.
     * @param ctx the parse tree
     */
    fn exit_funcType(&mut self, _ctx: &FuncTypeContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#funcFParams}.
     * @param ctx the parse tree
     */
    fn enter_funcFParams(&mut self, _ctx: &FuncFParamsContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#funcFParams}.
     * @param ctx the parse tree
     */
    fn exit_funcFParams(&mut self, _ctx: &FuncFParamsContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#funcFParam}.
     * @param ctx the parse tree
     */
    fn enter_funcFParam(&mut self, _ctx: &FuncFParamContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#funcFParam}.
     * @param ctx the parse tree
     */
    fn exit_funcFParam(&mut self, _ctx: &FuncFParamContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#block}.
     * @param ctx the parse tree
     */
    fn enter_block(&mut self, _ctx: &BlockContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#block}.
     * @param ctx the parse tree
     */
    fn exit_block(&mut self, _ctx: &BlockContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#blockItem}.
     * @param ctx the parse tree
     */
    fn enter_blockItem(&mut self, _ctx: &BlockItemContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#blockItem}.
     * @param ctx the parse tree
     */
    fn exit_blockItem(&mut self, _ctx: &BlockItemContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code assignment}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_assignment(&mut self, _ctx: &AssignmentContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code assignment}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_assignment(&mut self, _ctx: &AssignmentContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code expStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_expStmt(&mut self, _ctx: &ExpStmtContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code expStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_expStmt(&mut self, _ctx: &ExpStmtContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code blockStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_blockStmt(&mut self, _ctx: &BlockStmtContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code blockStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_blockStmt(&mut self, _ctx: &BlockStmtContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code ifStmt1}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_ifStmt1(&mut self, _ctx: &IfStmt1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code ifStmt1}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_ifStmt1(&mut self, _ctx: &IfStmt1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code ifStmt2}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_ifStmt2(&mut self, _ctx: &IfStmt2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code ifStmt2}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_ifStmt2(&mut self, _ctx: &IfStmt2Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code whileStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_whileStmt(&mut self, _ctx: &WhileStmtContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code whileStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_whileStmt(&mut self, _ctx: &WhileStmtContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code breakStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_breakStmt(&mut self, _ctx: &BreakStmtContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code breakStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_breakStmt(&mut self, _ctx: &BreakStmtContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code continueStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_continueStmt(&mut self, _ctx: &ContinueStmtContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code continueStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_continueStmt(&mut self, _ctx: &ContinueStmtContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code returnStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn enter_returnStmt(&mut self, _ctx: &ReturnStmtContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code returnStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    fn exit_returnStmt(&mut self, _ctx: &ReturnStmtContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#exp}.
     * @param ctx the parse tree
     */
    fn enter_exp(&mut self, _ctx: &ExpContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#exp}.
     * @param ctx the parse tree
     */
    fn exit_exp(&mut self, _ctx: &ExpContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#cond}.
     * @param ctx the parse tree
     */
    fn enter_cond(&mut self, _ctx: &CondContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#cond}.
     * @param ctx the parse tree
     */
    fn exit_cond(&mut self, _ctx: &CondContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#lVal}.
     * @param ctx the parse tree
     */
    fn enter_lVal(&mut self, _ctx: &LValContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#lVal}.
     * @param ctx the parse tree
     */
    fn exit_lVal(&mut self, _ctx: &LValContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code primaryExp1}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn enter_primaryExp1(&mut self, _ctx: &PrimaryExp1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code primaryExp1}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn exit_primaryExp1(&mut self, _ctx: &PrimaryExp1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code primaryExp2}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn enter_primaryExp2(&mut self, _ctx: &PrimaryExp2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code primaryExp2}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn exit_primaryExp2(&mut self, _ctx: &PrimaryExp2Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code primaryExp3}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn enter_primaryExp3(&mut self, _ctx: &PrimaryExp3Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code primaryExp3}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    fn exit_primaryExp3(&mut self, _ctx: &PrimaryExp3Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code intnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    fn enter_intnum(&mut self, _ctx: &IntnumContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code intnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    fn exit_intnum(&mut self, _ctx: &IntnumContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code floatnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    fn enter_floatnum(&mut self, _ctx: &FloatnumContext<'input>) {}
    /**
     * Exit a parse tree produced by the {@code floatnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    fn exit_floatnum(&mut self, _ctx: &FloatnumContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code unary1}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn enter_unary1(&mut self, _ctx: &Unary1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code unary1}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn exit_unary1(&mut self, _ctx: &Unary1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code unary2}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn enter_unary2(&mut self, _ctx: &Unary2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code unary2}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn exit_unary2(&mut self, _ctx: &Unary2Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code unary3}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn enter_unary3(&mut self, _ctx: &Unary3Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code unary3}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    fn exit_unary3(&mut self, _ctx: &Unary3Context<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#unaryOp}.
     * @param ctx the parse tree
     */
    fn enter_unaryOp(&mut self, _ctx: &UnaryOpContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#unaryOp}.
     * @param ctx the parse tree
     */
    fn exit_unaryOp(&mut self, _ctx: &UnaryOpContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#funcRParams}.
     * @param ctx the parse tree
     */
    fn enter_funcRParams(&mut self, _ctx: &FuncRParamsContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#funcRParams}.
     * @param ctx the parse tree
     */
    fn exit_funcRParams(&mut self, _ctx: &FuncRParamsContext<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#funcRParam}.
     * @param ctx the parse tree
     */
    fn enter_funcRParam(&mut self, _ctx: &FuncRParamContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#funcRParam}.
     * @param ctx the parse tree
     */
    fn exit_funcRParam(&mut self, _ctx: &FuncRParamContext<'input>) {}
    /**
     * Enter a parse tree produced by the {@code mul2}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    fn enter_mul2(&mut self, _ctx: &Mul2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code mul2}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    fn exit_mul2(&mut self, _ctx: &Mul2Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code mul1}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    fn enter_mul1(&mut self, _ctx: &Mul1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code mul1}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    fn exit_mul1(&mut self, _ctx: &Mul1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code add2}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    fn enter_add2(&mut self, _ctx: &Add2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code add2}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    fn exit_add2(&mut self, _ctx: &Add2Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code add1}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    fn enter_add1(&mut self, _ctx: &Add1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code add1}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    fn exit_add1(&mut self, _ctx: &Add1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code rel2}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    fn enter_rel2(&mut self, _ctx: &Rel2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code rel2}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    fn exit_rel2(&mut self, _ctx: &Rel2Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code rel1}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    fn enter_rel1(&mut self, _ctx: &Rel1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code rel1}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    fn exit_rel1(&mut self, _ctx: &Rel1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code eq1}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    fn enter_eq1(&mut self, _ctx: &Eq1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code eq1}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    fn exit_eq1(&mut self, _ctx: &Eq1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code eq2}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    fn enter_eq2(&mut self, _ctx: &Eq2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code eq2}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    fn exit_eq2(&mut self, _ctx: &Eq2Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code lAnd2}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    fn enter_lAnd2(&mut self, _ctx: &LAnd2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code lAnd2}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    fn exit_lAnd2(&mut self, _ctx: &LAnd2Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code lAnd1}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    fn enter_lAnd1(&mut self, _ctx: &LAnd1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code lAnd1}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    fn exit_lAnd1(&mut self, _ctx: &LAnd1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code lOr1}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    fn enter_lOr1(&mut self, _ctx: &LOr1Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code lOr1}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    fn exit_lOr1(&mut self, _ctx: &LOr1Context<'input>) {}
    /**
     * Enter a parse tree produced by the {@code lOr2}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    fn enter_lOr2(&mut self, _ctx: &LOr2Context<'input>) {}
    /**
     * Exit a parse tree produced by the {@code lOr2}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    fn exit_lOr2(&mut self, _ctx: &LOr2Context<'input>) {}
    /**
     * Enter a parse tree produced by {@link SysYParser#constExp}.
     * @param ctx the parse tree
     */
    fn enter_constExp(&mut self, _ctx: &ConstExpContext<'input>) {}
    /**
     * Exit a parse tree produced by {@link SysYParser#constExp}.
     * @param ctx the parse tree
     */
    fn exit_constExp(&mut self, _ctx: &ConstExpContext<'input>) {}
}

antlr_rust::coerce_from! { 'input : SysYListener<'input> }
