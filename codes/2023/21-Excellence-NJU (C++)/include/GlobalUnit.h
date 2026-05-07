//
// Created by q1w2e3r4 on 23-7-17.
//
#include "Function.h"
#include "codegen/AsmBuilder.hpp"
#include "codegen/MachineFunction.hpp"
#include <memory>
#include "Instruction.h"
#include <vector>
#include <map>

#ifndef COMPILER_GLOBALUNIT_H
#define COMPILER_GLOBALUNIT_H


class GlobalUnit {
public:
    map<string,Function*> func_table;
    map<string,Symbol*> global_symbol_table;
    vector<Instruction*> global_instr;

    Function* addFunc(std::string& funcName,Type* retType,std::vector<Type*> paramsTypes);

    Function* getFunc(const std::string& funcName);

    void addSymbol(Function* func,std::string& symbolName,Symbol* symbol);

    void codegen(MachineUnit *mUnit, AsmBuilder *builder);
    void addInstr(Instruction* instr){
        global_instr.push_back(instr);
    }

    void Emit(std::ostream &os);

    void PrintSymbol(); // debug
};


#endif //COMPILER_GLOBALUNIT_H
