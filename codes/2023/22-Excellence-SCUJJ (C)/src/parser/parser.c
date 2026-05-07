// #include <assert.h>
// #include <function.h>
// #include <ir.h>
// #include <value.h>
// #include <lexer.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <symbol.h>
// #include <sysy.h>
// #include <type.h>
// #include <util.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/lexer.h"
#include "../include/m-bitset.h"
#include "../include/pass.h"
#include "../include/symbol.h"
#include "../include/sysy.h"
#include "../include/type.h"
#include "../include/util.h"
#include "../include/value.h"
#include "../include/vector.h"

int                scope;
SYContext         *context;
Function          *_fnc;
extern bool        isOptimimze;

// BType → 'int' | 'float',分析基本类型
bool               parseBType(Type *ty);

static Value      *constExpr();
static Value      *Expr();
static Value      *AddExp();
static Value      *MulExp();
static Value      *UnaryExp();
static Value      *condExpr();
static Value      *LOrExp();
static Value      *LAndExp();
static Value      *EqExp();
static Value      *RelExp();

static void        block(BasicBlock *b, BasicBlock *c, BasicBlock *n);
static void        stmt();
static Value       NotEqualToZero(Value *v);
static Value      *createValue() { return (Value *)sy_malloc(sizeof(Value)); }
static inline void freeValue(Value *ptr) { return free(ptr); }
// 为数组增加一个维度
//-1代表数组没指定值
void               arrayAddDimension(Symbol *sym, int size) {
    assert(sym);
    ArrayType *array = sym->arrayTy;

    if (!array) {
        sym->arrayTy = sy_malloc(sizeof(ArrayType));
        sym->arrayTy->size = size;
        return;
    }
    while (array->next) {
        array = array->next;
    }
    array->next = sy_malloc(sizeof(ArrayType));
    array->next->size = size;
}

// FuncFParams → FuncFParam { ',' FuncFParam }
// FuncFParam  →  BType Ident ['[' ']' { '[' Exp ']' }]
static void parseFuncFParams(FunctionType *ty) {
    scope++;  // 函数参数是局部变量,后面进入block还会增加
    uint32_t ident;
    Symbol  *param;
    do {
        Type t = {0};
        if (!parseBType(&t)) {
            scope--;
            return;
        }
        ident = curId;
        consume(TOK_IDENT, "Expected a identifier");
        if (match(TOK_SqurL)) {
            // TODO
            consume(TOK_SqurR, "缺少");
            param = pushSymbol(ident, &t);
            param->type |= IS_ARRAY;
            param->isPointer = 1;
            arrayAddDimension(param, -1);
            while (match(TOK_SqurL)) {
                Value *val = Expr();
                if (!(ValueIsImm(val) || ValueIsFloat(val))) {
                    // TODO
                    sy_error("必须是整型常量");
                }
                consume(TOK_SqurR, "缺少");
                arrayAddDimension(param, val->imm.i);
                free(val);
            }
        } else {
            param = pushSymbol(ident, &t);
        }
        vector_push_back(&ty->params, param);
        param->ind = vector_len(&_fnc->symPool);
        vector_push_back(&_fnc->symPool, param);
        if (!match(TOK_Comma)) {
            break;
        }
    } while (1);
    scope--;
}

size_t getTypeSize(Symbol *sym) {
    ArrayType *at = NULL;
    size_t     size = 0;
    switch (sym->type) {
        case TY_INT:
            if (sym->type | IS_ARRAY) {
                at = sym->arrayTy;
                size = 4;
                while (at != NULL) {
                    size *= at->size;
                    at = at->next;
                }
                return size;
            }
            return 4;
        case TY_FLOAT:
            if (sym->type | IS_ARRAY) {
                at = sym->arrayTy;
                size = 4;
                while (at != NULL) {
                    size *= at->size;
                    at = at->next;
                }
                return size;
            }
            return 4;
    }
    assert(0);
}
// 得到当前维度值
size_t getArraySize(ArrayType *ty) {
    int size = 1;
    if (ty == NULL) return 0;
    while (ty != NULL) {
        size *= ty->size;
        ty = ty->next;
    }
    return size;
}
// 得到数组字节大小
size_t getArraySize2(ArrayType *ty) {
    int size = 1;
    if (ty == NULL) return 4;
    while (ty != NULL) {
        size *= ty->size;
        ty = ty->next;
    }
    return size * 4;
}
int get2(ArrayType *ty) {
    int size = 1;
    if (ty == NULL) return 0;
    while (ty && ty->next) {
        ty = ty->next;
    }
    return ty->size;
}
vector_template(int, arrayDn1);
struct arrrayDn {
    int dn;
    int curValNum;
};
vector_template(struct arrrayDn, arrayInfo);
int initDn(Symbol *sym) {
    ArrayType *tmp = sym->arrayTy;
    vector_clear(&arrayInfo);
    while (tmp) {
        struct arrrayDn ad = {tmp->size, 0};
        vector_push_back(&arrayInfo, ad);
        tmp = tmp->next;
    }
}
void dnAdd1() {
    struct arrrayDn *ad;
    int              i;
    vector_each_address(&arrayInfo, i, ad) { printf("%d ", ad->curValNum); }
    vector_get_address(&arrayInfo, vector_len(&arrayInfo) - 1)->curValNum++;
    vector_each_reverse_address(&arrayInfo, i, ad) {
        if (ad->curValNum >= ad->dn) {
            ad->curValNum = 0;
            vector_get_address(&arrayInfo, i - 1)->curValNum++;
        }
    }
    printf("\n");
}
size_t initExprNum;
void   dnAdd() {
    struct arrrayDn ad;
    int             i;
    // vector_each(&arrayInfo, i, ad) { printf("%d ", ad.curValNum); }
    // assert(vector_len(&arrayInfo) != 0);
    vector_get_address(&arrayInfo, vector_len(&arrayInfo) - 1)->curValNum++;
    vector_each_reverse(&arrayInfo, i, ad) {
        if (ad.curValNum >= ad.dn) {
            if (i >= 1) vector_get_address(&arrayInfo, i - 1)->curValNum = 0;
            if (i >= 2) vector_get_address(&arrayInfo, i - 2)->curValNum++;
        }
    }
    printf("\n");
}
extern FILE *asmFile;

