#include "../frontend/MIREmitter.h"
#include "../utility/Product.h"
#include "../utility/Diagnostor.h"
#include "../frontend/TypeChecker.h"

MIREmitter mir_emitter;

void MIREmitter::InitializeLibFunction()
{
    MIRFunction& getint=mir.NewLibFunction(I32,"getint");
    functable.Add(getint);

    MIRFunction& getch=mir.NewLibFunction(I32,"getch");
    functable.Add(getch);

    MIRFunction& getfloat=mir.NewLibFunction(F32,"getfloat");
    functable.Add(getfloat);

    MIRFunction& getarray=mir.NewLibFunction(I32,"getarray");
    getarray.NewParameter(I32_PTR,"",vector<int>{0});
    functable.Add(getarray);

    MIRFunction& getfarray=mir.NewLibFunction(I32,"getfarray");
    getfarray.NewParameter(F32_PTR,"",vector<int>{0});
    functable.Add(getfarray);



    MIRFunction& putint=mir.NewLibFunction(VOID,"putint");
    putint.NewParameter(I32,"",vector<int>());
    functable.Add(putint);

    MIRFunction& putch=mir.NewLibFunction(VOID,"putch");
    putch.NewParameter(I32,"",vector<int>());
    functable.Add(putch);

    MIRFunction& putfloat=mir.NewLibFunction(VOID,"putfloat");
    putfloat.NewParameter(F32,"",vector<int>());
    functable.Add(putfloat);

    MIRFunction& putarray=mir.NewLibFunction(VOID,"putarray");
    putarray.NewParameter(I32,"",vector<int>());
    putarray.NewParameter(I32_PTR,"",vector<int>{0});
    functable.Add(putarray);

    MIRFunction& putfarray=mir.NewLibFunction(VOID,"putfarray");
    putfarray.NewParameter(I32,"",vector<int>());
    putfarray.NewParameter(F32_PTR,"",vector<int>{0});
    functable.Add(putfarray);

    //@Todo:putf

    MIRFunction& starttime=mir.NewLibFunction(VOID,"starttime");
    functable.Add(starttime);

    MIRFunction& stoptime=mir.NewLibFunction(VOID,"stoptime");
    functable.Add(stoptime);

}

void MIREmitter::SetFunction(string function_name)
{
    in_function_name=function_name;
}

MIRFunction& MIREmitter::InFunction()
{
    return mir.GetFunction(in_function_name);
}

void MIREmitter::SetBasicBlock(int block_number)
{
    in_block_number=block_number;
}

MIRBasicBlock& MIREmitter::InBasicBlock()
{
    return InFunction().GetBasicBlock(in_block_number);
}



void MIREmitter::EmitMIR()
{
    InitializeLibFunction();

    ast->EmitMIR();
}

void EmitNode(unique_ptr<ASTNode>& node)
{
    node->EmitMIR();
}

void EmitChild(vector<unique_ptr<ASTNode>>& vec)
{
    for(unique_ptr<ASTNode>& ast_ptr:vec)
        ast_ptr->EmitMIR();
}



void TranslationUnitAST::EmitMIR()
{
    mir_emitter.vartable.EnterScope(); //{ new scope
    
    //Emit global declarations
    mir_emitter.def_global=true;
    EmitChild(variable_declarations);
    mir_emitter.def_global=false;

    //Emit function definitions
    EmitChild(function_definitions);

    mir_emitter.vartable.LeaveScope(); // leave scope }
}



void VariableDeclarationAST::EmitMIR()
{
    if(is_const) mir_emitter.def_const=true;

    if(variable_type.token_type==T_INT) 
        mir_emitter.def_data_type=I32;
    else if(variable_type.token_type==T_FLOAT) 
        mir_emitter.def_data_type=F32;

    EmitChild(variable_definitions);

    mir_emitter.def_const=false;
}

void VariableDefinitionAST::EmitMIR()
{
    mir_emitter.def_vartoken=variable_name;

    //Calculate array dimension
    vector<int> dimensions;
    for(unique_ptr<ASTNode>& expression_ptr:expressions)
    {
        ValueConstant dimension=expression_ptr->Evaluate();
        if(dimension.data_type==I32)
            dimensions.push_back(dimension.constant_value.i32);
        else InitialError("Array's dimension must be integer",variable_name.location);
    }

    //Modify data type if it is an array
    if(!dimensions.empty())
    {
        if(mir_emitter.def_data_type==I32)
            mir_emitter.def_data_type=I32_PTR;
        else if(mir_emitter.def_data_type==F32)
            mir_emitter.def_data_type=F32_PTR;
    }

    //Add new variable
    //Global variable
    if(mir_emitter.def_global)
    {
        Variable& global_variable=mir.NewGlobalVariable(
            mir_emitter.def_const,mir_emitter.def_data_type,variable_name.lexeme,dimensions
        );//Add to vartable
        mir_emitter.vartable.Add(global_variable,variable_name.location);
    } 
    //Local variable
    else
    {
        Variable& local_variable=mir_emitter.InFunction().NewLocalVariable(
            mir_emitter.def_const,mir_emitter.def_data_type,variable_name.lexeme,dimensions
        );//Add to vartable
        mir_emitter.vartable.Add(local_variable,variable_name.location);
    }

    //Initialize
    if(variable_initvalue) EmitNode(variable_initvalue);
    else if(mir_emitter.def_const) 
        InitialError("Const variable must be initialize",variable_name.location);

    //Recover
    if(mir_emitter.def_data_type==I32_PTR)
        mir_emitter.def_data_type=I32;
    else if(mir_emitter.def_data_type==F32_PTR)
        mir_emitter.def_data_type=F32;
}

