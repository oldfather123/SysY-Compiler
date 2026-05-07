// #include <assert.h>
// #include <function.h>
// #include <ir.h>
// #include <value.h>
// #include <lexer.h>
// #include <stdarg.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include <symbol.h>
// #include <sysy.h>
// #include <type.h>
// #include <util.h>

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/lexer.h"
#include "../include/m-bitset.h"
#include "../include/pass.h"
#include "../include/stack.h"
#include "../include/sysy.h"
#include "../include/type.h"
#include "../include/util.h"
#include "../include/value.h"
#include "../include/vector.h"

Symbol   *tok_starttime;
Symbol   *tok_stoptime;

Symbol   *tok_sysy_starttime;
Symbol   *tok_sysy_stoptime;

Function *definedFunction(char *name, FunctionType *fncType) {
    Type      t = {IS_FNC};
    Symbol   *sym = pushSymbol(defineToken(name, strlen(name))->tok, &t);
    Function *fnc = createFunction(NULL, sym);
    fnc->fncTy = *fncType;
    fnc->isExtern = 1;
    return fnc;
}

Symbol *makeTypeInt() {
    Symbol *s;
    s = sy_malloc(sizeof(Symbol));
    s->type = TY_INT;
    return s;
};
Symbol *makeTypeFloat() {
    Symbol *s;
    s = sy_malloc(sizeof(Symbol));
    s->type = TY_FLOAT;
    return s;
};
// n代表几个维度
//-1代表[]
Symbol *makeTypeArray(Type bty, int n, ...) {
    va_list ap;
    va_start(ap, n);

    Symbol *s;
    size_t  i;
    s = sy_malloc(sizeof(Symbol));
    s->type = bty | IS_ARRAY | IS_POINTER;
    for (i = 0; i < n; i++) {
        int tmp = va_arg(ap, int);
        arrayAddDimension(s, tmp);
    }
    return s;
}
/*
int getint(),getch(),getarray(int a[]);
float getfloat();
int getfarray(float a[]);

void putint(int a),putch(int a),putarray(int n,int a[]);
void putfloat(float a);
void putfarray(int n, float a[]);

void putf(char a[], ...);

 */

void sysy_init() {
    FunctionType ty = {0};
    vector_init(&ty.params);
    ty.ret = TY_INT;
    definedFunction("getint", &ty);
    definedFunction("getch", &ty);

    vector_init(&ty.params);
    ty.ret = TY_INT;
    vector_push_back(&ty.params, makeTypeArray(TY_INT, 1, -1));
    definedFunction("getarray", &ty);

    vector_init(&ty.params);
    ty.ret = TY_FLOAT;
    definedFunction("getfloat", &ty);

    vector_init(&ty.params);
    ty.ret = TY_INT;
    vector_push_back(&ty.params, makeTypeArray(TY_FLOAT, 1, -1));
    definedFunction("getfarray", &ty);

    vector_init(&ty.params);
    ty.ret = TY_VOID;
    vector_push_back(&ty.params, makeTypeInt());
    definedFunction("putint", &ty);

    vector_init(&ty.params);
    ty.ret = TY_VOID;
    vector_push_back(&ty.params, makeTypeInt());
    definedFunction("putch", &ty);

    vector_init(&ty.params);
    ty.ret = TY_VOID;
    vector_push_back(&ty.params, makeTypeInt());
    vector_push_back(&ty.params, makeTypeArray(TY_INT, 1, -1));
    definedFunction("putarray", &ty);

    vector_init(&ty.params);
    ty.ret = TY_VOID;
    vector_push_back(&ty.params, makeTypeFloat());
    definedFunction("putfloat", &ty);

    vector_init(&ty.params);
    ty.ret = TY_VOID;
    vector_push_back(&ty.params, makeTypeInt());
    vector_push_back(&ty.params, makeTypeArray(TY_FLOAT, 1, -1));
    definedFunction("putfarray", &ty);

    // vector_init(&ty.params);
    // ty.ret = TY_VOID;
    // vector_push_back(&ty.params, );
    // definedFunction("putf", &ty);

    vector_init(&ty.params);
    ty.ret = TY_VOID;
    definedFunction("before_main", &ty)->attribute = ATTR_constructor;
    definedFunction("after_main", &ty)->attribute = ATTR_destructor;

    vector_init(&ty.params);
    ty.ret = TY_VOID;
    // Function *f = definedFunction("stoptime", &ty);
    tok_stoptime = definedFunction("stoptime", &ty)->symbol;

    tok_starttime = definedFunction("starttime", &ty)->symbol;
    // f = definedFunction("tok_starttime", &ty);

    vector_init(&ty.params);
    ty.ret = TY_VOID;
    vector_push_back(&ty.params, makeTypeInt());
    tok_sysy_starttime = definedFunction("_sysy_starttime", &ty)->symbol;
    tok_sysy_stoptime = definedFunction("_sysy_stoptime", &ty)->symbol;

    vector_init(&ty.params);
    ty.ret = TY_INT;
    // TY_VOID | TY_POINTER,字符串指针
    vector_push_back(&ty.params, makeTypeArray(TY_VOID, 1, -1));
    definedFunction("printf", &ty);
}

// void putfloat(float a);
// void putfarray(int n, float a[]);

// void putf(char a[], ...);