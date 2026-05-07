#include "utility/System.h"
#include "utility/Product.h"
#include "utility/FileManager.h"
#include "frontend/Scanner.h"
#include "frontend/Parser.h"
#include "frontend/MIREmitter.h"
#include "midend/LIREmitter.h"
#include "optimize/Optimizer.h"
#include "backend/CodeGenerator.h"

void Usage()
{
    cout<<"Usage: babywolf SysY_file"<<endl;

    exit(0);
}

void Compile()
{
    scanner.ScanTokens();
    //PrintTokens();

    parser.Parse();
    //PrintAST();

    mir_emitter.EmitMIR();
    optimizer.Optimize();
    //PrintMIR();

    lir_emitter.EmitLIR();
    //PrintLIR();

    code_generator.CodeGen();
    //PrintAssembly();
}

// int main(int argc,char* argv[])
// {
//     string input_file_name;

//     switch(argc)
//     {
//         case 2:
//             input_file_name=argv[1];
//             break;
//         default:
//             Usage();
//             break;
//     }

//     file_manager.Initialize(input_file_name,"");
//     file_manager.Open();
//     file_manager.Load();

//     Compile();

//     file_manager.Close();
    
//     return 0;
// }

int main(int argc,char* argv[])
{
    //compiler -S -o testcase.s testcase.sy -O1
    string input_file_name=argv[4];
    string output_file_name=argv[3];

    file_manager.Initialize(input_file_name,output_file_name);
    file_manager.Open();
    file_manager.Load();

    Compile();

    file_manager.Write(assembly.GetAsm());

    file_manager.Close();
    
    return 0;
}