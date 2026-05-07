#include "../frontend/Parser.h"
#include "../utility/Product.h"
#include "../utility/Diagnostor.h"

Parser parser;

void Parser::Parse()
{
    ast=ParseTranslationUnit();
}



unique_ptr<ASTNode> Parser::ParseTranslationUnit()
{
    WhoAmI("ParseTranslationUnit");

    unique_ptr<TranslationUnitAST> node=make_unique<TranslationUnitAST>();

    while(!IsAtEnd())
    {
        if(Peek(LEFT_PAREN,3))
            AddChildToVector(node->function_definitions,
                            ParseFunctionDefinition());
        else
            AddChildToVector(node->variable_declarations,
                            ParseVariableDeclaration());
    }

    return node;
}



unique_ptr<ASTNode> Parser::ParseVariableDeclaration()
{
    WhoAmI("ParseVariableDeclaration");

    unique_ptr<VariableDeclarationAST> node=make_unique<VariableDeclarationAST>();

    if(Match(CONST))
        node->is_const=true;
    else 
        node->is_const=false;
        
    if(Match(T_INT)||Match(T_FLOAT))
        node->variable_type=PrevToken();   
    else ParseError("Vairable type loss",CurrentLocation());

    AddChildToVector(node->variable_definitions,
                    ParseVariableDefinition());
    while(Match(COMMA))
        AddChildToVector(node->variable_definitions,
                    ParseVariableDefinition());
    
    MatchSemicolon();

    return node;
}

unique_ptr<ASTNode> Parser::ParseVariableDefinition()
{
    WhoAmI("ParseVariableDefinition");

    unique_ptr<VariableDefinitionAST> node=make_unique<VariableDefinitionAST>();

    if(Match(IDENTIFIER))
        node->variable_name=PrevToken();
    else ParseError("Variable name loss",CurrentLocation());

    while(Match(LEFT_SQUARE))
    {
        AddChildToVector(node->expressions,
                        ParseExpression());
        MatchRightSquare();
    }

    if(Match(ASSIGN))
        node->variable_initvalue=ParseVariableInitValue();

    return node;
}

unique_ptr<ASTNode> Parser::ParseVariableInitValue()
{
    WhoAmI("ParseVariableInitValue");

    unique_ptr<VariableInitValueAST> node=make_unique<VariableInitValueAST>();

    if(Match(LEFT_BRACE))
    {
        if(!Peek(RIGHT_BRACE))
        {
            AddChildToVector(node->variable_initvalues,
                            ParseVariableInitValue());
            while(Match(COMMA))
                AddChildToVector(node->variable_initvalues,
                                ParseVariableInitValue());
        }
        MatchRightBrace();
    }
    else
        node->expression=ParseExpression();

    return node;
}



unique_ptr<ASTNode> Parser::ParseFunctionDefinition()
{
    WhoAmI("ParseFunctionDefinition");

    unique_ptr<FunctionDefinitionAST> node=make_unique<FunctionDefinitionAST>();

    if(Match(T_VOID)||Match(T_INT)||Match(T_FLOAT))
        node->return_type=PrevToken();
    else ParseError("MIRFunction return type loss",CurrentLocation());
    
    if(Match(IDENTIFIER))
        node->function_name=PrevToken();
    else ParseError("MIRFunction name loss",CurrentLocation());

    MatchLeftParen();
    if(!Peek(RIGHT_PAREN))
        node->parameter_list=ParseParameterList();
    MatchRightParen();

    MatchLeftBrace();
    while(!Match(RIGHT_BRACE))
        AddChildToVector(node->statements,ParseStatement());

    return node;
}

unique_ptr<ASTNode> Parser::ParseParameterList()
{
    WhoAmI("ParseParameterList");

    unique_ptr<ParameterListAST> node=make_unique<ParameterListAST>();

    AddChildToVector(node->parameters,
                    ParseParameter());
    while(Match(COMMA))
        AddChildToVector(node->parameters,
                        ParseParameter());

    return node;
}