void         initDnVal(Symbol *sym, Value *val) {
    size_t          i;
    struct arrrayDn ad;

    if (scope == 0) {
        // initExprNum++;

        if (sym->type & TY_INT) {
            if (ValueIsFloat(val)) {
                // assert(0);
                fprintf(asmFile, ".word %d\n", (int)val->imm.f);
            } else {
                fprintf(asmFile, ".word %d\n", val->imm.i);
            }
        } else if (sym->type & TY_FLOAT) {
            if (ValueIsInt(val)) {
                // ((float *)sym->arrayVal)[sym->arrayCount] =
                // (float)val->imm.i;
                fprintf(asmFile, ".word %d\n", val->imm.i);
            } else {
                // ((float *)sym->arrayVal)[sym->arrayCount] = val->imm.f;
                fprintf(asmFile, ".word %d\n", *((int *)(&val->imm.f)));
            }
        }
        dnAdd();
        return;
    }
    Value        v = symbolToValue(sym);
    Value        ind = SET_IMM_INT(0);
    // Value        ptr;
    instruction *ptr = createGetitemptr(_fnc, &v);
    vector_each(&arrayInfo, i, ad) {
        ind.imm.i = ad.curValNum;
        // v = createGetPtr(_fnc, &v, &ind);
        addArrayIndex(ptr, &ind);
    }
    // vector_push_back(&_fnc->currentBB->inst, ptr);
    if (!isOptimimze) {
        vector_push_back(&_fnc->currentBB->inst, ptr);
    } else {
        DL_APPEND(_fnc->currentBB->instlist, ptr);
    }
    typeCast(ptr->ret.ty, val);
    createStore(_fnc, &ptr->ret, val);
    dnAdd();
}
void initDnZero(Symbol *sym, ArrayType *ty, int exprNum) {
    size_t          i;
    struct arrrayDn ad;

    Value           ind = SET_IMM_INT(0);
    if (sym->iszero) {
        while (exprNum != getArraySize(ty)) {
            // 大数组
            dnAdd();
            exprNum++;
        }
        return;
    }
    while (exprNum != getArraySize(ty)) {
        // 大数组
        // initExprNum++;

        initDnVal(sym, &ind);
        exprNum++;
    }
}
void initArrayVal1(Symbol *sym, ArrayType *ty, int curDn, int exprNum) {
    int n = exprNum;
    while (n > get2(ty)) {
    }
}
void initArrayZero1(Symbol *sym, ArrayType *ty) {}
vector_template(int, arrayDn);
void initArrayZero(Symbol *sym, ArrayType *ty) {
    int curDn = 0;
    if (ty->next) {
        while (curDn < getArraySize(ty)) {
            vector_push_back(&arrayDn, curDn);
            initArrayZero(sym, ty->next);
            int _i;
            vector_pop_back(&arrayDn, _i);
            curDn++;
        }
    } else {
        size_t i, _i;
        Value  v = symbolToValue(sym);
        Value  ind = SET_IMM_INT(0);
        // Value  ptr;
        _i = 0;
        while (_i < ty->size) {
            if (scope == 0) {
                if (sym->type & TY_INT) {
                    fprintf(asmFile, ".word %d\n", 0);
                    // ((int *)sym->arrayVal)[sym->arrayCount] = 0;
                } else if (sym->type & TY_FLOAT) {
                    float t = 0;
                    fprintf(asmFile, ".word %d\n", *((int *)(&t)));
                    // ((float *)sym->arrayVal)[sym->arrayCount] = 0.;
                }
                _i++;
                continue;
            }
            v = symbolToValue(sym);
            instruction *ptr = createGetitemptr(_fnc, &v);

            vector_each(&arrayDn, i, curDn) {
                ind.imm.i = curDn;
                // v = createGetPtr(_fnc, &v, &ind);
                addArrayIndex(ptr, &ind);
            }
            i = 0;
            // 最后一个维度
            {
                ind.imm.i = _i;
                // ptr = createGetPtr(_fnc, &v, &ind);
                addArrayIndex(ptr, &ind);
                ind.imm.i = 0;
                // vector_push_back(&_fnc->currentBB->inst, ptr);
                if (!isOptimimze) {
                    vector_push_back(&_fnc->currentBB->inst, ptr);
                } else {
                    DL_APPEND(_fnc->currentBB->instlist, ptr);
                }
                createStore(_fnc, &ptr->ret, &ind);
                _i++;
            }
        }
    }
}
int  Curlyn = 0;
bool first = true;
// void initArray(Symbol *sym, ArrayType *ty) {
//     int exprNum = 0;
//     int curDn = 0;
//     Curlyn = 0;
//     while (curTok == TOK_CurlyL || first) {
//         Curlyn++;
//         first = false;
//         next();
//         exprNum = 0;
//         curDn++;
//         if (curTok == (TOK_CurlyL) && 0) {
//             vector_push_back(&arrayDn, curDn);
//             initArray(sym, ty->next);
//             int _n;
//             vector_pop_back(&arrayDn, _n);
//             curDn++;
int  curDn;

