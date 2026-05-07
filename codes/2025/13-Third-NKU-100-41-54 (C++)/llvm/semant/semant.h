#ifndef SEMANT_H
 #define SEMANT_H
 
 #include "../include/ast.h"
 #include "../include/symbol.h"
 #include "../include/type.h"
 //#include "../include/Instruction.h"
 #include <unordered_map>
 #include <set>
 #include <vector>
 extern IdTable id_table;
 
 /*
 semant
 */
 class SemantTable {
 public:
 
     //std::unordered_map<Symbol*, FuncDef> FunctionTable;
     std::unordered_map<std::string, FuncDef> FunctionTable;
     SymbolTable symbol_table;
 
 
     std::unordered_map<Symbol*, NodeAttribute> GlobalTable;
     std::unordered_map<Symbol*, int> GlobalStrTable;//新增
     SemantTable() {
         
         Symbol* getint = id_table.add_id("getint");
         FunctionTable["getint"] = new __FuncDef(new BuiltinType(BuiltinType::Int), getint, new std::vector<FuncFParam>{}, nullptr);
 
         Symbol* getch = id_table.add_id("getch");
         FunctionTable["getch"] = new __FuncDef(new BuiltinType(BuiltinType::Int), getch, new std::vector<FuncFParam>{}, nullptr);
 
         Symbol* getfloat = id_table.add_id("getfloat");
         FunctionTable["getfloat"] = new __FuncDef(new BuiltinType(BuiltinType::Float), getfloat, new std::vector<FuncFParam>{}, nullptr);
 
         Symbol* getarray = id_table.add_id("getarray");
         FunctionTable["getarray"] = new __FuncDef(
         new BuiltinType(BuiltinType::Int), getarray,
         new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Int), new std::vector<ExprBase>(1, nullptr))}, nullptr);
 
         Symbol* getfarray = id_table.add_id("getfarray");
         FunctionTable["getfarray"] = new __FuncDef(
         new BuiltinType(BuiltinType::Int), getfarray,
         new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Float), new std::vector<ExprBase>(1, nullptr))}, nullptr);
 
         Symbol* putint = id_table.add_id("putint");
         FunctionTable["putint"] =
         new __FuncDef(new BuiltinType(BuiltinType::Void), putint, new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Int))}, nullptr);
 
         Symbol* putch = id_table.add_id("putch");
         FunctionTable["putch"] =
         new __FuncDef(new BuiltinType(BuiltinType::Void), putch, new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Int))}, nullptr);
 
         Symbol* putfloat = id_table.add_id("putfloat");
         FunctionTable["putfloat"] =
         new __FuncDef(new BuiltinType(BuiltinType::Void), putfloat, new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Float))}, nullptr);
 
         Symbol* putarray = id_table.add_id("putarray");
         FunctionTable["putarray"] =
         new __FuncDef(new BuiltinType(BuiltinType::Void), putarray,
                       new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Int)),
                                                   new __FuncFParam(new BuiltinType(BuiltinType::Int), new std::vector<ExprBase>(1, nullptr))},
                       nullptr);
 
         Symbol* putfarray = id_table.add_id("putfarray");
         FunctionTable["putfarray"] = new __FuncDef(
         new BuiltinType(BuiltinType::Void), putfarray,
         new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Int)),
                                     new __FuncFParam(new BuiltinType(BuiltinType::Float), new std::vector<ExprBase>(1, nullptr))},
         nullptr);
 
         Symbol* starttime = id_table.add_id("_sysy_starttime");
         FunctionTable["_sysy_starttime"] =
         new __FuncDef(new BuiltinType(BuiltinType::Void), starttime, new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Int))}, nullptr);
 
         Symbol* stoptime = id_table.add_id("_sysy_stoptime");
         FunctionTable["_sysy_stoptime"] =
         new __FuncDef(new BuiltinType(BuiltinType::Void), stoptime, new std::vector<FuncFParam>{new __FuncFParam(new BuiltinType(BuiltinType::Int))}, nullptr);
 
         // 这里你可能还需要对这些语法树上的节点进行类型的标注, 进而检查对库函数的调用是否符合形参
         // 即正确填写NodeAttribute或者使用其他方法来完成检查
         for (auto F : FunctionTable) {
             F.second->TypeCheck();
         }
     }
 };
 
 #endif