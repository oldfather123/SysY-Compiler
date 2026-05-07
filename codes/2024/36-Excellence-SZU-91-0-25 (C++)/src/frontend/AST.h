#pragma once

#include "../utility/System.h"
#include "../frontend/Token.h"
#include "../midend/Value.h"
#include "../midend/Variable.h"

class ASTNode
{
public:
    virtual ~ASTNode()= default;
    
    virtual void EmitMIR()=0;
    virtual ValueConstant Evaluate(){return ValueConstant(-1);}
    virtual void Print(int depth)=0;
};



class TranslationUnitAST:public ASTNode
{
public:
    vector<unique_ptr<ASTNode>> variable_declarations;
    vector<unique_ptr<ASTNode>> function_definitions; 

    void EmitMIR() override;
    void Print(int depth) override;
};



class VariableDeclarationAST:public ASTNode
{
public:
    bool is_const;
    Token variable_type;
    vector<unique_ptr<ASTNode>> variable_definitions;

    void EmitMIR() override;
    void Print(int depth) override;
};

class VariableDefinitionAST:public ASTNode
{
public:
    Token variable_name;
    vector<unique_ptr<ASTNode>> expressions;
    unique_ptr<ASTNode> variable_initvalue;

    void EmitMIR() override;
    void Print(int depth) override;
};

class VariableInitValueAST:public ASTNode
{
public:
    unique_ptr<ASTNode> expression;
    vector<unique_ptr<ASTNode>> variable_initvalues;

    void EmitMIR() override;
    void Print(int depth) override;

    void FillArrayConst(vector<unique_ptr<ASTNode>>& init_values,Variable& def_var,
                        vector<int>& dimensions,int& index);
    void FillArrayVariable(vector<unique_ptr<ASTNode>>& init_values,Variable& def_var,
                            vector<int>& dimensions,int& index);       
};



class FunctionDefinitionAST:public ASTNode
{
public:
    Token return_type;
    Token function_name;
    unique_ptr<ASTNode>parameter_list;
    vector<unique_ptr<ASTNode>>statements;

    void EmitMIR() override;
    void Print(int depth) override;
};

class ParameterListAST:public ASTNode
{
public:
    vector<unique_ptr<ASTNode>> parameters;

    void EmitMIR() override;
    void Print(int depth) override;
};

class ParameterAST:public ASTNode
{
public:
    Token parameter_type;
    Token parameter_name;
    bool is_ptr;
    vector<unique_ptr<ASTNode>> expressions;

    void EmitMIR() override;
    void Print(int depth) override;
};



class CompoundStatementAST:public ASTNode
{
public:
    vector<unique_ptr<ASTNode>> statements;

    void EmitMIR() override;
    void Print(int depth) override;
};

class StatementAST:public ASTNode
{
public:
    unique_ptr<ASTNode> x_statement;

    void EmitMIR() override;
    void Print(int depth) override;
};

class AssignStatementAST:public ASTNode
{
public:
    Token variable_name;
    vector<unique_ptr<ASTNode>> expressions;
    unique_ptr<ASTNode> expression;

    void EmitMIR() override;
    void Print(int depth) override;
};

class ExpressionStatementAST:public ASTNode
{
public:
    unique_ptr<ASTNode> expression;

    void EmitMIR() override;
    void Print(int depth) override;
};

class IfStatementAST:public ASTNode
{
public:
    unique_ptr<ASTNode> condition_expression;
    unique_ptr<ASTNode> true_statement;
    unique_ptr<ASTNode> false_statement;

    void EmitMIR() override;
    void Print(int depth) override;
};

class WhileStatementAST:public ASTNode
{
public:
    unique_ptr<ASTNode> condition_expression;
    unique_ptr<ASTNode> statement;

    void EmitMIR() override;
    void Print(int depth) override;
};

class BreakStatementAST:public ASTNode
{
public:
    Token break_t;

    void EmitMIR() override;
    void Print(int depth) override;
};

class ContinueStatementAST:public ASTNode
{
public:
    Token continue_t;

    void EmitMIR() override;
    void Print(int depth) override;
};

class ReturnStatementAST:public ASTNode
{
public:
    Token return_t;

    unique_ptr<ASTNode> expression;

    void EmitMIR() override;
    void Print(int depth) override;
};



class ConditionExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> logicor_expression;

    void EmitMIR() override;
    void Print(int depth) override;
};

class ExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> addsub_expression;

    void EmitMIR() override;
    ValueConstant Evaluate() override;
    void Print(int depth) override;
};

class LogicOrExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> logicand_expression;
    vector<Token> infix_operators;
    vector<unique_ptr<ASTNode>> logicand_expressions;

    void EmitMIR() override;
    void Print(int depth) override;
};

class LogicAndExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> equal_expression;
    vector<Token> infix_operators;
    vector<unique_ptr<ASTNode>> equal_expressions;

    void EmitMIR() override;
    void Print(int depth) override;
};

class EqualExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> relation_expression;
    vector<Token> infix_operators;
    vector<unique_ptr<ASTNode>> relation_expressions;

    void EmitMIR() override;
    void Print(int depth) override;
};

class RelationExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> addsub_expression;
    vector<Token> infix_operators;
    vector<unique_ptr<ASTNode>> addsub_expressions;

    void EmitMIR() override;
    void Print(int depth) override;
};

class AddSubExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> muldiv_expression;
    vector<Token> infix_operators;
    vector<unique_ptr<ASTNode>> muldiv_expressions;

    void EmitMIR() override;
    ValueConstant Evaluate() override;
    void Print(int depth) override;
};

class MulDivExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> unary_expression;
    vector<Token> infix_operators;
    vector<unique_ptr<ASTNode>> unary_expressions;

    void EmitMIR() override;
    ValueConstant Evaluate() override;
    void Print(int depth) override;
};

class UnaryExpressionAST:public ASTNode
{
public:
    vector<Token> prefix_operators;
    unique_ptr<ASTNode> primary_expression;
    unique_ptr<ASTNode> functioncall_expression;

    void EmitMIR() override;
    ValueConstant Evaluate() override;
    void Print(int depth) override;
};

class PrimaryExpressionAST:public ASTNode
{
public:
    unique_ptr<ASTNode> expression;
    vector<unique_ptr<ASTNode>> expressions;
    Token variable_name;
    Token constant;

    void EmitMIR() override;
    ValueConstant Evaluate() override;
    void Print(int depth) override;
};

class FunctionCallExpressionAST:public ASTNode
{
public:
    Token function_name;
    vector<unique_ptr<ASTNode>> expressions;

    void EmitMIR() override;
    void Print(int depth) override;
};
