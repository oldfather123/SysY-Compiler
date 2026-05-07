#pragma once

#include "../utility/System.h"

class FunctionAssembly
{
private:
    string function_head;
    string function_prologue;
    string function_body;
    string function_epilogue;

public:
    FunctionAssembly(string function_name){
        function_head="\n.globl "+function_name+"\n"+function_name+":\n";
    }

    void AddPrologue(string prologue);
    void AddBody(string body);
    void AddEpilogue(string epilogue);

    string GetAsm();
};

class ProgramAssembly
{
private:
    string data;
    vector<FunctionAssembly> text;
public:
    ProgramAssembly():data(".data\n"){}

    void AddData(string global_def);
    void NewFunction(string function_name);
    
    void AddPrologue(string prologue);
    void AddBody(string body);
    void AddEpilogue(string epilogue);
    void AddLable(string lable);

    string GetAsm();
};