unique_ptr<ASTNode> Parser::ParseParameter()
{
    WhoAmI("ParseParameter");

    unique_ptr<ParameterAST> node=make_unique<ParameterAST>();

    if(Match(T_INT)||Match(T_FLOAT))
        node->parameter_type=PrevToken();
    else ParseError("Parameter type loss",CurrentLocation());
    
    if(Match(IDENTIFIER))
        node->parameter_name=PrevToken();
    else ParseError("Parameter name loss",CurrentLocation());

    node->is_ptr=false;
    if(Match(LEFT_SQUARE))
    {
        MatchRightSquare();
        node->is_ptr=true;
        while(Match(LEFT_SQUARE))
        {
            AddChildToVector(node->expressions,
                            ParseExpression());
            MatchRightSquare();
        }
    }

    return node;
}



unique_ptr<ASTNode> Parser::ParseCompoundStatement()
{
    WhoAmI("ParseCompoundStatement");
    
    unique_ptr<CompoundStatementAST> node=make_unique<CompoundStatementAST>();

    MatchLeftBrace();
    while(!Match(RIGHT_BRACE))
        AddChildToVector(node->statements,ParseStatement());

    return node;
}   

unique_ptr<ASTNode> Parser::ParseStatement()
{
    WhoAmI("ParseStatement");

    unique_ptr<StatementAST> node=make_unique<StatementAST>();

    if(Peek(LEFT_BRACE))
        node->x_statement=ParseCompoundStatement();
    else if(Peek(CONST)||Peek(T_INT)||Peek(T_FLOAT))
        node->x_statement=ParseVariableDeclaration();
    else if(Peek(IF))
        node->x_statement=ParseIfStatement();
    else if(Peek(WHILE))
        node->x_statement=ParseWhileStatement();
    else if(Peek(BREAK))
        node->x_statement=ParseBreakStatement();
    else if(Peek(CONTINUE))
        node->x_statement=ParseContinueStatement();
    else if(Peek(RETURN))
        node->x_statement=ParseReturnStatement();
    else 
        node->x_statement=ParseAssignStatement();

    return node;
}

unique_ptr<ASTNode> Parser::ParseAssignStatement()
{
    WhoAmI("ParseAssignStatement");
    
    unique_ptr<AssignStatementAST> node=make_unique<AssignStatementAST>();

    int old_current=current;

    if(Match(IDENTIFIER))
        node->variable_name=PrevToken();
    else 
    {
        //backtrack
        current=old_current;
        return ParseExpressionStatement();
    }

    while(Match(LEFT_SQUARE))
    {
        AddChildToVector(node->expressions,
                        ParseExpression());
        
        if(!Match(RIGHT_SQUARE))
        {
            //backtrack
            current=old_current;
            return ParseExpressionStatement();
        }
    }

    if(!Match(ASSIGN))
    {
        //backtrack
        current=old_current;
        return ParseExpressionStatement();
    }
    
    node->expression=ParseExpression();
    MatchSemicolon();

    return node;
}

unique_ptr<ASTNode> Parser::ParseExpressionStatement()
{
    WhoAmI("ParseExpressionStatement");

    unique_ptr<ExpressionStatementAST> node=make_unique<ExpressionStatementAST>();

    if(!Peek(SEMICOLON))
        node->expression=ParseExpression();
    MatchSemicolon();

    return node;
}

unique_ptr<ASTNode> Parser::ParseIfStatement()
{
    WhoAmI("ParseIfStatement");

    unique_ptr<IfStatementAST> node=make_unique<IfStatementAST>();

    if(!Match(IF)) ParseError("Keyword 'if' loss",CurrentLocation());
    
    MatchLeftParen();
    node->condition_expression=ParseConditionExpression();
    MatchRightParen();
    node->true_statement=ParseStatement();

    if(Match(ELSE))
        node->false_statement=ParseStatement();
    
    return node;
}

unique_ptr<ASTNode> Parser::ParseWhileStatement()
{
    WhoAmI("ParseWhileStatement");

    unique_ptr<WhileStatementAST> node=make_unique<WhileStatementAST>();

    if(!Match(WHILE)) ParseError("Keyword 'while' loss",CurrentLocation());

    MatchLeftParen();
    node->condition_expression=ParseConditionExpression();
    MatchRightParen();

    node->statement=ParseStatement();

    return node;
}

unique_ptr<ASTNode> Parser::ParseBreakStatement()
{
    WhoAmI("ParseBreakStatement");

    unique_ptr<BreakStatementAST> node=make_unique<BreakStatementAST>();

    if(Match(BREAK))
        node->break_t=PrevToken();
    else ParseError("Keyword 'break' loss",CurrentLocation());
    MatchSemicolon();

    return node;
}