void initArray(Symbol *sym, ArrayType *ty) {
    int exprNum = 0;
    if (curTok == TOK_CurlyL || first) {
        curDn++;
        if (first) {
            first = false;
        } else {
            match(TOK_CurlyL);
        }
        if (match(TOK_CurlyR)) {
            if (!sym->iszero) {
                for (int i = 0; i < getArraySize(ty); i++) {
                    Value ind = SET_IMM_INT(0);
                    initDnVal(sym, &ind);
                }
            } else {
                for (int i = 0; i < getArraySize(ty); i++) {
                    dnAdd();
                }
            }
            curDn--;
            return;
        }
        while (1) {
            if (curTok == TOK_CurlyL) {
                if (!exprNum == 0) {
                    if (exprNum < get2(ty)) {
                        // curDn第几个的维度剩下补充0
                        initDnZero(sym, ty, curDn);
                    }
                }
                initArray(sym, ty->next);
            } else if (curTok == TOK_CurlyR) {
                if (!exprNum == 0) {
                    if (exprNum < get2(ty)) {
                        // curDn第几个的维度剩下补充0
                        initDnZero(sym, ty, exprNum);
                    }
                }
                match(TOK_CurlyR);
                curDn--;
                return;
            } else {
                Value *val = Expr();
                initDnVal(sym, val);
                free(val);
                exprNum++;
            }
            match(TOK_Comma);
        }
    }
}

static void initFunction(Function *fnc) {
    Symbol *s;
    vector_each3(&fnc->fncTy.params, s) {
        Value v;
        v = symbolToValue(s);
        createArgument(fnc, &v);
    }
}

// CompUnit → [CompUnit](Decl | FuncDef)
bool decl() {
    uint32_t ident;
    bool     isConst = false;
    Symbol  *sym = NULL;
    Type     t = {0};
    if (match(TOK_CONST)) {
        t |= IS_COSNT;
        isConst = true;
    }
    if (!parseBType(&t)) {
        // 函数内可以没有声明语句
        if (scope) {
            return false;
        } else
            sy_error("No type information");  // 没有类型信息
    }
_var:
    ident = curId;
    consume(TOK_IDENT, "Expected a identifier");

    if (match(TOK_ParL)) {
        // FuncDef → FuncType Ident '(' [FuncFParams] ')' Block

        // TODO返回值类型
        t |= IS_FNC;
        sym = pushSymbol(ident, &t);
        Function *fnc = createFunction(context, sym);
        _fnc = fnc;
        if (isOptimimze) {
            fnc->insert = 1;
        }
        localStack = NULL;
        Symbol *tmp = localStack;  // 弹出参数
        parseFuncFParams(&fnc->fncTy);
        fnc->fncTy.ret = t & BTYPE_MASK;
        // TODO
        consume(TOK_ParR, "缺少");

        // 每个函数的第一个基本快都是空基本块
        BasicBlock *firstbb = createBasicBlock(_fnc);
        fnc->firstBB = firstbb;
        setCurrentBB(fnc, firstbb);
        initFunction(fnc);
        BasicBlock *bb = createBasicBlock(_fnc);
        firstbb->nextblock = bb;
        setCurrentBB(_fnc, bb);
        fprintf(stderr, "parse function %s\n", fnc->name);
        block(NULL, NULL, NULL);
        assert(scope == 0);
        fprintf(stderr, "parse  end\n");
        _fnc->finalBB = _fnc->currentBB;
        popSymbol(tmp);
        fprintf(stderr, "pop symbol  end\n");
        optimize(fnc);
        fprintf(stderr, "optimize function %s\n", fnc->name);
    } else {
        // ConstDecl | VarDecl
        if (t & TY_VOID) {
            sy_error("Not an allowed type ");  // 不允许的类型
        }
        sym = pushSymbol(ident, &t);
        // 局部变量放在symPool
        //  TODO 这里实现得不好
        if (scope != 0) {
            sym->ind = vector_len(&_fnc->symPool);
            vector_push_back(&_fnc->symPool, sym);
        }
        // 数组
        if (curTok == TOK_SqurL) {
            sym->type |= IS_ARRAY | IS_POINTER;
            while (match(TOK_SqurL)) {
                Value *val = Expr();
                if (!ValueIsImm(val)) {
                    // TODO 非负数
                    sy_error("必须是整型常量");
                }
                arrayAddDimension(sym, val->imm.i);
                // TODO
                consume(TOK_SqurR, "缺少");
            }
            if (match(TOK_Assign)) {
                sym->isInit = 1;
                if (curTok != TOK_CurlyL) {
                    sy_error("缺少");
                }
                if (match(TOK_CurlyL)) {
                    if (match(TOK_CurlyR)) {
                        sym->iszero = 1;
                        goto _go;
                    }
                }
                if (getArraySize(sym->arrayTy) >= 1000) {
                    // 大数组
                    sym->iszero = 1;
                }
                // TODO
                initDn(sym);
                if (scope == 0) {
                    fprintf(asmFile, ".data\n");  //.size buffer,262144
                    // fprintf(asmFile, ".size %s:\n", getToken(sym->tok)->str);
                    fprintf(asmFile, ".%s:\n", getToken(sym->tok)->str);
                    // sy标准中int和float类型都是4字节
                    // sym->arrayVal = sy_malloc(getArraySize(sym->arrayTy) *
                    // 4);
                }
                first = true;
                curDn = 0;
                vector_clear(&arrayDn);
                initExprNum = 0;
                initArray(sym, sym->arrayTy);
                if ((sym->iszero) && (scope == 0)) {
                    fprintf(asmFile, ".zero %d\n",
                            (getArraySize(sym->arrayTy) - initExprNum) * 4);
                }
                goto _go;
            } else {
                if (isConst) sy_error("const 必须赋值");
            }
        }

        if (match(TOK_Assign)) {
            sym->isInit = 1;
            Value *val = Expr();

            if (isConst || scope == 0) {
                if (!ValueIsImm(val)) {
                    if (isConst) sy_error("必须是常量");
                }
                // initialize
                if (sym->type & TY_INT) {
                    if (ValueIsFloat(val)) {
                        sym->i = (int)val->imm.f;
                    } else {
                        sym->i = val->imm.i;
                    }
                    free(val);
                } else if (sym->type & TY_FLOAT) {
                    if (ValueIsInt(val)) {
                        sym->f = (float)val->imm.i;
                    } else {
                        sym->f = val->imm.f;
                    }
                    free(val);
                }
            } else {
                Value *r = createValue();
                *r = symbolToValue(sym);
                typeCast(r->ty, val);
                createStore(_fnc, r, val);
                free(val);
                free(r);
            }
        } else {
            if (isConst) sy_error("const 必须赋值");
        }
    _go:
        if (match(TOK_SemiColon)) {
            return true;
        }
        if (match(TOK_Comma)) {
            goto _var;
        }
    }
}