void VariableInitValueAST::EmitMIR()
{
    Variable& def_var=mir_emitter.vartable.Visit(
        mir_emitter.def_vartoken.lexeme,mir_emitter.def_vartoken.location
    );
    
    //Scalar
    if(def_var.data_type==I32||def_var.data_type==F32)
    {
        if(!variable_initvalues.empty())
            InitialError("Can't initialize scalar by list",mir_emitter.def_vartoken.location);

        //Global (const) scalar
        if(mir_emitter.def_global)
        {
            ValueConstant value_initial=expression->Evaluate();
            
            value_initial.Convert(def_var.data_type);
            def_var.initial.push_back(value_initial);
        }
                
        //Local scalar
        else
        {
            //Const
            if(mir_emitter.def_const)
            {
                ValueConstant value_initial=expression->Evaluate();

                value_initial.Convert(def_var.data_type);
                def_var.initial.push_back(value_initial);
            }

            //Variable
            else 
            {
                EmitNode(expression);

                //@Instruction: copy
                shared_ptr<Copy> copy=make_shared<Copy>();
                copy->value=mir_emitter.result_expression;
                copy->variable=make_shared<ValueVariable>(def_var.store_type,def_var.data_type,def_var.number);
                
                CheckCopy(copy);
                mir_emitter.InBasicBlock().AddInsturction(copy);
            }
        }
    }

    //Array
    else if(def_var.data_type==I32_PTR||def_var.data_type==F32_PTR)
    {
        if(expression)
            InitialError("Can't initialize array by scalar",mir_emitter.def_vartoken.location);
        
        int index=0;

        //Global (const) array[][]
        if(mir_emitter.def_global){
            def_var.Initialize();
            FillArrayConst(variable_initvalues,def_var,def_var.dimensions,index);
        }

        //Local array[][]
        else
        {
            //Const
            if(mir_emitter.def_const)
            {
                def_var.Initialize();
                FillArrayConst(variable_initvalues,def_var,def_var.dimensions,index);
            }

            //Variable
            else FillArrayVariable(variable_initvalues,def_var,def_var.dimensions,index);
        }
    }
}

void VariableInitValueAST::FillArrayConst(vector<unique_ptr<ASTNode>>& init_values,Variable& def_var,
                                          vector<int>& dimensions,int& index)
{
    DataType elem_data_type;
    if(def_var.data_type==I32_PTR) elem_data_type=I32;
    else if(def_var.data_type==F32_PTR) elem_data_type=F32;

    int total_size=1;
    for(int& dimension:dimensions)
        total_size*=dimension;

    int start_index = index;

    bool first_is_expression=false;
    for(unique_ptr<ASTNode>& ast_ptr:init_values)
    {
        VariableInitValueAST* init_value_ptr=dynamic_cast<VariableInitValueAST*>(ast_ptr.get());
        
        //Scalar
        if(init_value_ptr->expression)
        {
            ValueConstant number=init_value_ptr->expression->Evaluate();

            number.Convert(elem_data_type);
            def_var.initial[index++]=number;

            first_is_expression=true;
        }

        //Subarray
        else
        {
            //That's very tricky,C programming language is rubbish!
            int i=0;    //{{1},{2}}, sub array in begin
            if(first_is_expression)
            {
                int count=1;
                i=dimensions.size();
                while(count<=index)count*=dimensions[--i];
            }
            i++;
            vector<int> sub_dimension;
            while(i<dimensions.size()) sub_dimension.push_back(dimensions[i++]);

            //Recursive fill
            FillArrayConst(init_value_ptr->variable_initvalues,def_var,sub_dimension,index);
        }
    }

    index=start_index+total_size;
}

void VariableInitValueAST::FillArrayVariable(vector<unique_ptr<ASTNode>>& init_values,Variable& def_var,
                                             vector<int>& dimensions,int& index)
{
    DataType elem_data_type;
    if(def_var.data_type==I32_PTR) elem_data_type=I32;
    else if(def_var.data_type==F32_PTR) elem_data_type=F32;

    int total_size=1;
    for(int& dimension:dimensions)
        total_size*=dimension;

    int start_index = index;

    bool first_is_expression=false;
    for(unique_ptr<ASTNode>& ast_ptr:init_values)
    {
        VariableInitValueAST* init_value_ptr=dynamic_cast<VariableInitValueAST*>(ast_ptr.get());
        
        //Scalar
        if(init_value_ptr->expression)
        {
            EmitNode(init_value_ptr->expression);

            //@Instruction: copy
            shared_ptr<Copy> copy=make_shared<Copy>();
            copy->value=mir_emitter.result_expression;
            shared_ptr<ValueConstant> value_index=make_shared<ValueConstant>(index++);

            copy->variable=make_shared<ValueVariable>(def_var.store_type,elem_data_type,def_var.number,value_index);
            
            CheckCopy(copy);
            mir_emitter.InBasicBlock().AddInsturction(copy);

            first_is_expression=true;
        }

        //Subarray
        else
        {
            //That's very tricky,C programming language is rubbish!
            int i=0;    //{{1},{2}}, sub array in begin
            if(first_is_expression)
            {
                int count=1;
                i=dimensions.size();
                while(count<=index)count*=dimensions[--i];
            }
            i++;
            vector<int> sub_dimension;
            while(i<dimensions.size())sub_dimension.push_back(dimensions[i++]);

            //Recursive fill
            FillArrayVariable(init_value_ptr->variable_initvalues,def_var,sub_dimension,index);
        }
    }

    //Fill remaining elements with 0
    while(index<start_index+total_size)
    {
        //@Instruction: copy
        shared_ptr<Copy> copy=make_shared<Copy>();
        copy->value=make_shared<ValueConstant>(elem_data_type);
        shared_ptr<ValueConstant> value_index = make_shared<ValueConstant>(index++);

        copy->variable=make_shared<ValueVariable>(def_var.store_type,elem_data_type,def_var.number,value_index);
            
        CheckCopy(copy);
        mir_emitter.InBasicBlock().AddInsturction(copy);
    }
}