unique_ptr<ASTNode> Parser::ParseContinueStatement()
{
    WhoAmI("ParseContinueStatement");

    unique_ptr<ContinueStatementAST> node=make_unique<ContinueStatementAST>();

    if(Match(CONTINUE))
        node->continue_t=PrevToken();
    else ParseError("Keyword 'continue' loss",CurrentLocation());
    MatchSemicolon();
    
    return node;
}

unique_ptr<ASTNode> Parser::ParseReturnStatement()
{
    WhoAmI("ParseReturnStatement");

    unique_ptr<ReturnStatementAST> node=make_unique<ReturnStatementAST>();

    if(Match(RETURN))
        node->return_t=PrevToken();
    else ParseError("Keyword 'return' loss",CurrentLocation());
    
    if(!Peek(SEMICOLON))
        node->expression=ParseExpression();
    MatchSemicolon();

    return node;
}



unique_ptr<ASTNode> Parser::ParseConditionExpression()
{
    WhoAmI("ParseConditionExpression");
    
    unique_ptr<ConditionExpressionAST> node=make_unique<ConditionExpressionAST>();

    node->logicor_expression=ParseLogicOrExpression();

    return node;
}

unique_ptr<ASTNode> Parser::ParseExpression()
{
    WhoAmI("ParseExpression");

    unique_ptr<ExpressionAST> node=make_unique<ExpressionAST>();

    node->addsub_expression=ParseAddSubExpression();

    return node;
}

unique_ptr<ASTNode> Parser::ParseLogicOrExpression()
{
    WhoAmI("ParseLogicOrExpression");

    unique_ptr<LogicOrExpressionAST> node=make_unique<LogicOrExpressionAST>();

    node->logicand_expression=ParseLogicAndExpression();
    while(Match(T_OR))
    {
        node->infix_operators.push_back(PrevToken());
        AddChildToVector(node->logicand_expressions,
                        ParseLogicAndExpression());
    }

    return node;
}

unique_ptr<ASTNode> Parser::ParseLogicAndExpression()
{
    WhoAmI("ParseLogicAndExpression");

    unique_ptr<LogicAndExpressionAST> node=make_unique<LogicAndExpressionAST>();

    node->equal_expression=ParseEqualExpression();
    while(Match(T_AND))
    {
        node->infix_operators.push_back(PrevToken());
        AddChildToVector(node->equal_expressions,
                        ParseEqualExpression());
    }

    return node;
}

unique_ptr<ASTNode> Parser::ParseEqualExpression()
{
    WhoAmI("ParseEqualExpression");

    unique_ptr<EqualExpressionAST> node=make_unique<EqualExpressionAST>();

    node->relation_expression=ParseRelationExpression();
    while(Match(EQUAL)||Match(NOT_EQUAL))
    {
        node->infix_operators.push_back(PrevToken());
        AddChildToVector(node->relation_expressions,
                        ParseRelationExpression());
    }

    return node;
}

unique_ptr<ASTNode> Parser::ParseRelationExpression()
{
    WhoAmI("ParseRelationExpression");

    unique_ptr<RelationExpressionAST> node=make_unique<RelationExpressionAST>();

    node->addsub_expression=ParseAddSubExpression();
    while(Match(LESS)||Match(LESS_EQUAL)||Match(GREATER)||Match(GREATER_EQUAL))
    {
        node->infix_operators.push_back(PrevToken());
        AddChildToVector(node->addsub_expressions,
                        ParseAddSubExpression());
    }

    return node;
}

unique_ptr<ASTNode> Parser::ParseAddSubExpression()
{
    WhoAmI("ParseAddSubExpression");

    unique_ptr<AddSubExpressionAST> node=make_unique<AddSubExpressionAST>();

    node->muldiv_expression=ParseMulDivExpression();
    while(Match(PLUS)||Match(MINUS))
    {
        node->infix_operators.push_back(PrevToken());
        AddChildToVector(node->muldiv_expressions,
                        ParseMulDivExpression());
    }

    return node;
}