// 分析基本类型
bool parseBType(Type *ty) {
    if (match(TOK_INT)) {
        *ty |= TY_INT;
        return true;
    } else if (match(TOK_FLOAT)) {
        *ty |= TY_FLOAT;
        return true;
    } else if (match(TOK_VOID)) {
        *ty |= TY_VOID;
        return true;
    } else {
        return false;
    }
}
//&& ||
BasicBlock *successBB, *failBB;
BasicBlock *whileExpr(BasicBlock *nextStmt) {
    BasicBlock *whileBB = createBasicBlock(_fnc);
    whileBB->isLoop = 1;
    BasicBlock *bodyBBEnter = createBasicBlock(_fnc);
    BasicBlock *endBB = createBasicBlock(_fnc);

    _fnc->currentBB->nextblock = whileBB;
    setCurrentBB(_fnc, whileBB);

    successBB = bodyBBEnter;
    failBB = endBB;

    consume(TOK_ParL, "expected '('");
    Value *result = condExpr();
    consume(TOK_ParR, "Expected a ')' after the expression.");

    //&& ||已经不需要值了
    if (!(result->ty & TY_VOID)) {
        *result = NotEqualToZero(result);
        createIF(_fnc, result);
        free(result);
    }

    // 因为&& ||有可能生成新的基本块,所以以当前正在插入的基本块为主
    BasicBlock *currentWhileEnter = _fnc->currentBB;
    currentWhileEnter->nextblock = bodyBBEnter;
    currentWhileEnter->branchblock = endBB;

    setCurrentBB(_fnc, bodyBBEnter);
    stmt(endBB, whileBB, nextStmt);
    _fnc->currentBB->nextblock = whileBB;

    if (!_fnc->currentBB->nextblock) {
        assert(0);
    }
    setCurrentBB(_fnc, endBB);
    return endBB;
}
//'if' '( Cond ')' Stmt [ 'else' Stmt ]
void ifExpr(BasicBlock *b, BasicBlock *c, BasicBlock *nextStmt) {
    BasicBlock *thenBB = createBasicBlock(_fnc);
    BasicBlock *nextIfEnter = createBasicBlock(_fnc);
    successBB = thenBB;
    failBB = nextIfEnter;

    consume(TOK_ParL, "expected '('");
    Value *result = condExpr();
    consume(TOK_ParR, "Expected a ')' after the expression.");

    //&& ||已经不需要值了
    if (!(result->ty & TY_VOID)) {
        *result = NotEqualToZero(result);
        createIF(_fnc, result);
        free(result);
    }
    BasicBlock *currentIfEnter = _fnc->currentBB;

    if (!nextStmt) {
        nextStmt = createBasicBlock(_fnc);
    }

    // if Enter
    // 在&&||这里已经设置了next和分支所以不用再次设置
    if (!currentIfEnter->nextblock) currentIfEnter->nextblock = thenBB;

    // thenBB
    setCurrentBB(_fnc, thenBB);
    stmt(b, c, nextStmt);

    // 基本快可能会被break或者continue改写控制流
    if (!_fnc->currentBB->nextblock) {
        _fnc->currentBB->nextblock = nextStmt;
    }
    // 在&&||这里已经设置了next和分支所以不用再次设置
    if (!currentIfEnter->branchblock) currentIfEnter->branchblock = nextIfEnter;
    if (match(TOK_ELSE)) {
        setCurrentBB(_fnc, nextIfEnter);

        // TODO这里不可以是NULL,继承上一级
        if (match(TOK_IF)) {
            ifExpr(b, c, nextStmt);
        } else {
            stmt(b, c, nextStmt);
        }
        // if (_fnc->currentBB->nextblock == NULL) {
        if (!_fnc->currentBB->nextblock)
            _fnc->currentBB->nextblock = nextStmt;
        else {
            if (_fnc->currentBB->nextblock == _fnc->currentBB) {
                _fnc->currentBB->nextblock = nextStmt;
            }
        }
        // }
        // assert(!_fnc->currentBB->nextblock);

    } else {  // if语句(递归)结束
        // if Enter
        // 没有else就增加一个
        nextIfEnter->nextblock = nextStmt;
        setCurrentBB(_fnc, nextStmt);
    }

    if (_fnc->currentBB != nextStmt && !_fnc->currentBB->nextblock) {
        _fnc->currentBB->nextblock = nextStmt;
    }
    setCurrentBB(_fnc, nextStmt);
}
/*
Stmt          → LVal '=' Exp ';' | [Exp] ';'  | Block  | 'if' '( Cond ')' Stmt [
'else' Stmt ]  | 'while' '(' Cond ')' Stmt  | 'break' ';'  | 'continue' ';'  |
'return' [Exp] ';'
 */
