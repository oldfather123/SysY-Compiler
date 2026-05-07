//
// Created by q1w2e3r4 on 23-7-17.
//

#include "GlobalUnit.h"
#include "codegen/AsmBuilder.hpp"
#include "codegen/MachineFunction.hpp"
#include "IRInstruction.h"
#include <memory>
#include <type_traits>

Function * GlobalUnit::addFunc(std::string& funcName, Type *retType, std::vector<Type *> paramsTypes) {
    FuncType * funcType = new FuncType(retType,paramsTypes);
    Function * function = new Function(funcName,funcType);
    func_table[funcName] = function;
    return function;
}

Function* GlobalUnit::getFunc(const std::string& funcName) {
    auto it = func_table.find(funcName);
    if (it == func_table.end()){
        cerr << "Function not exist!" << endl;
        throw exception();
    }
    return it->second;
}

void GlobalUnit::addSymbol(Function* func,std::string& symbolName,Symbol* symbol) {
    if(func == nullptr) // global symbol
        global_symbol_table[symbolName] = symbol;
    else
        func->addSymbol(symbolName,symbol);
}


void GlobalUnit::PrintSymbol() {
    cout << "Global Symbol:" << endl;
    for(const auto& it:global_symbol_table){
        cout << it.first << ": " << it.second->symbolType->getSize() << endl;
    }
    for(const auto& it:func_table){
        it.second->PrintSymbol();
    }
}


void GlobalUnit::codegen(MachineUnit *mUnit, AsmBuilder *builder) {
    builder->setUnit(mUnit);
    mUnit->IR_func_table = this->func_table;

//    for(const auto& it:global_symbol_table){
//        if(it.second->constVal != nullptr){
//            cout << it.first << ":" << endl;
//            cout << "\t.word\t" << it.second->constVal->get_Ref() << endl;
//        }
//    }
    for(auto ir: this->global_instr){
        if(dynamic_cast<AddGlobalInstruction*>(ir)){
            dynamic_cast<AddGlobalInstruction*>(ir)->codegen(builder);
        }else{
            throw NotImplemented("global var");
        }
    }

    for (auto &[name, func] : func_table) {
        if (func->block_list.size() > 0) func->codegen(builder);
    }

    for(auto& floatconst : IRInstruction::float_table){
        mUnit->float_literals << floatconst.first << ':' << endl;
        float f = floatconst.second;
        std::bitset<32> bits(*reinterpret_cast<unsigned int*>(&f));
        unsigned long decimal_value = static_cast<int>(bits.to_ulong());
        if (f < 0) {
            decimal_value = ~(decimal_value - 1);
            mUnit->float_literals << "\t.word\t" << "-" << decimal_value << '\n';
        } else{
            mUnit->float_literals << "\t.word\t" << decimal_value << '\n';
        }
    }
}
void GlobalUnit::Emit(std::ostream &os) {
    os << "declare i32 @getint()" << std::endl;
    os << "declare i32 @getch()" << std::endl;
    os << "declare float @getfloat()" << std::endl;
    os << "declare i32 @getarray(i32* %0)" << std::endl;
    os << "declare i32 @getfarray(float* %0)" << std::endl;
    os << "declare void @putint(i32 %0)" << std::endl;
    os << "declare void @putch(i32 %0)" << std::endl;
    os << "declare void @putarray(i32 %0, i32* %1)" << std::endl;
    os << "declare void @putfloat(float %0)" << std::endl;
    os << "declare void @putfarray(i32 %0, float* %1)" << std::endl;
    os << "declare void @putf(i32* %0, ...)" << std::endl;
    os << "declare void @before_main()" << std::endl;
    os << "declare void @after_main()" << std::endl;
    os << "declare void @_sysy_starttime(i32 %0)" << std::endl;
    os << "declare void @_sysy_stoptime(i32 %0)" << std::endl;
    os << "declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg)" << endl;

    for(auto instr: global_instr){
        instr->outPut(os);
    }
    for(auto it: this->func_table){
        it.second->Emit(os);
    }
}