void FunctionDefinitionAST::EmitMIR()
{   
    //Create new MIR function 
    MIRFunction& function=mir.NewFunction(return_type.ToDataType(),function_name.lexeme);

    //Add new function to functable
    mir_emitter.functable.Add(function,function_name.location);

    mir_emitter.SetFunction(function_name.lexeme);
    mir_emitter.SetBasicBlock(mir_emitter.InFunction().entry_block_number);
    
    mir_emitter.vartable.EnterScope(); //{ new scope

    //Add parameters
    if(parameter_list) EmitNode(parameter_list);

    //Emit function statements
    EmitChild(statements);
    
    //Add ret instruction if function don't have return statement
    //@Instruction:ret
    if(mir_emitter.InBasicBlock().succeeds.find(mir_emitter.InFunction().exit_block_number)
        ==mir_emitter.InBasicBlock().succeeds.end())
    {
        shared_ptr<Return> ret=make_shared<Return>();
        mir_emitter.InBasicBlock().AddInsturction(ret);

        //Build cfg edge
        mir_emitter.InBasicBlock().AddSucceed(mir_emitter.InFunction().exit_block_number);
        mir_emitter.InFunction().GetExitBasicBlock().AddPrecursor(mir_emitter.InBasicBlock().number);
    }

    mir_emitter.vartable.LeaveScope(); // leave scope }
}

void ParameterListAST::EmitMIR()
{
    EmitChild(parameters);
}

void ParameterAST::EmitMIR()
{   
    DataType parameter_data_type=parameter_type.ToDataType();
        
    //Calculate array dimension
    vector<int> dimensions;
    if(is_ptr) dimensions.push_back(0);
        
    for(unique_ptr<ASTNode>& expression_ptr:expressions)
    {
        ValueConstant dimension=expression_ptr->Evaluate();

        if(dimension.data_type==I32)
            dimensions.push_back(dimension.constant_value.i32);
        else InitialError("Array pointer parameter 's dimension must be integer",parameter_name.location);
    }      

    //Modify data type if it is an array
    if(!dimensions.empty())
    {
        if(parameter_data_type==I32) parameter_data_type=I32_PTR;
        else if(parameter_data_type==F32) parameter_data_type=F32_PTR;
    }

    //Add parameter
    Variable& parameter=mir_emitter.InFunction().NewParameter(
        parameter_data_type,parameter_name.lexeme,dimensions
    );
    mir_emitter.vartable.Add(parameter,parameter_name.location);
}



void CompoundStatementAST::EmitMIR()
{
    mir_emitter.vartable.EnterScope(); //{ new scope

    EmitChild(statements);

    mir_emitter.vartable.LeaveScope(); // leave scope }
}

void StatementAST::EmitMIR()
{
    EmitNode(x_statement);
}

void AssignStatementAST::EmitMIR()
{
    //Visit assigned variable
    Variable& assign_var=mir_emitter.vartable.Visit(
        variable_name.lexeme,variable_name.location
    );
    
    if(assign_var.is_const)
        AssignError("Const variable can't be assigned",variable_name.location);
    
    //Emit value
    EmitNode(expression);

    //@Instruction: copy
    shared_ptr<Copy> assign=make_shared<Copy>();
    assign->value=mir_emitter.result_expression;

    // var=v;
    if(assign_var.data_type==I32||assign_var.data_type==F32)
        assign->variable=make_shared<ValueVariable>(
            assign_var.store_type,assign_var.data_type,assign_var.number
        );
    
    // var[][]=v; default think it is only array's item, don't use subarray's pointer...
    else if(assign_var.data_type==I32_PTR||assign_var.data_type==F32_PTR)
    {
        if(expressions.size()!=assign_var.dimensions.size())
            AssignError("Only array elem can be assigned,pointer is invalid",variable_name.location);

        //Calculate index
        //@Instruction: add mul
        shared_ptr<Value> index=make_shared<ValueConstant>(0);
        int weight_number=1;

        for (int i = assign_var.dimensions.size()-1;i>=0;i--) 
        {
            shared_ptr<Value> weight=make_shared<ValueConstant>(weight_number);

            shared_ptr<Mul> mul=make_shared<Mul>();
            mul->a=weight;
            EmitNode(expressions[i]);
            mul->b=mir_emitter.result_expression;

            Check(mul->b,I32);CheckMul(mul);
            mul->c=mir_emitter.InFunction().NewTempLocalVar(I32);
            mir_emitter.InBasicBlock().AddInsturction(mul);

            shared_ptr<Add> add=make_shared<Add>();
            add->a=index;
            add->b=mul->c;

            CheckAdd(add);
            add->c=mir_emitter.InFunction().NewTempLocalVar(I32);
            mir_emitter.InBasicBlock().AddInsturction(add);

            weight_number*=assign_var.dimensions[i];
            index=add->c;
        }

        //Transform to array elem data type
        DataType elem_data_type;
        if(assign_var.data_type==I32_PTR) elem_data_type=I32;
        else if(assign_var.data_type==F32_PTR) elem_data_type=F32;

        assign->variable=make_shared<ValueVariable>(
            assign_var.store_type,elem_data_type,assign_var.number,index
        );
    }   
    
    CheckCopy(assign);
    mir_emitter.InBasicBlock().AddInsturction(assign);
}

void ExpressionStatementAST::EmitMIR()
{
    if(expression) EmitNode(expression);
}

