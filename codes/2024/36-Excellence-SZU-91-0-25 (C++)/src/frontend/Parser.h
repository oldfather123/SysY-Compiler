#pragma once

#include "../utility/System.h"
#include "../utility/Cursor.h"
#include "../frontend/Token.h"
#include "../frontend/AST.h"

class Parser
{
private:
    int current;

public:
    Parser():current(0){}

    void Parse();

    unique_ptr<ASTNode> ParseTranslationUnit();

    unique_ptr<ASTNode> ParseVariableDeclaration();
    unique_ptr<ASTNode> ParseVariableDefinition();
    unique_ptr<ASTNode> ParseVariableInitValue();

    unique_ptr<ASTNode> ParseFunctionDefinition();
    unique_ptr<ASTNode> ParseParameterList();
    unique_ptr<ASTNode> ParseParameter();

    unique_ptr<ASTNode> ParseCompoundStatement();
    unique_ptr<ASTNode> ParseStatement();
    unique_ptr<ASTNode> ParseAssignStatement();
    unique_ptr<ASTNode> ParseExpressionStatement();
    unique_ptr<ASTNode> ParseIfStatement();
    unique_ptr<ASTNode> ParseWhileStatement();
    unique_ptr<ASTNode> ParseBreakStatement();
    unique_ptr<ASTNode> ParseContinueStatement();
    unique_ptr<ASTNode> ParseReturnStatement();

    unique_ptr<ASTNode> ParseConditionExpression();
    unique_ptr<ASTNode> ParseExpression();
    unique_ptr<ASTNode> ParseLogicOrExpression();
    unique_ptr<ASTNode> ParseLogicAndExpression();
    unique_ptr<ASTNode> ParseEqualExpression();
    unique_ptr<ASTNode> ParseRelationExpression();
    unique_ptr<ASTNode> ParseAddSubExpression();
    unique_ptr<ASTNode> ParseMulDivExpression();
    unique_ptr<ASTNode> ParseUnaryExpression();
    unique_ptr<ASTNode> ParsePrimaryExpression();
    unique_ptr<ASTNode> ParseFunctionCallExpression();

    //Tool
    bool IsAtEnd();
    bool Match(TokenType expected);
    void MatchSemicolon();
    void MatchLeftParen();
    void MatchRightParen();
    void MatchLeftSquare();
    void MatchRightSquare();
    void MatchLeftBrace();
    void MatchRightBrace();

    bool Peek(TokenType expected);
    bool Peek(TokenType expected,int n);
    void AddChildToVector(vector<unique_ptr<ASTNode>>& vec,unique_ptr<ASTNode> child);
    Token PrevToken();
    Cursor CurrentLocation();
};

extern Parser parser;