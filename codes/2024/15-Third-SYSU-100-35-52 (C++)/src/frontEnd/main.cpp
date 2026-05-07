#include "MyVisitor.h"
#include "SysY2022Lexer.h"
#include "Value.h"
#include "antlr4-runtime.h"
#include "riscv.h"
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <string>
#include <utils.h>

using namespace antlr4;



int main(int argc, char** argv) {  // NOLINT
    std::ifstream sourceFile(argv[4]);
    assert(sourceFile.is_open());
    ANTLRInputStream input(sourceFile);
    SysY2022Lexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    tokens.fill();
    SysY2022Parser parser(&tokens);
    SysY2022Parser::CompUnitContext* tree = parser.compUnit();
    MyVisitor visitor;
    visitor.visitCompUnit(tree);
    visitor.opt();


    bool emitIR = strcmp(argv[1], "--emit-ir") == 0;
    std::ofstream irFile;
    if(emitIR) {
        // FILE* ir_file = fopen(argv[3], "w");
        irFile.open(argv[3], std::ios::out);
        assert(irFile);
        visitor.print(irFile);
        irFile.close();
    } else {
        auto progAsm = emitProg(visitor.irModule);
        cout << "finish emitProg" << endl;
        optimizeMachineIR(progAsm);
        cout << "finish bless" << endl;
        StringBuilder s;
        auto globalVariables = visitor.irModule.getGlobalVariables();
        buildProgram(&s, progAsm, globalVariables, visitor.irModule.shouldAddCacheLookup);
        s.addTerminator();

        FILE* assemblyFile = fopen(argv[3], "w");
        if(assemblyFile == NULL) {
            assert(false && "error opening assembly output file");
        }
        fprintf(assemblyFile, "%s", s.c_str());
        fclose(assemblyFile);
    }


    //FIXME:  加一些主动释放内存的代码
    //for(auto& func : visitor.irModule.globalFunctions) {
    //    for (auto& arg : func->formArguments) {
    //        delete arg; // NOLINT
    //    }
    //}


    //for(Type* type : Type::types) {
    //    delete type;  // NOLINT
    //}
    //Type::types.clear();

    //delete Void::value;  // NOLINT

    //// delete global variables
    //for(auto& var : visitor.irModule.globalVariables) {
    //    delete var;  // NOLINT
    //}

    return 0;
}