void IfStatementAST::EmitMIR()
{
    //Emit condition expression
    EmitNode(condition_expression);
    //@Instruction: br
    shared_ptr<Branch> branch=make_shared<Branch>();
    branch->condition=mir_emitter.result_expression;
    Check(branch->condition,I32);

    //Creat true block
    int old_block_number=mir_emitter.InBasicBlock().number;
    int true_block_begin_number=mir_emitter.InFunction().NewBasicBlock();
    mir_emitter.InBasicBlock().AddSucceed(true_block_begin_number);
    branch->true_number=true_block_begin_number;
    
    //Have else
    if(false_statement)
    {
        //Create false block
        int false_block_begin_number=mir_emitter.InFunction().NewBasicBlock();
        mir_emitter.InBasicBlock().AddSucceed(false_block_begin_number);
        branch->false_number=false_block_begin_number;

        //Add branch instruction to old block
        mir_emitter.InBasicBlock().AddInsturction(branch);

        //Build true block
        mir_emitter.SetBasicBlock(true_block_begin_number);
        mir_emitter.InBasicBlock().AddPrecursor(old_block_number);//---
        //Emit true statement
        EmitNode(true_statement);//===
        int true_block_end_number=mir_emitter.InBasicBlock().number;

        //Create next block
        int next_block_number=mir_emitter.InFunction().NewBasicBlock();
        mir_emitter.InBasicBlock().AddSucceed(next_block_number);//---Sequence is important!!!

        //@Instruction:jump
        //True block will jump to next block
        shared_ptr<Jump> jump1=make_shared<Jump>();
        jump1->number=next_block_number;
        mir_emitter.InBasicBlock().AddInsturction(jump1);

        //Build false block
        mir_emitter.SetBasicBlock(false_block_begin_number);
        mir_emitter.InBasicBlock().AddPrecursor(old_block_number);//---
        //Emit false statement
        EmitNode(false_statement);//===
        int false_block_end_number=mir_emitter.InBasicBlock().number;
        mir_emitter.InBasicBlock().AddSucceed(next_block_number);//---Sequence is important!!!

        //@Instruction: jump
        //False block will jump to next block
        shared_ptr<Jump> jump2=make_shared<Jump>();
        jump2->number=next_block_number;
        mir_emitter.InBasicBlock().AddInsturction(jump2);

        //Point to next block
        mir_emitter.SetBasicBlock(next_block_number);
        mir_emitter.InBasicBlock().AddPrecursor(true_block_end_number);
        mir_emitter.InBasicBlock().AddPrecursor(false_block_end_number);
    }
    //Only if
    else
    {
        //Creat next block
        int next_block_number=mir_emitter.InFunction().NewBasicBlock();
        mir_emitter.InBasicBlock().AddSucceed(next_block_number);
        branch->false_number=next_block_number;

        //Add branch instruction to old block
        mir_emitter.InBasicBlock().AddInsturction(branch);

        //Build true block
        mir_emitter.SetBasicBlock(true_block_begin_number);
        mir_emitter.InBasicBlock().AddPrecursor(old_block_number);//---
        
        //Emit true block
        EmitNode(true_statement);//===
        int true_block_end_number=mir_emitter.InBasicBlock().number;
        mir_emitter.InBasicBlock().AddSucceed(next_block_number);//---Sequence is important!!!
        
        //@Instruction: jump
        //True block will jump to next block
        shared_ptr<Jump> jump=make_shared<Jump>();
        jump->number=next_block_number;
        mir_emitter.InBasicBlock().AddInsturction(jump);

        //Point to next block
        mir_emitter.SetBasicBlock(next_block_number);
        mir_emitter.InBasicBlock().AddPrecursor(true_block_end_number);
        mir_emitter.InBasicBlock().AddPrecursor(old_block_number); 
    }
}

void WhileStatementAST::EmitMIR()
{
    //Create condition block
    int old_block_number=mir_emitter.InBasicBlock().number;
    int condition_block_begin_number=mir_emitter.InFunction().NewBasicBlock();
    mir_emitter.InBasicBlock().AddSucceed(condition_block_begin_number);
    
    //@Instruction: jump
    //Old block will jump to codition block
    shared_ptr<Jump> jump1=make_shared<Jump>();
    jump1->number=condition_block_begin_number;
    mir_emitter.InBasicBlock().AddInsturction(jump1);

    //Build condition block
    mir_emitter.SetBasicBlock(condition_block_begin_number);
    mir_emitter.InBasicBlock().AddPrecursor(old_block_number);
    EmitNode(condition_expression);
    int condition_block_end_number=mir_emitter.InBasicBlock().number;

    //@Instruction: branch
    shared_ptr<Branch> branch=make_shared<Branch>();
    branch->condition=mir_emitter.result_expression;
    Check(branch->condition,I32);

    //Create loop block
    int loop_block_begin_number=mir_emitter.InFunction().NewBasicBlock();
    mir_emitter.InBasicBlock().AddSucceed(loop_block_begin_number);
    branch->true_number=loop_block_begin_number;

    //Create next block
    int next_block_number=mir_emitter.InFunction().NewBasicBlock();
    mir_emitter.InBasicBlock().AddSucceed(next_block_number);
    branch->false_number=next_block_number;

    //Add loop information
    mir_emitter.loop_context.push(
        LoopContext(condition_block_begin_number,next_block_number)
    );

    //Add branch instruction to condition block
    mir_emitter.InBasicBlock().AddInsturction(branch);

    //Build loop block
    mir_emitter.SetBasicBlock(loop_block_begin_number);//---
    mir_emitter.InBasicBlock().AddPrecursor(condition_block_end_number);
    EmitNode(statement);//===
    int loop_block_end_number=mir_emitter.InBasicBlock().number;
    mir_emitter.InBasicBlock().AddSucceed(condition_block_begin_number);//---Sequence is important!!!

    //@Instruction: jump
    //Loop block will jump to condition block
    shared_ptr<Jump> jump2=make_shared<Jump>();
    jump2->number=condition_block_begin_number;
    mir_emitter.InBasicBlock().AddInsturction(jump2);

    //Add loop block end to condition block's precursor
    mir_emitter.SetBasicBlock(condition_block_begin_number);
    mir_emitter.InBasicBlock().AddPrecursor(loop_block_end_number);

    //Point to next block
    mir_emitter.SetBasicBlock(next_block_number);
    mir_emitter.InBasicBlock().AddPrecursor(condition_block_end_number);

    //Clear loop information
    mir_emitter.loop_context.pop();
}