static void stmt(BasicBlock *b, BasicBlock *c, BasicBlock *n) {
    if (match(TOK_IF)) {
        ifExpr(b, c, NULL);
    } else if (match(TOK_WHILE)) {
        whileExpr(n);
    } else if (match(TOK_RETURN)) {
        if (match(TOK_SemiColon)) {
            createRet(_fnc);
            return;
        }
        if (_fnc->fncTy.ret & BTYPE_MASK == TY_VOID) {
            sy_error("无返回值");
        }

        Value *result = Expr();
        typeCast(_fnc->fncTy.ret, result);
        if (ValueIsFloat(result)) {
            createRetF(_fnc, result);
        } else {
            createRetI(_fnc, result);
        }
        free(result);
        consume(TOK_SemiColon, "expected ';'");
    } else if (curTok == TOK_CurlyL) {
        block(b, c, n);
    } else if (match(TOK_BREAK)) {
        // 不在循环语句中
        if (b == NULL) {
            // TODO
            sy_error("不在循环中");
        }
        consume(TOK_SemiColon, "expected ';'");
        _fnc->currentBB->nextblock = b;
    } else if (match(TOK_CONTINUE)) {
        // 不在循环语句中
        if (c == NULL) {
            // TODO
            sy_error("不在循环中");
        }
        consume(TOK_SemiColon, "expected ';'");
        _fnc->currentBB->nextblock = c;
    } else if (match(TOK_SemiColon)) {
        // skip ;
    } else {
        free(Expr());
        consume(TOK_SemiColon, "expected ';'");
    }
}

// Block → '{' { BlockItem } '}',新的作用域
static void block(BasicBlock *b, BasicBlock *c, BasicBlock *n) {
    consume(TOK_CurlyL, "缺少");

    scope++;
    Symbol *tmp = localStack;
    // TODO 作用域symbol出栈
    while (!match(TOK_CurlyR)) {
        if (decl()) continue;
        if (match(TOK_CurlyR)) break;
        stmt(b, c, n);
    }
    scope--;
    popSymbol(tmp);
}

static bool   isConstExpr = false;
static Value *constExpr() {
    isConstExpr = true;
    AddExp();
    isConstExpr = false;
}

/**
 * @brief 执行语法分析语义分析中间代码生成
 */
void parser() {
    next();
    while (curTok != TOK_EOF) {
        decl();
    }
}

/**
 * @brief 编译一个源文件
 * @param[in] *fileName
 */
void compileFile(char *fileName) {
    yyin = fopen(fileName, "r");
    if (yyin == NULL) {
        sy_error("failed to open file '%s'", fileName);
    }
    parser();
}

static Value call() {
    consume(TOK_ParL, "Expected a ')' after the expression.");
}

// 将类型转换为ty
void typeCast(Type ty, Value *v) {
    if ((ty & BTYPE_MASK) == (v->ty & BTYPE_MASK)) {
        return;
    }
    if ((ty & BTYPE_MASK) == TY_INT) {
        if (ValueIsImm(v)) {
            *v = SET_IMM_INT((int)(v->imm.f));
        } else {
            *v = createF2I(_fnc, v);
        }

    } else if ((ty & BTYPE_MASK) == TY_FLOAT) {
        if (ValueIsImm(v)) {
            *v = SET_IMM_FLOAT((float)(v->imm.i));
        } else
            *v = createI2F(_fnc, v);
    } else {
        assert(0);
    }
}

/**
 * @brief 类型隐式转换
 * @param[in] *r
 * @param[in] *l
 */
Type implictionCast(Value *l, Value *r) {
    Type t = 0;
    assert(r && l);
    if (r->ty & ~(IS_COSNT | IS_IMM | IS_VAR | IS_GLOBAL | BTYPE_MASK)) {
        // TODO 类型错误
        sy_error("类型错误");
    }
    if (l->ty & ~(IS_COSNT | IS_IMM | IS_VAR | IS_GLOBAL | BTYPE_MASK)) {
        // TODO 类型错误
        sy_error("类型错误");
    }

    if ((l->ty & BTYPE_MASK) == (r->ty & BTYPE_MASK)) {
        return l->ty;
    }

    if (ValueIsFloat(r) || ValueIsFloat(l)) {
        if (ValueIsInt(r)) {
            if (ValueIsImm(r)) {
                *r = SET_IMM_FLOAT((float)r->imm.i);
            } else
                *r = createI2F(_fnc, r);
        } else if (ValueIsInt(l)) {
            if (ValueIsImm(l)) {
                *l = SET_IMM_FLOAT((float)l->imm.i);
            } else
                *l = createI2F(_fnc, l);
        }
        t = TY_FLOAT;
        return t;
    };

    assert(0);
    return t;
}

// Exp → AddExp
static Value *Expr() { return AddExp(); }
// AddExp → MulExp | AddExp ('+' | '−') MulExp
static Value *AddExp() {
    Value *l = MulExp();

    while (curTok == TOK_Add || curTok == TOK_Sub) {
        Token t = curTok;
        next();
        Value *r = MulExp();
        // 此时l和r的类型是一样的
        implictionCast(l, r);

        if (ValueIsFloat(l)) {
            if (t == TOK_Add) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    *l = SET_IMM_FLOAT(l->imm.f + r->imm.f);
                } else
                    *l = createAddF(_fnc, l, r);
            } else if (t == TOK_Sub) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    *l = SET_IMM_FLOAT(l->imm.f - r->imm.f);
                } else
                    *l = createSubF(_fnc, l, r);
            }
        } else {
            if (t == TOK_Add) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    *l = SET_IMM_INT(l->imm.i + r->imm.i);
                } else
                    *l = createAdd(_fnc, l, r);
            } else if (t == TOK_Sub) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    *l = SET_IMM_INT(l->imm.i - r->imm.i);
                } else
                    *l = createSub(_fnc, l, r);
            }
        }
        free(r);
    }
    return l;
}

// → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
static Value *MulExp() {
    Value *l = UnaryExp();
    while (curTok == TOK_Mod || curTok == TOK_Mul || curTok == TOK_Div) {
        Token t = curTok;
        next();

        Value *r = UnaryExp();
        implictionCast(l, r);
        if (ValueIsFloat(l)) {
            if (t == TOK_Mod) {
                assert(0);
                *l = createMod(_fnc, l, r);
            } else if (t == TOK_Mul) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    *l = SET_IMM_FLOAT(l->imm.f * r->imm.f);
                } else
                    *l = createMulF(_fnc, l, r);
            } else if (t == TOK_Div) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    assert(r->imm.f != 0.);
                    *l = SET_IMM_FLOAT(l->imm.f / r->imm.f);
                } else
                    *l = createDivF(_fnc, l, r);
            }
        } else {
            if (t == TOK_Mod) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    *l = SET_IMM_INT(l->imm.i % r->imm.i);
                } else
                    *l = createMod(_fnc, l, r);

            } else if (t == TOK_Mul) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    *l = SET_IMM_INT(l->imm.i * r->imm.i);
                } else
                    *l = createMul(_fnc, l, r);
            } else if (t == TOK_Div) {
                if (ValueIsImm(l) && ValueIsImm(r)) {
                    assert(r->imm.i != 0);
                    *l = SET_IMM_INT(l->imm.i / r->imm.i);
                } else
                    *l = createDiv(_fnc, l, r);
            }
        }
        free(r);
    }
    return l;
}

// UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')'| UnaryOp UnaryExp
static Value *UnaryExp() {
    Symbol  *sym = NULL;
    uint32_t ident;
    Value   *r;
    bool     i;
    switch (curTok) {
        // case TOK_Add:
        //     next();
        //     i = true;
        //     while (match(TOK_Add)) {
        //         i = !i;
        //     }
        //     r = UnaryExp();
        //     if (i) {
        //         if (ValueIsFloat(r)) {
        //             *r = createPosF(_fnc, r);
        //         } else {
        //             *r = createPos(_fnc, r);
        //         }
        //     }
        //     return r;
        // C语言中+不做处理
        case TOK_Add:
            next();
            return UnaryExp();
        case TOK_Sub:
            next();
            i = true;
            while (match(TOK_Sub)) {
                i = !i;
            }
            r = UnaryExp();
            if (i) {
                if (ValueIsImm(r)) {
                    if (ValueIsFloat(r)) {
                        r->imm.f = -r->imm.f;
                    } else {
                        r->imm.i = -r->imm.i;
                    }
                } else {
                    if (ValueIsFloat(r)) {
                        *r = createNegF(_fnc, r);
                    } else {
                        *r = createNeg(_fnc, r);
                    }
                }
            }
            return r;
        case TOK_NOT:
            next();
            i = true;
            while (match(TOK_NOT)) {
                i = !i;
            }
            r = UnaryExp();
            if (i) {
                if (ValueIsFloat(r)) {
                    *r = createNotF(_fnc, r);
                } else {
                    *r = createNot(_fnc, r);
                }
            }
            return r;
            // PrimaryExp → '(' Exp ')' |
        case TOK_ParL:
            next();
            r = Expr();
            consume(TOK_ParR, "Expected a ')' after the expression.");
            return r;
        // Number → IntConst | floatConst
        case TOK_IMM_FLOAT:
            r = createValue();
            *r = SET_IMM_FLOAT(immFloat);
            next();
            return r;
        case TOK_IMM_INT:
            r = createValue();
            *r = SET_IMM_INT(immInt);
            next();
            return r;
        case TOK_STRING:
            r = createValue();
            r->ty = TY_VOID | IS_POINTER;
            r->imm.str = CurString;
            next();
            return r;
        default:
            ident = curId;
            consume(TOK_IDENT, "Expression syntax error");  // 表达式错误
            sym = symbolSearch(ident);
            if (!sym) {
                // 未定义
                sy_error("Undefined symbol %s", getToken(ident)->str);
            }
            r = createValue();
            *r = symbolToValue(sym);

            if (TOK_ParL == curTok) {
                next();
                if (sym == tok_starttime) {
                    match(TOK_ParR);
                    *r = symbolToValue(tok_sysy_starttime);
                    Value        line = SET_IMM_INT(yylineno);
                    instruction *ptr = createParam(_fnc, &line);
                    if (!isOptimimze) {
                        vector_push_back(&_fnc->currentBB->inst, ptr);
                    } else {
                        DL_APPEND(_fnc->currentBB->instlist, ptr);
                    }
                    *r = createCall(_fnc, r);
                    return r;
                } else if (sym == tok_stoptime) {
                    match(TOK_ParR);
                    *r = symbolToValue(tok_sysy_stoptime);
                    Value        line = SET_IMM_INT(yylineno);
                    instruction *ptr = createParam(_fnc, &line);
                    if (!isOptimimze) {
                        vector_push_back(&_fnc->currentBB->inst, ptr);
                    } else {
                        DL_APPEND(_fnc->currentBB->instlist, ptr);
                    }
                    *r = createCall(_fnc, r);
                    return r;
                }

                if (match(TOK_ParR)) {
                    *r = createCall(_fnc, r);
                    return r;
                };
                vector_template(instruction *, _params);
                vector_init(&_params);
                int pi = 0;
                while (!match(TOK_ParR)) {
                    Value *tmp = Expr();
                    if (!(tmp->ty & IS_POINTER) &&
                        !(vector_get(&sym->fnc->fncTy.params, pi)->type &
                          IS_POINTER)) {
                        if (strcmp(sym->fnc->name, "printf")) {
                            typeCast(
                                vector_get(&sym->fnc->fncTy.params, pi)->type &
                                    BTYPE_MASK,
                                tmp);
                        }
                    }
                    vector_push_back(&_params, createParam(_fnc, tmp));
                    free(tmp);
                    match(TOK_Comma);
                    pi++;
                }
                instruction *ptr;
                vector_each3(&_params, ptr) {
                    // vector_push_back(&_fnc->currentBB->inst, ptr);
                    if (!isOptimimze) {
                        vector_push_back(&_fnc->currentBB->inst, ptr);
                    } else {
                        DL_APPEND(_fnc->currentBB->instlist, ptr);
                    }
                }
                vector_free(&_params);

                *r = createCall(_fnc, r);
                return r;
            } else if (curTok == TOK_SqurL) {
                Value       *v = r;
                instruction *ptr = createGetitemptr(_fnc, v);
                ArrayType   *tempArray = sym->arrayTy;
                while (match(TOK_SqurL)) {
                    tempArray = tempArray->next;
                    Value *tmp = Expr();
                    // *v = createGetPtr(_fnc, v, tmp);
                    addArrayIndex(ptr, tmp);
                    free(tmp);
                    match(TOK_SqurR);
                }
                // vector_push_back(&_fnc->currentBB->inst, ptr);
                if (!isOptimimze) {
                    vector_push_back(&_fnc->currentBB->inst, ptr);
                } else {
                    DL_APPEND(_fnc->currentBB->instlist, ptr);
                }

                if (match(TOK_Assign)) {
                    if (sym->type & IS_COSNT) sy_error("const");
                    Value *val = Expr();
                    typeCast(ptr->ret.ty, val);
                    createStore(_fnc, &ptr->ret, val);
                    return val;
                } else {
                    // int a[12][12];a[10]
                    if (tempArray) {
                        *v = ptr->ret;
                    } else {
                        *v = createLoad(_fnc, &ptr->ret);
                    }

                    return v;
                }

                *v = ptr->ret;
                return v;
            } else {
                if (match(TOK_Assign)) {
                    // TODO
                    if (sym->type & IS_COSNT) sy_error("const");
                    Value *val = Expr();
                    typeCast(r->ty, val);
                    // if (ValueIsFloat(result)) {
                    //     createRetF(_fnc, result);
                    // } else {
                    //     createRetI(_fnc, result);
                    // }
                    createStore(_fnc, r, val);
                    return val;
                }
                if (sym->type & IS_COSNT) {
                    if (sym->type & TY_INT) {
                        *r = SET_IMM_INT(sym->i);
                    } else {
                        *r = SET_IMM_FLOAT(sym->f);
                    }
                    return r;
                }
                if (r->ty & IS_GLOBAL) {
                    *r = createLoad(_fnc, r);

                    return r;
                }
                return r;
            }
    }
}

