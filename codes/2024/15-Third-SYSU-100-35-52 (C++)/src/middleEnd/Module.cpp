#include "Module.h"
#include "Function.h"
#include "StringTable.h"
#include "Value.h"
#include "Variable.h"

#include <memory>
#include <utility>
// NOLINTBEGIN
Module::Module() {
    globalStringTable = std::make_unique<StringTableNode>();
    currStringTable = globalStringTable.get();  // NOLINT

    vector<Value*> memsetArgv = { new Variable(TypePtr(new PtrType(Type::getInt8())), ""),
                                    new Variable(Type::getInt8(), ""),
                                    new Variable(Type::getInt64(), ""),
                                    new Variable(Type::getBool(), "") };
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "llvm.memset.p0i8.i64", true, memsetArgv));

    vector<Value*> memcpyArgv = { new Variable(TypePtr(new PtrType(Type::getInt8())), ""),
                                    new Variable(TypePtr(new PtrType(Type::getInt8())), ""),
                                    new Variable(Type::getInt64(), ""),
                                    new Variable(Type::getBool(), "") };
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "llvm.memcpy.p0i8.p0i8.i64", true, memcpyArgv));

    vector<Value*> putintArgv = { new Variable(Type::getInt(), "") };
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "putint", false, putintArgv));

    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "putch", false, putintArgv));

    vector<Value*> putfloatArgv = { new Variable(Type::getFloat(), "") };
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "putfloat", false, putfloatArgv));

    vector<Value*> putArrayArgv = { new Variable(Type::getInt(), ""),
                                      new Variable(TypePtr(new PtrType(Type::getInt())), "") };
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "putarray", false, putArrayArgv));
    vector<Value*> putfArrayArgv = { new Variable(Type::getInt(), ""),
                                       new Variable(TypePtr(new PtrType(Type::getFloat())), "") };
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "putfarray", false, putfArrayArgv));

    globalFunctions.emplace_back(std::make_unique<Function>(Type::getInt(), "getint", false));
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getInt(), "getch", false));
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getFloat(), "getfloat", false));
    vector<Value*> getArrayArgv = { new Variable(TypePtr(new PtrType(Type::getInt())), "") };
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getInt(), "getarray", false, getArrayArgv));
    vector<Value*> getfArrayArgv = { new Variable(TypePtr(new PtrType(Type::getFloat())), "") };
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getInt(), "getfarray", false, getfArrayArgv));
    vector<Value*> timeArgv = { new Variable(Type::getInt(), "") };

    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "_sysy_starttime", false, timeArgv));
    globalFunctions.emplace_back(std::make_unique<Function>(Type::getVoid(), "_sysy_stoptime", false, timeArgv));
}

void Module::pushFunction(std::unique_ptr<Function> function) {
    globalFunctions.emplace_back(std::move(function));
}

void Module::eraseFunction(Function* function){
    for(auto it = globalFunctions.begin(); it != globalFunctions.end(); it++){
        if(it->get() == function){
            globalFunctions.erase(it);
            return;
        }
    }
}

void Module::pushVariable(Variable* globalVariable, string trueName) {
    // to-do
    globalVariables.emplace_back(globalVariable);
    globalStringTable->insert(globalVariable, std::move(trueName));
}


StringTableNodePtr Module::popStringTable() {
    StringTableNodePtr tmp = currStringTable;
    currStringTable = currStringTable->father;
    return tmp;
}


FunctionPtr Module::getFunction(string name) {  // NOLINT
    if(name == "starttime")
        return globalFunctions[12].get();
    if(name == "stoptime")
        return globalFunctions[13].get();
    for(auto& func : globalFunctions) {
        if(name == func->getName())
            return func.get();
    }
    std::cerr << "func " << name << " not find!" << endl;
    return nullptr;
}

vector<Variable*> Module::getGlobalVariables() {
    vector<Variable*> res;
    for(auto& i : globalVariables)
        res.push_back(i);
    return res;
}
vector<Function*> Module::getGlobalFunctions(){
    vector<Function*> res;
    for(auto& i : globalFunctions)
        res.push_back(i.get());
    return res;
}



void Module::pushBackStringTable() {
    currStringTable = new StringTableNode(currStringTable);
}


// TODO: 为什么 同名 还使用oldname？
// void Module::VariableisterVariable(VariablePtr variable) {  // NOLINT
//     // fix :局部alloca的变量修正为Ptr,因为是通过指针来访问这些变量的
//     // 将原本的变量类型套上一层ptr
//     //  TypePtr addPtr = TypePtr(new PtrType(variable->type));
//     //  variable = VariablePtr(new Ptr(variable->name,variable->isGlobal,variable->isConst,addPtr));

//     VariablePtr tmp = currStringTable->lookUp(variable->name);

//     int stringLen = variable->name.length();  // NOLINT
//     if(stringLen > 1024) {
//         string newName = variable->name.substr(0, 1024);
//         currStringTable->variableTable[variable->name] = variable;
//         variable->name = newName;
//     }
//     // 处理同名，alloca统一在entry开头进行，不能有同名
//     else if(tmp && !(tmp->isGlobal) || globalFunctions.back()->findVariable(variable->name)) {
//         int suffix = 0;
//         string oldName = variable->name;
//         string newName = oldName;
//         do {                                          // NOLINT
//             newName = oldName + to_string(suffix++);  // to-do: 使用更高效的办法
//         } while(currStringTable->lookUp(newName) || globalFunctions.back()->findVariable(newName));
//         variable->name += to_string(suffix - 1);
//         currStringTable->variableTable[oldName] = variable;
//     } else {
//         currStringTable->insert(variable);
//     }
//     globalFunctions.back()->pushVariable(variable);
// }

void Module::dump(std::ostream& out) {
    for(auto& i : globalVariables)
        i->dump(out);
    for(auto& i : globalFunctions)
        if(!i->isLib)
            i->dump(out);
    for(auto& i : globalFunctions)
        if(i->isLib)
            i->dump(out);
}
// NOLINTEND