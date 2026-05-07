#include "../frontend/AST.h"
#include "../utility/Diagnostor.h"
#include "../frontend/MIREmitter.h"
#include "../midend/Variable.h"
#include "../utility/Product.h"

ValueConstant ExpressionAST::Evaluate()
{
    return addsub_expression->Evaluate();
}

ValueConstant AddSubExpressionAST::Evaluate()
{
    ValueConstant result_constant=muldiv_expression->Evaluate();

    for(int i=0;i<infix_operators.size();i++)
    {
        ValueConstant temp=muldiv_expressions[i]->Evaluate();
        switch(infix_operators[i].token_type)
        {
            case PLUS:{
                if(result_constant.data_type==I32&&temp.data_type==I32)
                    result_constant.constant_value.i32+=temp.constant_value.i32;
                else if(result_constant.data_type==F32&&temp.data_type==F32)
                    result_constant.constant_value.f32+=temp.constant_value.f32;
                else if(result_constant.data_type==F32&&temp.data_type==I32)
                    result_constant.constant_value.f32+=(float)temp.constant_value.i32;
                else if(result_constant.data_type==I32&&temp.data_type==F32){
                    result_constant.Convert(F32);
                    result_constant.constant_value.f32+=temp.constant_value.f32;
                }
                break;
            }

            case MINUS:{
                if(result_constant.data_type==I32&&temp.data_type==I32)
                    result_constant.constant_value.i32-=temp.constant_value.i32;
                else if(result_constant.data_type==F32&&temp.data_type==F32)
                    result_constant.constant_value.f32-=temp.constant_value.f32;
                else if(result_constant.data_type==F32&&temp.data_type==I32)
                    result_constant.constant_value.f32-=(float)temp.constant_value.i32;
                else if(result_constant.data_type==I32&&temp.data_type==F32){
                    result_constant.Convert(F32);
                    result_constant.constant_value.f32-=temp.constant_value.f32;
                }
                break;
            }
        }
    }

    return result_constant;
}

ValueConstant MulDivExpressionAST::Evaluate()
{
    ValueConstant result_constant=unary_expression->Evaluate();

    for(int i=0;i<infix_operators.size();i++)
    {
        ValueConstant temp=unary_expressions[i]->Evaluate();
        switch(infix_operators[i].token_type)
        {
            case STAR:{
                if(result_constant.data_type==I32&&temp.data_type==I32)
                    result_constant.constant_value.i32*=temp.constant_value.i32;
                else if(result_constant.data_type==F32&&temp.data_type==F32)
                    result_constant.constant_value.f32*=temp.constant_value.f32;
                else if(result_constant.data_type==F32&&temp.data_type==I32)
                    result_constant.constant_value.f32*=(float)temp.constant_value.i32;
                else if(result_constant.data_type==I32&&temp.data_type==F32){
                    result_constant.Convert(F32);
                    result_constant.constant_value.f32*=temp.constant_value.f32;
                }
                break;
            }
            
            case SLASH:{
                //Div 0
                switch(temp.data_type)
                {
                    case I32:
                        if(temp.constant_value.i32==0)
                            InitialError("Can't div zero",infix_operators[i].location);
                        break;
                    case F32:
                        if(temp.constant_value.f32==0.0)
                            InitialError("Can't div zero",infix_operators[i].location);
                        break;
                    default:
                        CompilerError("The data type can'be evaluate in MulDivExpressionAST::Evaluate()");
                }

                if(result_constant.data_type==I32&&temp.data_type==I32)
                    result_constant.constant_value.i32/=temp.constant_value.i32;
                else if(result_constant.data_type==F32&&temp.data_type==F32)
                    result_constant.constant_value.f32/=temp.constant_value.f32;
                else if(result_constant.data_type==F32&&temp.data_type==I32)
                    result_constant.constant_value.f32/=(float)temp.constant_value.i32;
                else if(result_constant.data_type==I32&&temp.data_type==F32){
                    result_constant.Convert(F32);
                    result_constant.constant_value.f32/=temp.constant_value.f32;
                }
                break;
            }
            case PERCENT:{
                if(result_constant.data_type==I32&&temp.data_type==I32){
                    //@Mod 0
                    if(temp.constant_value.i32==0)
                        InitialError("Can't mod zero",infix_operators[i].location);
                    
                    result_constant.constant_value.i32%=temp.constant_value.i32;
                }
                else TypeError("Mod is only valid on integers",infix_operators[i].location);
                break;
            }
        }
    }

    return result_constant;
}

ValueConstant UnaryExpressionAST::Evaluate()
{
    //@Optimize: Reduce unary op number
    int count_neg=0,count_not=0;
    for(Token& prefix_operator:prefix_operators)
    {
        switch(prefix_operator.token_type)
        {
            case MINUS:count_neg++;break;
            case T_NOT:count_not++;break;
        }
    }
    count_neg%=2;
    count_not%=2;

    //Primary is default
    ValueConstant result_constant=primary_expression->Evaluate();

    if(count_neg)
    {
        switch(result_constant.data_type)
        {
            case I32:
                result_constant.constant_value.i32=-result_constant.constant_value.i32;
                break;
            case F32:
                result_constant.constant_value.f32=-result_constant.constant_value.f32;
                break;
            default: CompilerError("The data type can'be evaluate in UnaryExpressionAST::Evaluate()");
        }
    }

    if(count_not)
    {
        switch(result_constant.data_type)
        {
            case I32:
                result_constant.constant_value.i32=!result_constant.constant_value.i32;
                break;
            case F32:
                result_constant.data_type=I32;
                result_constant.constant_value.i32=!(int)result_constant.constant_value.f32;
                break;
            default: CompilerError("The data type can'be evaluate in UnaryExpressionAST::Evaluate()");
        }
    }

    return result_constant;
}

ValueConstant PrimaryExpressionAST::Evaluate()
{
    ValueConstant result_constant;

    //(<expression>)
    if(expression) result_constant=expression->Evaluate();
    
    //IDENTIFIER 
    else if(variable_name.is_valid)
    {
        //Visit used variable
        Variable& variable=mir_emitter.vartable.Visit(
            variable_name.lexeme,variable_name.location
        );

        if(variable.is_const||variable.store_type==GLOBAL)
        {
            // var 
            if(variable.data_type==I32||variable.data_type==F32)
            {
                result_constant=variable.initial[0];
            }

            // var[][] default think it is only array's item, don't use subarray's pointer...
            else if(variable.data_type==I32_PTR||variable.data_type==F32_PTR)
            {
                //Calculate index
                int index = 0;
                int weight = 1;
                for (int i = variable.dimensions.size()-1;i>=0;i--) 
                {
                    index+=expressions[i]->Evaluate().constant_value.i32*weight;
                    weight*=variable.dimensions[i];
                }

                result_constant=variable.initial[index];
            }
        }
        else InitialError("Can't initialize by non-const variable ",variable_name.location);
    }

    //CONSTANT
    else if(constant.is_valid)
    {
        switch(constant.token_type)
        {
            case CONSTANT_INT:  result_constant=ValueConstant(stoi(constant.lexeme));break;
            case CONSTANT_FLOAT:result_constant=ValueConstant(stof(constant.lexeme));break;
        }
    }
    
    return result_constant;
}

