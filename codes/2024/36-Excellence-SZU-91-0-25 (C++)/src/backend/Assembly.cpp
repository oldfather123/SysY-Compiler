#include "../backend/Assembly.h"
#include "../utility/Diagnostor.h"

void FunctionAssembly::AddPrologue(string prologue)
{
    function_prologue+=prologue;
}

void FunctionAssembly::AddBody(string body)
{
    function_body+=body;
}

void FunctionAssembly::AddEpilogue(string epilogue)
{
    function_epilogue+=epilogue;
}

string FunctionAssembly::GetAsm()
{
    return function_head+function_prologue+function_body+function_epilogue;
}



void ProgramAssembly::AddData(string global_def)
{
    data+=global_def;
}

void ProgramAssembly::NewFunction(string function_name)
{
    text.push_back(FunctionAssembly(function_name));
}



void ProgramAssembly::AddPrologue(string prologue)
{
    if(!text.empty())
        text.back().AddPrologue("\t"+prologue+"\n");
    else CompilerError("text is empty");
}

void ProgramAssembly::AddBody(string body)
{
    if(!text.empty())
        text.back().AddPrologue("\t"+body+"\n");
    else CompilerError("text is empty");
}

void ProgramAssembly::AddEpilogue(string epilogue)
{
    if(!text.empty())
        text.back().AddPrologue("\t"+epilogue+"\n");
    else CompilerError("text is empty");
}

void ProgramAssembly::AddLable(string lable)
{
    if(!text.empty())
        text.back().AddPrologue("  "+lable+":\n");
    else CompilerError("text is empty");
}



string ProgramAssembly::GetAsm()
{
    string assembly;
    
    assembly+=data;
    
    assembly+="\n.text\n";
    for(FunctionAssembly& func_asm:text)
        assembly+=func_asm.GetAsm();

    return assembly;
}