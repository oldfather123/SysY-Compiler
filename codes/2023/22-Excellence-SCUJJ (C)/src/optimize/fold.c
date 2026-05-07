#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/lexer.h"
#include "../include/pass.h"
#include "../include/symbol.h"
#include "../include/sysy.h"
#include "../include/util.h"
#include "../include/value.h"
#include "../include/vector.h"

bool foldIntValue(TAC op, Value *l, Value *r, Value *result) {
    assert(l->ty & TY_INT && r->ty & TY_INT);
    int lval, rval, ret;
    lval = getImmIntValue(l);
    rval = getImmIntValue(r);
    switch (op) {
        case IR_OP_ADD:
            ret = lval + rval;
            break;
        case IR_OP_SUB:
            ret = lval - rval;
            break;
        case IR_OP_MUL:
            ret = lval * rval;
            break;
        case IR_OP_DIV:
            ret = lval / rval;
            break;
        case IR_OP_MOD:
            ret = lval % rval;
            break;
        case IR_OP_EQ:
            ret = lval == rval;
            break;
        case IR_OP_NE:
            ret = lval != rval;
            break;
        case IR_OP_GE:
            ret = lval > rval;
            break;
        case IR_OP_GT:
            ret = lval >= rval;
            break;
        case IR_OP_LT:
            ret = lval <= rval;
            break;
        case IR_OP_LE:
            ret = lval < rval;
            break;
        default:
            return false;
    }
    *result = SET_IMM_INT(ret);
    return true;
}
bool foldFloatValue(TAC op, Value *l, Value *r, Value *result) {
    assert(l->ty & TY_FLOAT && r->ty & TY_FLOAT);
    float lval, rval, ret;
    lval = getImmFloatValue(l);
    rval = getImmFloatValue(r);
    switch (op) {
        case IR_OP_ADDF:
            ret = lval + rval;
            break;
        case IR_OP_SUBF:
            ret = lval - rval;
            break;
        case IR_OP_MULF:
            ret = lval * rval;
            break;
        case IR_OP_DIVF:
            ret = lval / rval;
            break;
        case IR_OP_EQF:
            ret = lval == rval;
            break;
        case IR_OP_NEF:
            ret = lval != rval;
            break;
        case IR_OP_GEF:
            ret = lval > rval;
            break;
        case IR_OP_GTF:
            ret = lval >= rval;
            break;
        case IR_OP_LTF:
            ret = lval <= rval;
            break;
        case IR_OP_LEF:
            ret = lval < rval;
            break;
        default:
            return false;
    }
    *result = SET_IMM_FLOAT(ret);
    return true;
}

/**
 * @brief 尝试表达式进行折叠
 * @param[in] op
 * @param[in] *l
 * @param[in] *r
 * @param[in] *result
 */
bool foldValue(TAC op, Value *l, Value *r, Value *result) {
    int   iv, iret;
    float fv, fret;

    if (!(l->ty & IS_IMM && r->ty & IS_IMM)) {
        return false;
    }

    switch (op) {
        case IR_OP_NEG:
            iv = getImmIntValue(l);
            iret = -iv;
            *result = SET_IMM_INT(iret);
            break;
        case IR_OP_NEGF:
            fv = getImmFloatValue(l);
            fret = -fv;
            *result = SET_IMM_FLOAT(fret);
            break;
        case IR_OP_I2F:
            iv = getImmIntValue(l);
            fret = (float)iv;
            *result = SET_IMM_FLOAT(fret);
            break;
        case IR_OP_F2I:
            iv = getImmFloatValue(l);
            iret = (int)fv;
            *result = SET_IMM_INT(iret);
            break;
        case IR_OP_NOT:
            iv = getImmIntValue(l);
            iret = !iv;
            *result = SET_IMM_INT(iret);
            break;
        case IR_OP_NOTF:
            fv = getImmFloatValue(l);
            iret = !fv;
            *result = SET_IMM_INT(iret);
            break;
        default:
            goto _op2;
    }
    return true;
_op2:
    if (l->ty == r->ty && l->ty & TY_INT) {
        return foldIntValue(op, l, r, result);
    } else if (l->ty == r->ty && l->ty & TY_FLOAT) {
        return foldFloatValue(op, l, r, result);
    } else {
        return false;
    }
}