void BreakStatementAST::EmitMIR()
{
    int old_block_number=mir_emitter.InBasicBlock().number;

    if(mir_emitter.loop_context.empty())
        Error("Break is not in loop",break_t.location);

    int loop_next_block_number=mir_emitter.loop_context.top().next_block_begin_number;
    mir_emitter.InBasicBlock().AddSucceed(loop_next_block_number);

    //@Instruction: jump
    //Old block will jump to loop's next block
    shared_ptr<Jump> jump=make_shared<Jump>();
    jump->number=loop_next_block_number;
    mir_emitter.InBasicBlock().AddInsturction(jump);

    //Add precursor
    mir_emitter.SetBasicBlock(loop_next_block_number);
    mir_emitter.InBasicBlock().AddPrecursor(old_block_number);

    //Create new block and point to it
    mir_emitter.SetBasicBlock(mir_emitter.InFunction().NewBasicBlock());
}

void ContinueStatementAST::EmitMIR()
{
    int old_block_nume=mir_emitter.InBasicBlock().number;
    
    if(mir_emitter.loop_context.empty())
        Error("Continue is not in loop",continue_t.location);

    int loop_condition_block_number=mir_emitter.loop_context.top().condition_block_begin_number;
    mir_emitter.InBasicBlock().AddSucceed(loop_condition_block_number);

    //@Instruction: jump
    //Old block will jump to loop's condition block
    shared_ptr<Jump> jump=make_shared<Jump>();
    jump->number=loop_condition_block_number;
    mir_emitter.InBasicBlock().AddInsturction(jump);

    //Add precursor
    mir_emitter.SetBasicBlock(loop_condition_block_number);
    mir_emitter.InBasicBlock().AddPrecursor(old_block_nume);

    //Create new block and point to it
    mir_emitter.SetBasicBlock(mir_emitter.InFunction().NewBasicBlock());
}

void ReturnStatementAST::EmitMIR()
{
    //@Instruction: ret
    shared_ptr<Return> ret=make_shared<Return>();

    if(expression)
    {
        if(mir_emitter.InFunction().return_type==VOID)
            TypeError("Void funcntion can't return value",return_t.location);

        EmitNode(expression);
        ret->return_value=mir_emitter.result_expression;
    }
    else
    {
        if(mir_emitter.InFunction().return_type!=VOID)
            TypeError("Non-void function can't return void",return_t.location);
    }

    //Add ret to old block
    mir_emitter.InBasicBlock().AddInsturction(ret);

    //Build cfg edge
    mir_emitter.InBasicBlock().AddSucceed(mir_emitter.InFunction().exit_block_number);
    mir_emitter.InFunction().GetExitBasicBlock().AddPrecursor(mir_emitter.InBasicBlock().number);

    //Create new block and point to it
    mir_emitter.SetBasicBlock(mir_emitter.InFunction().NewBasicBlock());
}



void ConditionExpressionAST::EmitMIR()
{
    EmitNode(logicor_expression);
}

void ExpressionAST::EmitMIR()
{
    EmitNode(addsub_expression);
}

void LogicOrExpressionAST::EmitMIR()
{
    //Emit first expression
    EmitNode(logicand_expression);

    shared_ptr<ValueVariable> final_result=mir_emitter.InFunction().NewTempLocalVar(I32);
    //@Instruction: copy
    shared_ptr<Copy> copy=make_shared<Copy>();
    copy->value=mir_emitter.result_expression;
    copy->variable=final_result;
    mir_emitter.InBasicBlock().AddInsturction(copy);

    if(!infix_operators.empty())
    {
        //Create end block
        int end_block_number=mir_emitter.InFunction().NewBasicBlock();
        mir_emitter.InBasicBlock().AddSucceed(end_block_number);
        //End's previous block numbers
        vector<int> endprev_block_numbers;
        endprev_block_numbers.push_back(mir_emitter.InBasicBlock().number);

        for(int i=0;i<infix_operators.size();i++)
        {   
            //@Instruction: br
            shared_ptr<Branch> branch=make_shared<Branch>();
            branch->condition=mir_emitter.result_expression;//@Bug:lead free twice
            branch->true_number=end_block_number;
            Check(branch->condition,I32); 

            //Create new block
            int previous_block_number=mir_emitter.InBasicBlock().number;
            int new_block_number=mir_emitter.InFunction().NewBasicBlock();

            //Add br
            branch->false_number=new_block_number;
            mir_emitter.InBasicBlock().AddSucceed(new_block_number);
            mir_emitter.InBasicBlock().AddInsturction(branch);
            
            //Build new block
            mir_emitter.SetBasicBlock(new_block_number);
            mir_emitter.InBasicBlock().AddPrecursor(previous_block_number);
            EmitNode(logicand_expressions[i]);
            
            //@Instruction: copy
            shared_ptr<Copy> copy=make_shared<Copy>();
            copy->value=mir_emitter.result_expression;
            copy->variable=final_result;
            mir_emitter.InBasicBlock().AddInsturction(copy);

            //Must after Emit!!!
            endprev_block_numbers.push_back(mir_emitter.InBasicBlock().number);
            mir_emitter.InBasicBlock().AddSucceed(end_block_number);

            if(i==infix_operators.size()-1)
            {
                //@Instruction: jump
                shared_ptr<Jump> jump=make_shared<Jump>();
                jump->number=end_block_number;
                mir_emitter.InBasicBlock().AddInsturction(jump);
            }
        }

        mir_emitter.SetBasicBlock(end_block_number);
        for(int& endprev_block_number:endprev_block_numbers)
            mir_emitter.InBasicBlock().AddPrecursor(endprev_block_number);
    }

    mir_emitter.result_expression = final_result;
    
    // //Emit others
    // for(int i=0;i<infix_operators.size();i++)
    // {
    //     //@Instruction: or
    //     shared_ptr<Or> _or=make_shared<Or>();
    //     _or->a=mir_emitter.result_expression;
    //     EmitNode(logicand_expressions[i]);
    //     _or->b=mir_emitter.result_expression;

    //     CheckOr(_or);
    //     _or->c=mir_emitter.InFunction().NewTempLocalVar(I32);

    //     mir_emitter.result_expression=_or->c;
    //     mir_emitter.InBasicBlock().AddInsturction(_or);
    // }
}