unique_ptr<ASTNode> Parser::ParseMulDivExpression()
{
    WhoAmI("ParseMulDivExpression");

    unique_ptr<MulDivExpressionAST> node=make_unique<MulDivExpressionAST>();

    node->unary_expression=ParseUnaryExpression();
    while(Match(STAR)||Match(SLASH)||Match(PERCENT))
    {
        node->infix_operators.push_back(PrevToken());
        AddChildToVector(node->unary_expressions,
                        ParseUnaryExpression());
    }

    return node;
}

unique_ptr<ASTNode> Parser::ParseUnaryExpression()
{
    WhoAmI("ParseUnaryExpression");

    unique_ptr<UnaryExpressionAST> node=make_unique<UnaryExpressionAST>();

    while(Match(PLUS)||Match(MINUS)||Match(T_NOT))
        node->prefix_operators.push_back(PrevToken());

    if(Peek(IDENTIFIER)&&Peek(LEFT_PAREN,2))
        node->functioncall_expression=ParseFunctionCallExpression();
    else
        node->primary_expression=ParsePrimaryExpression();
    
    return node;
}

unique_ptr<ASTNode> Parser::ParsePrimaryExpression()
{
    WhoAmI("ParsePrimaryExpression");

    unique_ptr<PrimaryExpressionAST> node=make_unique<PrimaryExpressionAST>();

    if(Match(LEFT_PAREN))
    {
        node->expression=ParseExpression();
        MatchRightParen();
    }
    else if(Match(IDENTIFIER))
    {
        node->variable_name=PrevToken();
        while(Match(LEFT_SQUARE))
        {
            AddChildToVector(node->expressions,
                            ParseExpression());
            MatchRightSquare();
        }
    }
    else if(Match(CONSTANT_INT)||Match(CONSTANT_FLOAT))
        node->constant=PrevToken();
    else ParseError("Primary character loss",CurrentLocation());

    return node;
}

unique_ptr<ASTNode> Parser::ParseFunctionCallExpression()
{
    WhoAmI("ParseFunctionCallExpression");

    unique_ptr<FunctionCallExpressionAST> node=make_unique<FunctionCallExpressionAST>();

    if(Match(IDENTIFIER))
        node->function_name=PrevToken();
    else ParseError("Called function loss",CurrentLocation());

    MatchLeftParen();
    if(!Peek(RIGHT_PAREN))
    {
        AddChildToVector(node->expressions,
                        ParseExpression());
        while(Match(COMMA))
            AddChildToVector(node->expressions,
                            ParseExpression());
    }
    MatchRightParen();

    return node;
}



//Tool
bool Parser::IsAtEnd()
{
    return tokens[current].token_type==CODE_EOF;
}

bool Parser::Match(TokenType expected)
{
    if(!IsAtEnd()&&expected==tokens[current].token_type)
    {
        current++;
        return true;
    }
    return false;
}

void Parser::MatchSemicolon()
{
    if(!Match(SEMICOLON))
        ParseError("Semicolon ';' loss",CurrentLocation());
}

void Parser::MatchLeftParen()
{
    if(!Match(LEFT_PAREN))
        ParseError("Left paren '(' loss",CurrentLocation());
}

void Parser::MatchRightParen()
{
    if(!Match(RIGHT_PAREN))
        ParseError("Right paren ')' loss",CurrentLocation());
}

void Parser::MatchLeftSquare()
{
    if(!Match(LEFT_SQUARE))
        ParseError("Left square '[' loss",CurrentLocation());
}

void Parser::MatchRightSquare()
{
    if(!Match(RIGHT_SQUARE))
        ParseError("Right square ']' loss",CurrentLocation());
}

void Parser::MatchLeftBrace()
{
    if(!Match(LEFT_BRACE))
        ParseError("Left brace '{' loss",CurrentLocation());
}

void Parser::MatchRightBrace()
{
    if(!Match(RIGHT_BRACE))
        ParseError("Right brace '}' loss",CurrentLocation());
}

bool Parser::Peek(TokenType expected)
{
    return Peek(expected,1);
}

bool Parser::Peek(TokenType expected,int n)
{
    int index=current+n-1;

    if(index<tokens.size()-1
        && expected==tokens[index].token_type)
        return true;
    return false;
}

void Parser::AddChildToVector(vector<unique_ptr<ASTNode>>& vec,unique_ptr<ASTNode> child)
{
    vec.push_back(std::move(child));
}

Token Parser::PrevToken()
{
    return tokens[current-1];
}

Cursor Parser::CurrentLocation()
{
    return tokens[current].location;
}
