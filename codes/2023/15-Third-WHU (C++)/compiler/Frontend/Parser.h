#pragma once
#ifndef PASER_H
#define PASER_H
#include "AST.h"
#include"SymbolTable.h"

void error(std::string str);
int findEndComment(FILE* lsource);
static int gettok(FILE* lsource);

static int getNextToken();
static int GetTokPrecedence();
std::shared_ptr<ExprAST> MainLoop(FILE* lsource, ASTContext* ctx, SymbolTable* symbolTable, std::vector<std::string>& StrVector);
static int HandleDefinition();
void setBinopPrecedence();


static int HandleFunctionDef(ValType* valtype,std::string name);
static int  HandleExpression(std::string name, ValType* valtype);
static std::shared_ptr<ExprAST> ParseExpression();
static std::shared_ptr<UnaryExprAST> ParseUnary();
static std::shared_ptr<ExprAST> ParsePrimary();
static std::shared_ptr<ExprAST> ParseIdentifierExpr();
static std::shared_ptr<ExprAST> ParseParenExpr();
static std::shared_ptr<ExprAST> ParseFloatExpr();
static std::shared_ptr<ExprAST> ParseIntExpr();
static std::shared_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::shared_ptr<ExprAST> LHS);
static std::shared_ptr<ExprAST> ParseCompoundStmt();
static std::shared_ptr<ExprAST> ParseReturnStmt();
static std::shared_ptr<ExprAST> ParseIfStmt();
static std::shared_ptr<ExprAST> ParseWhileStmt();
static std::shared_ptr<ExprAST> ParseValStmt();
static std::shared_ptr<ExprAST> ParseArrayDef(std::string iname, ValType* valtype);
static ArrayType* ParseArrayType(ValType* valtype);
static std::shared_ptr<ExprAST> ParseArrayInitial(ValType* valtype);
static std::shared_ptr<ExprAST> ParseStmt();
static std::shared_ptr<ExprAST> ParseExpressionForExpr(std::shared_ptr<ExprAST> LHS);
static ArrayType* ParsePointType(ValType* valtype);


#endif // !PASER_H