void LogicAndExpressionAST::EmitMIR()
{
    //Emit first expression
    EmitNode(equal_expression);

    shared_ptr<ValueVariable> final_result=mir_emitter.InFunction().NewTempLocalVar(I32);
    //@Instruction: copy
    shared_ptr<Copy> copy=make_shared<Copy>();
    copy->value=mir_emitter.result_expression;
    copy->variable=final_result;
    mir_emitter.InBasicBlock().AddInsturction(copy);

    if(!infix_operators.empty())
    {
        //Create end block
        int end_block_number=mir_emitter.InFunction().NewBasicBlock();
        mir_emitter.InBasicBlock().AddSucceed(end_block_number);
        //End's previous block numbers
        vector<int> endprev_block_numbers;
        endprev_block_numbers.push_back(mir_emitter.InBasicBlock().number);

        for(int i=0;i<infix_operators.size();i++)
        {   
            //@Instruction: br
            shared_ptr<Branch> branch=make_shared<Branch>();
            branch->condition=mir_emitter.result_expression;
            branch->false_number=end_block_number;
            Check(branch->condition,I32); 

            //Create new block
            int previous_block_number=mir_emitter.InBasicBlock().number;
            int new_block_number=mir_emitter.InFunction().NewBasicBlock();

            //Add br
            branch->true_number=new_block_number;
            mir_emitter.InBasicBlock().AddSucceed(new_block_number);
            mir_emitter.InBasicBlock().AddInsturction(branch);

            //Build new block
            mir_emitter.SetBasicBlock(new_block_number);
            mir_emitter.InBasicBlock().AddPrecursor(previous_block_number);
            EmitNode(equal_expressions[i]);
            
            //@Instruction: copy
            shared_ptr<Copy> copy=make_shared<Copy>();
            copy->value=mir_emitter.result_expression;
            copy->variable=final_result;
            mir_emitter.InBasicBlock().AddInsturction(copy);

            //Must after Emit!!!
            endprev_block_numbers.push_back(mir_emitter.InBasicBlock().number);
            mir_emitter.InBasicBlock().AddSucceed(end_block_number);

            if(i==infix_operators.size()-1)
            {
                //@Instruction: jump
                shared_ptr<Jump> jump=make_shared<Jump>();
                jump->number=end_block_number;
                mir_emitter.InBasicBlock().AddInsturction(jump);
            }
        }

        mir_emitter.SetBasicBlock(end_block_number);
        for(int& endprev_block_number:endprev_block_numbers)
            mir_emitter.InBasicBlock().AddPrecursor(endprev_block_number);
    }

    mir_emitter.result_expression = final_result;
    
    // //Emit others
    // for(int i=0;i<infix_operators.size();i++)
    // {
    //     //@Instruction: and
    //     shared_ptr<And> _and=make_shared<And>();
    //     _and->a=mir_emitter.result_expression;
    //     EmitNode(equal_expressions[i]);
    //     _and->b=mir_emitter.result_expression;

    //     CheckAnd(_and);
    //     _and->c=mir_emitter.InFunction().NewTempLocalVar(I32);

    //     mir_emitter.result_expression=_and->c;
    //     mir_emitter.InBasicBlock().AddInsturction(_and);
    // }
}

void EqualExpressionAST::EmitMIR()
{
    //Emit first expression
    EmitNode(relation_expression);

    //Emit others
    for(int i=0;i<infix_operators.size();i++)
    {
        switch(infix_operators[i].token_type)
        {
            case EQUAL:{
                //@Instruction: equ
                shared_ptr<Equal> equal=make_shared<Equal>();
                equal->a=mir_emitter.result_expression;
                EmitNode(relation_expressions[i]);
                equal->b=mir_emitter.result_expression;

                CheckEqual(equal);
                equal->c=mir_emitter.InFunction().NewTempLocalVar(I32);

                mir_emitter.result_expression=equal->c;
                mir_emitter.InBasicBlock().AddInsturction(equal);
                break;
            }
            case NOT_EQUAL:{
                //@Instruction: nequ
                shared_ptr<NotEqual> not_equal=make_shared<NotEqual>();
                not_equal->a=mir_emitter.result_expression;
                EmitNode(relation_expressions[i]);
                not_equal->b=mir_emitter.result_expression;

                CheckNotEqual(not_equal);
                not_equal->c=mir_emitter.InFunction().NewTempLocalVar(I32);

                mir_emitter.result_expression=not_equal->c;
                mir_emitter.InBasicBlock().AddInsturction(not_equal);
                break;
            }
        }
    }
}

void RelationExpressionAST::EmitMIR()
{
    //Emit first expression
    EmitNode(addsub_expression);

    //Emit others
    for(int i=0;i<infix_operators.size();i++)
    {
        switch(infix_operators[i].token_type)
        {
            case LESS:{
                //@Instruction: les
                shared_ptr<Less> less=make_shared<Less>();
                less->a=mir_emitter.result_expression;
                EmitNode(addsub_expressions[i]);
                less->b=mir_emitter.result_expression;

                CheckLess(less);
                less->c=mir_emitter.InFunction().NewTempLocalVar(I32);
                
                mir_emitter.result_expression=less->c;
                mir_emitter.InBasicBlock().AddInsturction(less);
                break;
            }
            case LESS_EQUAL:{
                //@Instruction: lequ
                shared_ptr<LessEqual> less_equal=make_shared<LessEqual>();
                less_equal->a=mir_emitter.result_expression;
                EmitNode(addsub_expressions[i]);
                less_equal->b=mir_emitter.result_expression;

                CheckLessEqual(less_equal);
                less_equal->c=mir_emitter.InFunction().NewTempLocalVar(I32);

                mir_emitter.result_expression=less_equal->c;
                mir_emitter.InBasicBlock().AddInsturction(less_equal);
                break;
            }
            case GREATER:{
                //@Instruction: gre
                shared_ptr<Greater> greater=make_shared<Greater>();
                greater->a=mir_emitter.result_expression;
                EmitNode(addsub_expressions[i]);
                greater->b=mir_emitter.result_expression;

                CheckGreater(greater);
                greater->c=mir_emitter.InFunction().NewTempLocalVar(I32);
                
                mir_emitter.result_expression=greater->c;
                mir_emitter.InBasicBlock().AddInsturction(greater);
                break;
            }
            case GREATER_EQUAL:{
                //@Instruction: gequ
                shared_ptr<GreaterEqual> greater_equal=make_shared<GreaterEqual>();
                greater_equal->a=mir_emitter.result_expression;
                EmitNode(addsub_expressions[i]);
                greater_equal->b=mir_emitter.result_expression;

                CheckGreaterEqual(greater_equal);
                greater_equal->c=mir_emitter.InFunction().NewTempLocalVar(I32);
                
                mir_emitter.result_expression=greater_equal->c;
                mir_emitter.InBasicBlock().AddInsturction(greater_equal);
                break;
            }
        }
    }
}

