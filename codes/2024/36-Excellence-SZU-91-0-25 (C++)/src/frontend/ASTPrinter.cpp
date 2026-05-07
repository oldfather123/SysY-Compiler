#include "../frontend/AST.h"

//Tool
void PrintTab(int n)
{
    cout<<endl;
    for(int i=0;i<n;i++)
        cout<<"\t";
}

void PrintName(string ast_name)
{
    cout<<ast_name;
}

void PrintStuff(string stuff)
{
    cout<<stuff;
}

void PrintNode(unique_ptr<ASTNode>& node,int depth)
{
    node->Print(depth);
}

void PrintChild(vector<unique_ptr<ASTNode>>& vec,int depth)
{
    for(unique_ptr<ASTNode>& ast_ptr:vec)
        PrintNode(ast_ptr,depth);
}

void PrintInfixExpr(vector<Token>& infix_operators,
    vector<unique_ptr<ASTNode>>& infix_expressions,int depth)
{
    for(int i=0;i<infix_operators.size();i++)
    {
        PrintTab(depth);
        PrintStuff("infix_operator:"+infix_operators[i].lexeme);
        PrintNode(infix_expressions[i],depth);
    }
}



//Begin
void TranslationUnitAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("TranslationUnit");

    PrintChild(variable_declarations,depth+1);
    PrintChild(function_definitions,depth+1);
}



void VariableDeclarationAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("VariableDeclaration");
    
    if(is_const)
        PrintStuff(" <const>");
    PrintStuff(" variable_type:"+variable_type.lexeme);
    PrintChild(variable_definitions,depth+1);
}

void VariableDefinitionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("VariableDefinition");

    PrintStuff(" variable_name:"+variable_name.lexeme);
    PrintChild(expressions,depth+1);
    if(variable_initvalue)
        PrintNode(variable_initvalue,depth+1);
}

void VariableInitValueAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("VariableInitValue");

    if(expression)
        PrintNode(expression,depth+1);
    else
        PrintChild(variable_initvalues,depth+1);
}



void FunctionDefinitionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("FunctionDefinition");

    PrintStuff(" return_type:"+return_type.lexeme);
    PrintStuff(" function_name:"+function_name.lexeme);
    if(parameter_list)
        PrintNode(parameter_list,depth+1);
    PrintChild(statements,depth+1);
}

void ParameterListAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("ParameterList");

    PrintChild(parameters,depth+1);
}

void ParameterAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("Parameter");

    PrintStuff(" parameter_type:"+parameter_type.lexeme);
    PrintStuff(" parameter_name:"+parameter_name.lexeme);
    if(is_ptr)
        PrintStuff(" <ptr>");
    PrintChild(expressions,depth+1);
}



void CompoundStatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("CompoundStatement");

    PrintChild(statements,depth+1);
}

void StatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("Statement");

    PrintNode(x_statement,depth+1);
}

void AssignStatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("AssignStatement");

    PrintStuff(" variable_name:"+variable_name.lexeme);
    PrintChild(expressions,depth+1);
    PrintTab(depth+1);PrintStuff("value:");
    PrintNode(expression,depth+1);
}

void ExpressionStatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("ExpressionStatement");

    if(expression)
        PrintNode(expression,depth+1);
}

void IfStatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("IfStatement");

    PrintNode(condition_expression,depth+1);
    PrintNode(true_statement,depth+1);
    if(false_statement)
        PrintNode(false_statement,depth+1);
}

void WhileStatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("WhileStatement");

    PrintNode(condition_expression,depth+1);
    PrintNode(statement,depth+1);
}

void BreakStatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("BreakStatement");
}

void ContinueStatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("ContinueStatement");
}

void ReturnStatementAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("ReturnStatement");

    if(expression)
    {
        PrintStuff(" <have return_value>");
        PrintNode(expression,depth+1);
    }
}



void ConditionExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("ConditionExpression");

    PrintNode(logicor_expression,depth+1);
}

void ExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("Expression");

    PrintNode(addsub_expression,depth+1);
}

void LogicOrExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("LogicOrExpression");

    PrintNode(logicand_expression,depth+1);
    PrintInfixExpr(infix_operators,logicand_expressions,depth+1);
}

void LogicAndExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("LogicAndExpression");

    PrintNode(equal_expression,depth+1);
    PrintInfixExpr(infix_operators,equal_expressions,depth+1);
}

void EqualExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("EqualExpression");

    PrintNode(relation_expression,depth+1);
    PrintInfixExpr(infix_operators,relation_expressions,depth+1);
}

void RelationExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("RelationExpression");

    PrintNode(addsub_expression,depth+1);
    PrintInfixExpr(infix_operators,addsub_expressions,depth+1);
}

void AddSubExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("AddSubExpression");

    PrintNode(muldiv_expression,depth+1);
    PrintInfixExpr(infix_operators,muldiv_expressions,depth+1);
}

void MulDivExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("MulDivExpression");

    PrintNode(unary_expression,depth+1);
    PrintInfixExpr(infix_operators,unary_expressions,depth+1);
}

void UnaryExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("UnaryExpression");

    if(!prefix_operators.empty())
    {
        PrintStuff(" prefix_operators:");
        for(Token& prefix_operator:prefix_operators)
            PrintStuff(" "+prefix_operator.lexeme);
    }
    if(primary_expression)
        PrintNode(primary_expression,depth+1);
    else if(functioncall_expression)
        PrintNode(functioncall_expression,depth+1);
}

void PrimaryExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("PrimaryExpression");
    
    if(expression)
    {
        PrintStuff(" ( )");
        PrintNode(expression,depth+1);
    } 
    else if(variable_name.is_valid)
    {
        PrintStuff(" variable_name:"+variable_name.lexeme);
        PrintChild(expressions,depth+1);
    }
    else if(constant.is_valid)
        PrintStuff(" "+TokenTypeText[constant.token_type]+":"+constant.lexeme);
}

void FunctionCallExpressionAST::Print(int depth)
{
    PrintTab(depth);
    PrintName("FunctionCallExpression");

    PrintStuff(" function_name:"+function_name.lexeme);
    PrintChild(expressions,depth+1);
}