static Value NotEqualToZero(Value *v) {
    if (ValueIsFloat(v)) {
        Value zero = SET_IMM_FLOAT(0);
        return createCmpEqF(_fnc, v, &zero);
    } else if (ValueIsInt(v)) {
        Value zero = SET_IMM_INT(0);
        return createCmpNe(_fnc, v, &zero);
    } else {
        assert(0);
    }
    assert(0);
}

// Cond → LOrExp
static Value *condExpr() { return LOrExp(); }
// 逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp
BasicBlock   *prevAnd;
bool          prevOR;
vector_template(BasicBlock *, andBB);
static Value *LOrExp() {
    prevOR = false;
    vector_clear(&andBB);

    bool iscond = false;
    prevAnd = NULL;
    Value *l = LAndExp();
    if (curTok == TOK_OR) {
        prevOR = true;
        iscond = true;
        if (!(l->ty & TY_VOID)) {
            Value v = NotEqualToZero(l);
            createIF(_fnc, &v);
        }
        _fnc->currentBB->nextblock = successBB;
        BasicBlock *bb = createBasicBlock(_fnc);
        _fnc->currentBB->branchblock = bb;
        BasicBlock *ptr;
        vector_each3(&andBB, ptr) { ptr->branchblock = bb; }
        vector_clear(&andBB);
        setCurrentBB(_fnc, bb);
        if (prevAnd) {
            prevAnd->branchblock = bb;
            prevAnd = NULL;
        }
    }
    while (match(TOK_OR)) {
        BasicBlock *curbb = _fnc->currentBB;

        Value      *l = LAndExp();
        if (!(l->ty & TY_VOID)) {
            Value v = NotEqualToZero(l);
            createIF(_fnc, &v);
        }
        free(l);
        if (curTok == TOK_OR) {
            if (!curbb->nextblock) curbb->nextblock = successBB;
            BasicBlock *bb = createBasicBlock(_fnc);
            BasicBlock *ptr;
            vector_each3(&andBB, ptr) { ptr->branchblock = bb; }
            vector_clear(&andBB);

            _fnc->currentBB->branchblock = bb;
            setCurrentBB(_fnc, bb);
        } else {
            if (!curbb->nextblock) curbb->nextblock = successBB;
            if (!curbb->branchblock) curbb->branchblock = failBB;
        }
    }
    vector_clear(&andBB);
    if (iscond) l->ty = TY_VOID;
    return l;
}