void AddSubExpressionAST::EmitMIR()
{
    //Emit first expression
    EmitNode(muldiv_expression);

    //Emit others
    for(int i=0;i<infix_operators.size();i++)
    {
        switch(infix_operators[i].token_type)
        {
            case PLUS:{
                //@Instruction: add
                shared_ptr<Add> add=make_shared<Add>();
                add->a=mir_emitter.result_expression;
                EmitNode(muldiv_expressions[i]);
                add->b=mir_emitter.result_expression;

                CheckAdd(add);
                add->c=mir_emitter.InFunction().NewTempLocalVar(add->data_type);

                mir_emitter.result_expression=add->c;
                mir_emitter.InBasicBlock().AddInsturction(add);
                break;
            }
            case MINUS:{
                //@Instruction: sub
                shared_ptr<Sub> sub=make_shared<Sub>();
                sub->a=mir_emitter.result_expression;
                EmitNode(muldiv_expressions[i]);
                sub->b=mir_emitter.result_expression;

                CheckSub(sub);
                sub->c=mir_emitter.InFunction().NewTempLocalVar(sub->data_type);

                mir_emitter.result_expression=sub->c;
                mir_emitter.InBasicBlock().AddInsturction(sub);
                break;
            }
        }
    }
}

void MulDivExpressionAST::EmitMIR()
{
    //Emit first expression
    EmitNode(unary_expression);

    //Emit others
    for(int i=0;i<infix_operators.size();i++)
    {
        switch(infix_operators[i].token_type)
        {
            case STAR:{
                //@Instruction: mul
                shared_ptr<Mul> mul=make_shared<Mul>();
                mul->a=mir_emitter.result_expression;
                EmitNode(unary_expressions[i]);
                mul->b=mir_emitter.result_expression;

                CheckMul(mul);
                mul->c=mir_emitter.InFunction().NewTempLocalVar(mul->data_type);
                
                mir_emitter.result_expression=mul->c;
                mir_emitter.InBasicBlock().AddInsturction(mul);
                break;
            }
            case SLASH:{
                //@Instruction: div
                shared_ptr<Div> div=make_shared<Div>();
                div->a=mir_emitter.result_expression;
                EmitNode(unary_expressions[i]);
                div->b=mir_emitter.result_expression;

                CheckDiv(div);
                div->c=mir_emitter.InFunction().NewTempLocalVar(div->data_type);

                mir_emitter.result_expression=div->c;
                mir_emitter.InBasicBlock().AddInsturction(div);
                break;
            }
            case PERCENT:{
                //@Instruction: mod
                shared_ptr<Mod> mod=make_shared<Mod>();
                mod->a=mir_emitter.result_expression;
                EmitNode(unary_expressions[i]);
                mod->b=mir_emitter.result_expression;

                CheckMod(mod,infix_operators[i].location);
                mod->c=mir_emitter.InFunction().NewTempLocalVar(I32);

                mir_emitter.result_expression=mod->c;
                mir_emitter.InBasicBlock().AddInsturction(mod);
                break;
            }
        }
    }
}

void UnaryExpressionAST::EmitMIR()
{
    //Emit lower expression
    if(primary_expression)
        EmitNode(primary_expression);
    else if(functioncall_expression)
        EmitNode(functioncall_expression);

    //From right to left
    for(int i=prefix_operators.size()-1;i>=0;i--)
    {
        //@Instruction: neg
        if(prefix_operators[i].token_type==MINUS)
        {
            shared_ptr<Negative> neg=make_shared<Negative>();
            neg->data_type=ValueDataType(mir_emitter.result_expression);
            neg->a=mir_emitter.result_expression;
            neg->b=mir_emitter.InFunction().NewTempLocalVar(neg->data_type);

            mir_emitter.result_expression=neg->b;
            mir_emitter.InBasicBlock().AddInsturction(neg);
        }
        //@Instruction: not
        else if(prefix_operators[i].token_type==T_NOT)
        {
            shared_ptr<Not> _not=make_shared<Not>();
            _not->a=mir_emitter.result_expression;
            _not->b=mir_emitter.InFunction().NewTempLocalVar(I32);
        
            CheckNot(_not);
            mir_emitter.result_expression=_not->b;
            mir_emitter.InBasicBlock().AddInsturction(_not);
        }
    }
}

