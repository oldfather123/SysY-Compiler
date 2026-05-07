#include "SyntaxTree.hpp"
#include <unordered_map>
#include <utility>
//a--难度
void SyntaxAnalyseCompUnit(ast::compunit_syntax* &self, ast::compunit_syntax* compunit, ast::syntax_tree_node* func_def);
void SyntaxAnalyseFuncDef(ast::func_def_syntax* &self, vartype var_type, char* Ident,ast::block_syntax * block);
void SyntaxAnalyseFuncFDef(ast::func_f_param_syntax *&self, vartype var_type, char* ident, ast::var_dimension_syntax* dimension, bool has_dim);
void SyntaxAnalyseFuncFDecl(ast::func_def_syntax* &self, ast::func_f_param_syntax *var_def, ast::func_def_syntax *var_def_group);
void SyntaxAnalyseFuncFDeclGroup(ast::func_def_syntax* &self, ast::func_f_param_syntax *var_def, ast::func_def_syntax *var_def_group);
void SynataxAnalyseFuncType(vartype &self, char* type);
void SynataxAnalyseBlock(ast::block_syntax* &self, ast::block_syntax* block_items);
void SynataxAnalyseBlockItems(ast::block_syntax* &self,ast::block_syntax* block_items, ast::stmt_syntax* stmt);
void SynataxAnalyseStmtReturn(ast::stmt_syntax* &self, ast::expr_syntax* exp);
void SynataxAnalysePrimaryExpIntConst(ast::expr_syntax* &self, char* current_symbol);
void SynataxAnalysePrimaryExpFloatConst(ast::expr_syntax *&self, char *current_symbol);
//a-难度
void SynataxAnalyseStmtBlock(ast::stmt_syntax* &self, ast::block_syntax *block);
void SynataxAnalysePrimaryExpVar(ast::expr_syntax* &self, char* current_symbol);
void SynataxAnalyseVarType(vartype &self, char* type);
void SynataxAnalyseVarDecl(ast::stmt_syntax* &self, vartype var_type, ast::var_def_stmt_syntax *var_def,ast::var_decl_stmt_syntax *var_def_group, bool is_const);
void SynataxAnalyseVarDefGroup(ast::var_decl_stmt_syntax * &self, ast::var_def_stmt_syntax *var_def,ast::var_decl_stmt_syntax *var_def_group);
void SynataxAnalyseVarDef(ast::var_def_stmt_syntax *&self,char* ident, ast::var_dimension_syntax* current_dim,ast::init_syntax* init);
void SynataxAnalyseAddExp(ast::expr_syntax* &self,ast::expr_syntax* exp1,char * op,ast::expr_syntax* exp2);
//a难度
void SynataxAnalyseMulExp(ast::expr_syntax* &self,ast::expr_syntax* exp1,char * op,ast::expr_syntax* exp2);
void SynataxAnalyseStmtAssign(ast::stmt_syntax *&self,ast::lval_syntax* target,ast::expr_syntax* value);
void SynataxAnalyseLval(ast::lval_syntax *&self,char *ident, ast::var_dimension_syntax* current_dim);
//a+难度
void SynataxAnalyseStmtIf(ast::stmt_syntax *&self,ast::expr_syntax *cond,ast::stmt_syntax *then_body,ast::stmt_syntax *else_body);
void SynataxAnalyseLOrExp(ast::expr_syntax* &self,ast::expr_syntax* cond1,ast::expr_syntax* cond2);
void SynataxAnalyseLAndExp(ast::expr_syntax* &self,ast::expr_syntax* cond1,ast::expr_syntax* cond2);
void SynataxAnalyseEqExp(ast::expr_syntax* &self,ast::expr_syntax* cond1,char * op,ast::expr_syntax* cond2);
void SynataxAnalyseRelExp(ast::expr_syntax* &self,ast::expr_syntax *cond1,char * op,ast::expr_syntax *exp);
//a++难度
void SynataxAnalyseUnaryExp(ast::expr_syntax* &self,char * op,ast::expr_syntax* exp);
void SyntaxAnalyseVarDimension(ast::var_dimension_syntax* &self, ast::expr_syntax* new_dimension, ast::var_dimension_syntax* current_dim);
void SynataxAnalyseStmtExp(ast::stmt_syntax* &self, ast::expr_syntax *exp);
// void SynataxAnalyseStmtEmpty(ast::stmt_syntax* &self);
void SynataxAnalyseStmtWhile(ast::stmt_syntax* &self, ast::expr_syntax* cond, ast::stmt_syntax *while_body);
void SynataxAnalyseInitVal(ast::init_syntax* &self, ast::expr_syntax* exp, ast::init_syntax* new_init, ast::init_syntax* init_group);
void SynataxAnalyseInitValGroup(ast::init_syntax* &self, ast::init_syntax* new_init, ast::init_syntax* init_group);
void SyntaxAnalyseFuncCall(ast::expr_syntax* &self, char* func_name, ast::expr_syntax* param, ast::func_call_syntax* param_group);
void SyntaxAnalyseFuncCallGroup(ast::func_call_syntax* &self, ast::expr_syntax* param, ast::func_call_syntax* param_group);