// 逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp
static Value *LAndExp() {
    bool   iscond = false;
    Value *l = EqExp();
    if (curTok == TOK_AND) {
        iscond = true;
        // cond
        if (!(l->ty & TY_VOID)) {
            Value v = NotEqualToZero(l);
            createIF(_fnc, &v);
        }
        _fnc->currentBB->branchblock = failBB;
        prevAnd = _fnc->currentBB;
        vector_push_back(&andBB, _fnc->currentBB);
        BasicBlock *bb = createBasicBlock(_fnc);
        vector_push_back(&andBB, bb);
        _fnc->currentBB->nextblock = bb;
        setCurrentBB(_fnc, bb);
    } else {
        prevAnd = NULL;
    }
    while (match(TOK_AND)) {
        Value *l = EqExp();
        // cond
        if (!(l->ty & TY_VOID)) {
            Value v = NotEqualToZero(l);
            createIF(_fnc, &v);
        }
        free(l);
        _fnc->currentBB->branchblock = failBB;
        if (curTok == TOK_AND) {
            BasicBlock *bb = createBasicBlock(_fnc);
            vector_push_back(&andBB, bb);
            _fnc->currentBB->nextblock = bb;
            setCurrentBB(_fnc, bb);
        } else {
            _fnc->currentBB->nextblock = successBB;
        }
    }
    if (iscond) l->ty = TY_VOID;
    return l;
}
// 相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
static Value *EqExp() {
    bool   iscond = false;
    Value *l = RelExp();
    Token  t = curTok;
    while (match(TOK_Equal) || match(TOK_NotEq)) {
        iscond = true;
        Value *r = RelExp();
        // 此时l和r的类型是一样的
        if (!(l->ty & TY_VOID || r->ty & TY_VOID)) {
            implictionCast(l, r);
        } else {
            if ((l->ty & TY_FLOAT || r->ty & TY_FLOAT)) assert(0);
            // 默认是int
            l->ty |= TY_INT;
            r->ty |= TY_INT;
        }
        if (l->ty & IS_IMM && r->ty & IS_IMM) {
            if (ValueIsFloat(l)) {
                if (t == TOK_Equal) {
                    *l = SET_IMM_INT(l->imm.f == r->imm.f);
                } else {
                    *l = SET_IMM_INT(l->imm.f != r->imm.f);
                }
            } else {
                if (t == TOK_Equal) {
                    *l = SET_IMM_INT(l->imm.i == r->imm.i);
                } else {
                    *l = SET_IMM_INT(l->imm.i != r->imm.i);
                }
            }
        } else {
            if (ValueIsFloat(l)) {
                if (t == TOK_Equal) {
                    *l = createCmpEqF(_fnc, l, r);
                } else {
                    *l = createCmpNeF(_fnc, l, r);
                }
            } else {
                if (t == TOK_Equal) {
                    *l = createCmpEq(_fnc, l, r);
                } else {
                    *l = createCmpNe(_fnc, l, r);
                }
            }
        }
        t = curTok;
    }
    if (l->ty & IS_IMM) return l;
    if (iscond) l->ty = TY_VOID;
    return l;
}

Value generateBinaryIR(Token t, Value *l, Value *r) {
    assert((l->ty & BTYPE_MASK) == (r->ty & BTYPE_MASK));
    if (l->ty & IS_IMM && r->ty & IS_IMM) {
        if (l->ty & TY_FLOAT) {
            switch (t) {
                case TOK_Less:
                    return SET_IMM_INT(l->imm.f < r->imm.f);
                case TOK_LesEq:
                    //<=
                    return SET_IMM_INT(l->imm.f <= r->imm.f);
                case TOK_Greater:
                    return SET_IMM_INT(l->imm.f > r->imm.f);
                case TOK_GreEq:
                    return SET_IMM_INT(l->imm.f >= r->imm.f);
                default:
                    assert(0);
            }
        } else if (l->ty & TY_INT) {
            switch (t) {
                case TOK_Less:
                    return SET_IMM_INT(l->imm.i < r->imm.i);
                case TOK_LesEq:
                    return SET_IMM_INT(l->imm.i <= r->imm.i);
                case TOK_Greater:
                    return SET_IMM_INT(l->imm.i > r->imm.i);
                case TOK_GreEq:
                    return SET_IMM_INT(l->imm.i >= r->imm.i);
                default:
                    assert(0);
            }
        }
    } else {
        if (l->ty & TY_FLOAT) {
            switch (t) {
                case TOK_Less:
                    return createCmpLeF(_fnc, l, r);
                case TOK_LesEq:
                    return createCmpLtF(_fnc, l, r);
                case TOK_Greater:
                    return createCmpGeF(_fnc, l, r);
                case TOK_GreEq:
                    return createCmpGtF(_fnc, l, r);
                default:
                    assert(0);
            }
        } else if (l->ty & TY_INT) {
            switch (t) {
                case TOK_Less:
                    return createCmpLe(_fnc, l, r);
                case TOK_LesEq:
                    return createCmpLt(_fnc, l, r);
                case TOK_Greater:
                    return createCmpGe(_fnc, l, r);
                case TOK_GreEq:
                    return createCmpGt(_fnc, l, r);
                default:
                    assert(0);
            }
        }
    }

    assert(0);
}
// RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
static Value *RelExp() {
    bool   iscond = false;
    Value *l = AddExp();
    Token  tmp = curTok;
    while (match(TOK_LesEq) || match(TOK_Less) || match(TOK_Greater) ||
           match(TOK_GreEq)) {
        iscond = true;
        // TODO 类型转换
        Value *r = AddExp();

        implictionCast(l, r);

        *l = generateBinaryIR(tmp, l, r);
        tmp = curTok;
    }
    if (l->ty & IS_IMM) return l;
    if (iscond) l->ty = TY_VOID;
    return l;
}

// 左值表达式 LVal → Ident{'[' Exp ']'}
// 基本表达式 PrimaryExp → '(' Exp ')' |LVal | Number