void PrimaryExpressionAST::EmitMIR()
{
    //(<expression>)
    if(expression) EmitNode(expression);

    //IDENTIFIER
    else if(variable_name.is_valid)
    {
        //Visit used variable
        Variable& variable=mir_emitter.vartable.Visit(
            variable_name.lexeme,variable_name.location
        );
        
        // var
        if(variable.data_type==I32||variable.data_type==F32)
        {
            mir_emitter.result_expression=make_shared<ValueVariable>(
                variable.store_type,variable.data_type,variable.number
            );
        }
        
        // var[][]
        else if(variable.data_type==I32_PTR||variable.data_type==F32_PTR)
        {
            //var* pointer
            if(expressions.size()==0)
            {
                mir_emitter.result_expression=make_shared<ValueVariable>(
                    variable.store_type,variable.data_type,variable.number
                );
            }
            //var[]* subarray pointer
            else if(expressions.size()<variable.dimensions.size())
            {
                //Calculate offset
                shared_ptr<Value> index=make_shared<ValueConstant>(0);
                int weight_number=1;

                //Align
                for(int i=variable.dimensions.size()-1;i>expressions.size()-1;i--)
                    weight_number*=variable.dimensions[i];
                
                for(int i=expressions.size()-1;i>=0;i--)
                {
                    shared_ptr<Value> weight=make_shared<ValueConstant>(weight_number);

                    shared_ptr<Mul> mul=make_shared<Mul>();
                    mul->a=weight;
                    EmitNode(expressions[i]);
                    mul->b=mir_emitter.result_expression;

                    Check(mul->b,I32);CheckMul(mul);
                    mul->c=mir_emitter.InFunction().NewTempLocalVar(I32);
                    mir_emitter.InBasicBlock().AddInsturction(mul);

                    shared_ptr<Add> add=make_shared<Add>();
                    add->a=index;
                    add->b=mul->c;

                    CheckAdd(add);
                    add->c=mir_emitter.InFunction().NewTempLocalVar(I32);
                    mir_emitter.InBasicBlock().AddInsturction(add);

                    weight_number*=variable.dimensions[i];
                    index=add->c;
                }

                //Mul elem width
                shared_ptr<Mul> mul=make_shared<Mul>();
                mul->a=index;
                mul->b=make_shared<ValueConstant>(4);//Only have i32 and f32, all 4 bytes
                CheckMul(mul);
                mul->c=mir_emitter.InFunction().NewTempLocalVar(I32);
                mir_emitter.InBasicBlock().AddInsturction(mul);

                //Add offset
                shared_ptr<Value> offset=mul->c;
                shared_ptr<Add> add=make_shared<Add>();
                add->a=make_shared<ValueVariable>(variable.store_type,variable.data_type,variable.number);
                add->b=offset;
                
                CheckAdd(add); //Must do it,and add->a must is variable;It relate pointer and add data type
                add->c=mir_emitter.InFunction().NewTempLocalVar(add->data_type);
                mir_emitter.InBasicBlock().AddInsturction(add);
                mir_emitter.result_expression=add->c;            
            }
            //var[][] array elem
            else if(expressions.size()==variable.dimensions.size())
            {
                //Calculate index
                //@Instruction: add mul
                shared_ptr<Value> index=make_shared<ValueConstant>(0);
                int weight_number=1;

                for (int i = variable.dimensions.size()-1;i>=0;i--) 
                {
                    shared_ptr<Value> weight=make_shared<ValueConstant>(weight_number);

                    shared_ptr<Mul> mul=make_shared<Mul>();
                    mul->a=weight;
                    EmitNode(expressions[i]);
                    mul->b=mir_emitter.result_expression;
                    
                    Check(mul->b,I32);CheckMul(mul);
                    mul->c=mir_emitter.InFunction().NewTempLocalVar(I32);
                    mir_emitter.InBasicBlock().AddInsturction(mul);

                    shared_ptr<Add> add=make_shared<Add>();
                    add->a=index;
                    add->b=mul->c;
                    
                    CheckAdd(add);
                    add->c=mir_emitter.InFunction().NewTempLocalVar(I32);
                    mir_emitter.InBasicBlock().AddInsturction(add);

                    weight_number*=variable.dimensions[i];
                    index=add->c;
                }

                //Transform to array elem data type
                DataType elem_data_type;
                if(variable.data_type==I32_PTR) elem_data_type=I32;
                else if(variable.data_type==F32_PTR) elem_data_type=F32;

                mir_emitter.result_expression=make_shared<ValueVariable>(
                    variable.store_type,elem_data_type,variable.number,index
                );
            }
        }
    }

    //CONSTANT
    else if(constant.is_valid)
    {
        switch(constant.token_type)
        {
            case CONSTANT_INT:{
                int value_i32;
                if(constant.lexeme.size()>2&&constant.lexeme[0]=='0'&&
                    (constant.lexeme[1]=='x'||constant.lexeme[1]=='X'))
                    value_i32=stoi(constant.lexeme,nullptr,16);
                else if(constant.lexeme.size()>1&&constant.lexeme[0]=='0')
                    value_i32=stoi(constant.lexeme,nullptr,8);
                else value_i32=stoi(constant.lexeme,nullptr,10);
                 
                //Load i32
                mir_emitter.result_expression=
                    make_shared<ValueConstant>(value_i32);
                break;
            }
            case CONSTANT_FLOAT:{
                float value_f32=stof(constant.lexeme);
                //Load f32
                mir_emitter.result_expression=
                    make_shared<ValueConstant>(value_f32);
                break;
            }
        }
    }
}

void FunctionCallExpressionAST::EmitMIR()
{
    //Visit called function
    MIRFunction& function=mir_emitter.functable.Visit(
        function_name.lexeme,function_name.location
    );
    
    if(function.parameter_number!=expressions.size())
        TypeError("The number of arguments does not match parameters",function_name.location);
    
    //@Instruction: call 
    shared_ptr<Calll> call=make_shared<Calll>();
    call->function_name=function.name;

    //Get arguments
    for(int i=0;i<expressions.size();i++)
    {
        //Emit argument expression
        EmitNode(expressions[i]);

        Check(mir_emitter.result_expression,function.local_variables[i].data_type);
        call->arguments.push_back(mir_emitter.result_expression);
    }

    //Get return value
    if(function.return_type!=VOID)
    {
        call->get_retvalue=mir_emitter.InFunction().NewTempLocalVar(function.return_type);
        //Set result value
        mir_emitter.result_expression=call->get_retvalue;
    }

    //Add to MIRBasicBlock
    mir_emitter.InBasicBlock().AddInsturction(call);
}