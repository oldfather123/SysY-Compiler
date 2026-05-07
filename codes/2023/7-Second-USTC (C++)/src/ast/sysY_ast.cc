#include "sysY_ast.hh"

#include "logging.hh"
#include <iostream>
#include <string>

void ASTCompUnit::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTConstDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTConstDef::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTConstInitVal::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTVarDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTVarDef::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTInitVal::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTFuncDef::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTFuncFParam::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTBlock::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTAssignStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTExpStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTBlockStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTSelectStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTIterationStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTBreakStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTContinueStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTLVal::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTNumber::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTUnaryExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTCallee::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTMulExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTAddExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTRelExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTEqExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTLAndExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ASTLOrExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ASTDeclaration::accept(ASTVisitor &visitor)
{
    auto var_decl = dynamic_cast<ASTVarDeclaration *>(this);
    if (var_decl)
    {
        var_decl->accept(visitor);
        return;
    }
    auto const_decl = dynamic_cast<ASTConstDeclaration *>(this);
    if (const_decl)
    {
        const_decl->accept(visitor);
        return;
    }
    auto fun_decl = dynamic_cast<ASTFuncDef *>(this);
    if (fun_decl)
    {
        fun_decl->accept(visitor);
        return;
    }
    LOG(ERROR) << "Abort due to node cast error.";
}

void ASTStmt::accept(ASTVisitor &visitor)
{
    auto assign_stmt = dynamic_cast<ASTAssignStmt *>(this);
    if (assign_stmt)
    {
        assign_stmt->accept(visitor);
        return;
    }
    auto exp_stmt = dynamic_cast<ASTExpStmt *>(this);
    if (exp_stmt)
    {
        exp_stmt->accept(visitor);
        return;
    }
    auto block_stmt = dynamic_cast<ASTBlockStmt *>(this);
    if (block_stmt)
    {
        block_stmt->accept(visitor);
        return;
    }
    auto selec_stmt = dynamic_cast<ASTSelectStmt *>(this);
    if (selec_stmt)
    {
        selec_stmt->accept(visitor);
        return;
    }
    auto iter_stmt = dynamic_cast<ASTIterationStmt *>(this);
    if (iter_stmt)
    {
        iter_stmt->accept(visitor);
        return;
    }
    auto break_stmt = dynamic_cast<ASTBreakStmt *>(this);
    if (break_stmt)
    {
        break_stmt->accept(visitor);
        return;
    }
    auto continue_stmt = dynamic_cast<ASTContinueStmt *>(this);
    if (continue_stmt)
    {
        continue_stmt->accept(visitor);
        return;
    }
    auto ret_stmt = dynamic_cast<ASTReturnStmt *>(this);
    if (ret_stmt)
    {
        ret_stmt->accept(visitor);
        return;
    }
    LOG(ERROR) << "Abort due to node cast error.";
}

void ASTExp::accept(ASTVisitor &visitor)
{
    if (this->AddExp == nullptr)
        std::cout << "error null" << std::endl;
    this->AddExp->accept(visitor);
}

void ASTCond::accept(ASTVisitor &visitor)
{
    this->LOrExp->accept(visitor);
}

void ASTBlockItem::accept(ASTVisitor &visitor)
{
    if (this->ConstDecl)
    {
        this->ConstDecl->accept(visitor);
        return;
    }
    else if (this->VarDecl)
    {
        this->VarDecl->accept(visitor);
        return;
    }
    else if (this->Stmt)
    {
        this->Stmt->accept(visitor);
        return;
    }
    LOG(ERROR) << "Abort due to node cast error.";
}

void ASTPrimaryExp::accept(ASTVisitor &visitor)
{
    if (this->Exp)
    {
        this->Exp->accept(visitor);
        return;
    }
    else if (this->LVal)
    {
        this->LVal->accept(visitor);
        return;
    }
    else if (this->Number)
    {
        this->Number->accept(visitor);
        return;
    }
    LOG(ERROR) << "Abort due to node cast error.";
}

void ASTConstExp::accept(ASTVisitor &visitor)
{
    this->AddExp->accept(visitor);
}

#define _DEBUG_PRINT_N_(N)                \
    {                                     \
        std::cout << std::string(N, '-'); \
    }

