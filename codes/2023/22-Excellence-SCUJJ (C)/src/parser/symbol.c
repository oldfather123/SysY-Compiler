// #include <lexer.h>
// #include <stack.h>
// #include <stdint.h>
// #include <symbol.h>
// #include <sysy.h>
// #include <util.h>

#include "../include/symbol.h"

#include <stdint.h>

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

vector_template(Symbol *, globalSymbols);

Symbol *globalStack;
Symbol *localStack;
Symbol *getGlobalSymbol(uint32_t ind) {
    return vector_get(&globalSymbols, ind);
}
Symbol *pushSymbol(uint32_t tok, Type *ty) {
    Symbol     *sym = NULL;
    Symbol     *tmp = NULL;

    tokenIdent *t = NULL;
    sym = sy_malloc(sizeof(Symbol));
    t = getToken(tok);

    if (scope == 0) {
        STACK_EACH(globalStack, tmp) {
            if (tmp->tok == tok) {
                sy_error("Redeclaration of %s", t->str);
            }
        }
        STACK_PUSH(globalStack, sym);
        sym->type |= IS_GLOBAL | IS_POINTER;
        sym->ind = vector_len(&globalSymbols);
        vector_push_back(&globalSymbols, sym);
    } else {
        STACK_EACH(localStack, tmp) {
            if (tmp->tok == tok && tmp->scope == scope) {
                sy_error("Redeclaration of %s", t->str);
            }
        }
        STACK_PUSH(localStack, sym);
    }
    sym->tok = tok;
    sym->scope = scope;
    sym->type |= *ty;

    sym->prev = t->sym;
    t->sym = sym;

    return sym;
}

void popSymbol(Symbol *s) {
    Symbol *sym = NULL;
    while (localStack != s) {
        if (!localStack->isTemp) {
            uint32_t    tok = localStack->tok;
            tokenIdent *t = getToken(tok);
            t->sym = localStack->prev;  // 退出作用域时,不同级别的同名标识符
        }
        assert(localStack);
        STACK_POP(localStack, sym);
    }
}

Symbol *symbolSearch(uint32_t tok) { return getToken(tok)->sym; }