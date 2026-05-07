#include "ast.h"

void CompUnit::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void DeclDef::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Def::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Decl::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void InitVal::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void FuncDef::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void FuncFParam::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Block::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Stmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void IfStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void WhileStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void AddExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void MulExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void UnaryExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void PrimaryExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void NumberNode::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Call::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void LVal::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void RelExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void EqExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void LAndExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void LOrExp::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void BlockItem::accept(ASTVisitor &visitor) { visitor.visit(*this); }
