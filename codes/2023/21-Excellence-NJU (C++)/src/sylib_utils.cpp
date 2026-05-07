#include "MyVisitor.h"
#include "Type.h"
#include <initializer_list>
#include <memory>

vector<sylib_func_t> sylib_funcs{
    //   funcName               retType     paramsType
    {"getint"s,             INT32TYPE,  {}},
    {"getch"s,              INT32TYPE,  {}},
    {"getfloat"s,           FLOATTYPE,  {}},
    {"getarray"s,           INT32TYPE,  { new PointerType(new Type(INT32TYPE)) }},
    {"getfarray"s,          INT32TYPE,  { new PointerType(new Type(FLOATTYPE)) }},

    {"putint"s,             VOIDTYPE,   {new Type(INT32TYPE)}},
    {"putch"s,              VOIDTYPE,   {new Type(INT32TYPE)}},
    {"putarray"s,           VOIDTYPE,   {new Type(INT32TYPE), new PointerType(new Type(INT32TYPE))}},
    {"putfloat"s,           VOIDTYPE,   {new Type(FLOATTYPE)}},
    {"putfarray"s,          VOIDTYPE,   {new Type(INT32TYPE), new PointerType(new Type(FLOATTYPE))}},

    {"putf"s,              VOIDTYPE,   { /* TODO */}},
    {"before_main"s,       VOIDTYPE,   {}},
    {"after_main"s,        VOIDTYPE,   {}},
};

void MyVisitor::add_sylib_func() {
    for (auto &func : sylib_funcs) {
        globalUnit->addFunc(func._funcName, new Type(func._retType), std::move(func._paramsTypes));
    }

    vector<Type*> v;
    v.push_back(new Type(INT32TYPE));
    FuncType * funcType = new FuncType(new Type(VOIDTYPE),v);
    Function * function = new Function("_sysy_starttime"s,funcType);
    globalUnit->func_table["starttime"] = function;

    function = new Function("_sysy_stoptime"s,funcType);
    globalUnit->func_table["stoptime"] = function;
}