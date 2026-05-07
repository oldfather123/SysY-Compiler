#pragma once
#ifndef Sem_Ana
#define Sem_Ana

#include "AST.h"
#include "Type.h"


int SemanticAnalyze(ASTContext* stx);


static std::shared_ptr<ExprAST> AddImplicitCastForOp(ValType* valtype,std::shared_ptr<ExprAST> Expr);

static std::shared_ptr<ExprAST> AddImplicitCastForEqual(ValType* valtype,Type::TypeID LID, Type::TypeID RID, std::shared_ptr<ExprAST> Expr);

#endif // !Sem_Ana