#define _TYPE_(t) (((t) == ASTType::TYPE_INT) ? "int" : (((t) == ASTType::TYPE_FLOAT) ? "float" : "void"))

void ASTPrinter::visit(ASTCompUnit &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "compunit" << std::endl;
    add_depth();
    for (auto decl : node.Declaration_list)
    {
        decl->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTConstDeclaration &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstDecl" << _TYPE_(node.type) << std::endl;
    add_depth();
    for (auto constdef : node.ConstDef_list)
    {
        constdef->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTConstDef &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstDef:" << '\t' << node.id << std::endl;
    add_depth();
    for (auto constexp : node.ConstExp_list)
    {
        constexp->accept(*this);
    }
    if (node.ConstInitVal)
    {
        node.ConstInitVal->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTConstInitVal &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstInitVal" << std::endl;
    add_depth();
    if (node.ConstExp)
    {
        node.ConstExp->accept(*this);
    }
    else
    {
        for (auto constval : node.ConstInitVal_list)
        {
            constval->accept(*this);
        }
    }
    remove_depth();
}

void ASTPrinter::visit(ASTVarDeclaration &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "VarDecl:\t" << _TYPE_(node.type) << std::endl;
    add_depth();
    for (auto vardef : node.VarDef_list)
    {
        vardef->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTVarDef &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "VarDef:" << '\t' << node.id << std::endl;
    add_depth();
    for (auto constexp : node.ConstExp_list)
    {
        constexp->accept(*this);
    }
    if (node.InitVal)
    {
        node.InitVal->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTInitVal &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "InitVal" << std::endl;
    add_depth();
    if (node.Exp)
    {
        node.Exp->accept(*this);
    }
    else
    {
        for (auto constval : node.InitVal_list)
        {
            constval->accept(*this);
        }
    }
    remove_depth();
}

void ASTPrinter::visit(ASTFuncDef &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "FuncDef:" << '\t' << _TYPE_(node.type) << node.id << std::endl;
    add_depth();
    for (auto funcparam : node.FuncFParam_list)
    {
        funcparam->accept(*this);
    }
    if (node.Block)
    {
        node.Block->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTFuncFParam &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ParamDef:" << _TYPE_(node.type) << node.id << "\tisarray:" << node.is_array << '\n';
    add_depth();
    for (auto exp : node.ParamArrayExp_list)
    {
        if (exp != nullptr)
        {
            exp->accept(*this);
        }
    }
    remove_depth();
}

void ASTPrinter::visit(ASTBlock &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Block" << node.BlockItem_list.size() << std::endl;
    add_depth();
    for (auto block_item : node.BlockItem_list)
    {
        if (block_item == nullptr)
            std::cout << "nullptr!" << std::endl;
        block_item->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTAssignStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "AssignStmt" << std::endl;
    add_depth();
    node.LVal->accept(*this);
    node.Exp->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTExpStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ExpStmt" << std::endl;
    if (node.Exp != nullptr)
    {
        add_depth();
        node.Exp->accept(*this);
        remove_depth();
    }
}

void ASTPrinter::visit(ASTBlockStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "BlockStmt" << std::endl;
    add_depth();
    node.Block->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTSelectStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "SelectStmt" << std::endl;
    add_depth();
    node.Cond->accept(*this);
    node.IfStmt->accept(*this);
    if (node.ElseStmt)
    {
        node.ElseStmt->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTIterationStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "IterationStmt" << std::endl;
    add_depth();
    node.Cond->accept(*this);
    node.Stmt->accept(*this);
    remove_depth();
}

void ASTPrinter::visit(ASTBreakStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "BreakStmt;" << std::endl;
}

void ASTPrinter::visit(ASTContinueStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ContinueStmt;" << std::endl;
}

void ASTPrinter::visit(ASTReturnStmt &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "ReturnStmt" << std::endl;
    add_depth();
    if (node.Exp)
    {
        node.Exp->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTLVal &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Lval:\t" << node.id << std::endl;
    add_depth();
    for (auto p : node.ArrayExp_list)
    {
        p->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTNumber &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Number:\t";
    if (node.type == ASTType::TYPE_INT)
        std::cout << node.i_val << '\n';
    else
    {
        std::cout << node.f_val << '\n';
    }
}

void ASTPrinter::visit(ASTUnaryExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "UaryExp" << std::endl;
    add_depth();
    if (node.PrimaryExp)
    {
        node.PrimaryExp->accept(*this);
    }
    else if (node.Callee)
    {
        node.Callee->accept(*this);
    }
    else
    {
        _DEBUG_PRINT_N_(depth);
        std::cout << node.op;
        node.UnaryExp->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTCallee &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "Call:\t" << node.id << std::endl;
    add_depth();
    for (auto exp : node.ParamExp_list)
    {
        exp->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTMulExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "MulExp:\t" << node.op << std::endl;
    add_depth();
    if (node.MulExp)
    {
        node.MulExp->accept(*this);
    }
    if (node.UnaryExp)
    {
        node.UnaryExp->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTAddExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "AddExp:\t" << node.op << std::endl;
    add_depth();
    if (node.AddExp)
    {
        node.AddExp->accept(*this);
    }
    if (node.MulExp)
    {
        node.MulExp->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTRelExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "RelExp:\t" << node.op << std::endl;
    add_depth();
    if (node.RelExp)
    {
        node.RelExp->accept(*this);
    }
    if (node.AddExp)
    {
        node.AddExp->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTEqExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "EqExp:\t" << node.op << std::endl;
    add_depth();
    if (node.EqExp)
    {
        node.EqExp->accept(*this);
    }
    if (node.RelExp)
    {
        node.RelExp->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTLAndExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "LAndExp:\t" << node.op << std::endl;
    add_depth();
    if (node.LAndExp)
    {
        node.LAndExp->accept(*this);
    }
    if (node.EqExp)
    {
        node.EqExp->accept(*this);
    }
    remove_depth();
}

void ASTPrinter::visit(ASTLOrExp &node)
{
    _DEBUG_PRINT_N_(depth);
    std::cout << "LOrExp:\t" << node.op << std::endl;
    add_depth();
    if (node.LOrExp)
    {
        node.LOrExp->accept(*this);
    }
    if (node.LAndExp)
    {
        node.LAndExp->accept(*this);
    }
    remove_depth();
}


void InitZeroJudger::visit(ASTCompUnit &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTConstDeclaration &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTConstDef &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTConstInitVal &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTVarDeclaration &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTVarDef &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTInitVal &node) {
    if (node.Exp) {
        node.Exp->accept(*this);
    } else {
        for (auto constval : node.InitVal_list) {
            constval->accept(*this);
        }
    }
}

void InitZeroJudger::visit(ASTFuncDef &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTFuncFParam &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTBlock &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTAssignStmt &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTExpStmt &node) {
    if (node.Exp != nullptr) {
        node.Exp->accept(*this);
    }
}

void InitZeroJudger::visit(ASTBlockStmt &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTSelectStmt &node) {
     all_zero = false;
}

void InitZeroJudger::visit(ASTIterationStmt &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTBreakStmt &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTContinueStmt &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTReturnStmt &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTLVal &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTNumber &node) {
    if (node.type == ASTType::TYPE_INT) {
        if(node.i_val!=0) {
            all_zero = false;
            return;
        }
    } else {
        if(node.f_val!=0){
            all_zero = false;
            return;
        }
    }
}

void InitZeroJudger::visit(ASTUnaryExp &node) {
    if (node.PrimaryExp) {
        node.PrimaryExp->accept(*this);
    } else if (node.Callee) {
        all_zero = false;
    } else {
        all_zero = false;
    }
}

void InitZeroJudger::visit(ASTCallee &node) {
    all_zero = false;

}

void InitZeroJudger::visit(ASTMulExp &node) {
    if (node.MulExp) {
            all_zero = false;
    }
    if (node.UnaryExp) {
        node.UnaryExp->accept(*this);
    }
}

void InitZeroJudger::visit(ASTAddExp &node)
{
    if (node.AddExp) {
            all_zero = false;
    }
    if (node.MulExp) {
        node.MulExp->accept(*this);
    }
}

void InitZeroJudger::visit(ASTRelExp &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTEqExp &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTLAndExp &node) {
    all_zero = false;
}

void InitZeroJudger::visit(ASTLOrExp &node) {
    all_zero = false;
}


