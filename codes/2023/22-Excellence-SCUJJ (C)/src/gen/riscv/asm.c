
#include "asm.h"

#include <assert.h>
#include <stdio.h>

#include "../../include/bitset.h"
#include "../../include/function.h"
#include "../../include/ir.h"
#include "../../include/lexer.h"
#include "../../include/pass.h"
#include "../../include/stack.h"
#include "../../include/symbol.h"
#include "../../include/util.h"
#include "../../include/value.h"
#include "riscv.h"

#define _X(op) #op,
char           *_asmstr[] = {_SY_ASM};
AsmState        getAsmState(riscvAsm asmop);
asminstruction *createAsmInst() { return sy_malloc(sizeof(asminstruction)); }
asminstruction *createAsm13(void *i) {
    asminstruction *ins = createAsmInst();
    ins->op = ASM_debug;
    ins->i = i;
    return ins;
}
Symbol     *getSymbol(Function *fnc, int32_t ind);

static void printValue(FILE *fp, Function *fnc, Value *v) {
    if (ValueIsImm(v)) {
        if (v->ty & TY_FLOAT)
            fprintf(fp, "%g", getImmFloatValue(v));
        else if (v->ty & TY_INT)
            fprintf(fp, "%I32d", getImmIntValue(v));
        else
            assert(0);
    } else if (v->ty & IS_VAR) {
        if (v->ty & IS_SSA) {
            ssaSymbol *sym = getSSAlVariable(fnc, v->ssaind);
            if (!sym->sym) {
                fprintf(fp, "_%d", sym->valueind);
            } else if (sym->sym->isTemp) {
                fprintf(fp, "_%d", sym->sym->tmpInd);
            } else {
                fprintf(fp, "%s_%d", getToken(sym->sym->tok)->str,
                        sym->valueind);
            }
            fprintf(fp, "(%d)", sym->ssaind);
        } else {
            NULL;
            Symbol *sym = NULL;
            if (v->ty & IS_GLOBAL) {
                sym = getGlobalSymbol(v->symbol);
            } else {
                sym = getSymbol(fnc, v->symbol);
            }
            if (sym->isTemp) {
                fprintf(fp, "_%d", sym->tmpInd);
            } else {
                fprintf(fp, "%s", getToken(sym->tok)->str);
            }
        }
    } else if (v->ty & TY_VOID && v->ty & IS_POINTER) {
        fprintf(fp, "\"%s\"", v->imm.str);
    } else if (v->ty & IS_GLOBAL) {
        Symbol *sym = getGlobalSymbol(v->symbol);
        fprintf(fp, "%s", getToken(sym->tok)->str);
    } else if (v->ty & TY_VOID) {
        fprintf(fp, "TY_VOID");
    } else
        assert(0);
}

static void optprintIns(FILE *fp, Function *fnc, instruction *inst) {
    fprintf(fp, "#");
    if (fnc->instNumbering) {
        fprintf(fp, "%-5d%-15s", inst->n, op_3AC_name[inst->op]);
    } else
        fprintf(fp, "%-20s", op_3AC_name[inst->op]);
    if (inst->op == IR_OP_PHI) {
        phiFunction *phi = inst->args[0].phi;
        phiParam     p;
        size_t       i;
        printValue(fp, fnc, &inst->ret);
        fputs("=\t", fp);
        vector_each(&phi->param, i, p) {
            printValue(fp, fnc, &p.v);
            fputs("\t", fp);
        }
        fputs("\n", fp);
        return;
    } else if (inst->op == IR_OP_GETPTR) {
        printValue(fp, fnc, &inst->ret);
        fputs("\t", fp);
        printValue(fp, fnc, &inst->args[0]);
        fputs("\t", fp);
        printValue(fp, fnc, &inst->args[1]);
        fputs("\t", fp);
        fputs("\n", fp);
        return;
    } else if (inst->op == IR_OP_ADDPTR) {
        printValue(fp, fnc, &inst->ret);
        fputs("\t", fp);
        printValue(fp, fnc, &inst->args[0]);
        fputs("\t", fp);
        printValue(fp, fnc, &inst->args[1]);
        fputs("\t", fp);
        fputs("\n", fp);
        return;
    } else if (inst->op == IR_OP_GETITEMPTR) {
        printValue(fp, fnc, &inst->ret);
        fputs(" = ", fp);

        arrayAddr *ptr = inst->args[1].arrayAddr;
        Value     *index;
        size_t     i;
        printValue(fp, fnc, &inst->args[0]);
        vector_each_address(&ptr->index, i, index) {
            fputs("[", fp);
            printValue(fp, fnc, index);
            fputs("]", fp);
        }
        fputs("\n", fp);
        return;
    }

    CASEINST(
        {
            printValue(fp, fnc, &inst->args[0]);
            fputs("\t", fp);
        },
        {
            printValue(fp, fnc, &inst->args[1]);
            fputs("\t", fp);
        },
        {
            printValue(fp, fnc, &inst->ret);
            fputs("\t", fp);
        });
}
Function       *debugFnc;
asminstruction *outputAsm13(FILE *fp, asminstruction *ins) {
    optprintIns(fp, debugFnc, ins->i);
    fprintf(fp, "\n");
    fprintf(fp, ".%s_%d:\n", debugFnc->name, ((instruction *)ins->i)->n);
}
asminstruction *createAsm12(riscvAsm asmop, int r0, int r1, int r2) {
    assert(getAsmState(asmop) == ASM_12);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].reg = r1;
    ins->src[1].reg = r2;
    return ins;
}

asminstruction *outputAsm12(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_12);
    if (ins->op == ASM_fne) {
        fprintf(fp, "feq.s ");
    } else {
        fprintf(fp, "%s.s ", _asmstr[ins->op]);
    }
    fprintf(fp, "%s,", regName[ins->dest]);
    fprintf(fp, "%s,", regName[ins->src[0].reg]);
    fprintf(fp, "%s", regName[ins->src[1].reg]);
    fprintf(fp, "\n");
}
asminstruction *createAsm11(riscvAsm asmop, int r0, int r1) {
    assert(getAsmState(asmop) == ASM_11);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].reg = r1;
    return ins;
}
asminstruction *outputAsm11(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_11);
    char *str;
    if (ins->op == ASM_fcvtsw) {
        str = "fcvt.s.w ";
    } else if (ins->op == ASM_fcvtws) {
        str = "fcvt.w.s ";
    } else if (ins->op == ASM_fcvtds) {
        str = "fcvt.d.s ";
    } else if (ins->op == ASM_fmv) {
        str = "fmv.x.d ";
    } else if (ins->op == ASM_fneg) {
        str = "fneg.s  ";
    } else if (ins->op == ASM_fmvs) {
        str = "fmv.s  ";
    } else {
        assert(0);
    }
    // fcvt.w.s a5, fa5, rtz
    if (ins->op == ASM_fcvtws) {
        fprintf(fp, "%s ", str);
        fprintf(fp, "%s,", regName[ins->dest]);
        fprintf(fp, "%s", regName[ins->src[0].reg]);
        fprintf(fp, ", rtz");
        fprintf(fp, "\n");
        fprintf(fp, "sext.w %s,%s\n", regName[ins->dest], regName[ins->dest]);
    } else {
        fprintf(fp, "%s ", str);
        fprintf(fp, "%s,", regName[ins->dest]);
        fprintf(fp, "%s", regName[ins->src[0].reg]);
        fprintf(fp, "\n");
    }
}
asminstruction *createAsm10(riscvAsm asmop) {
    assert(getAsmState(asmop) == ASM_10);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    return ins;
}
asminstruction *outputAsm10(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_10);
    fprintf(fp, "%s ", _asmstr[ins->op]);
    fprintf(fp, "\n");
}
asminstruction *createAsm9(riscvAsm asmop, char *symbol) {
    assert(getAsmState(asmop) == ASM_9);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->src[0].symbol = symbol;
    return ins;
}
asminstruction *outputAsm9(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_9);
    fprintf(fp, "%s:", ins->src[0].symbol);
    fprintf(fp, "\n");
}
asminstruction *createAsm8(riscvAsm asmop, int r0, char *symbol) {
    assert(getAsmState(asmop) == ASM_8);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].symbol = symbol;
    return ins;
}
// fprintf(fp, "sd      %s, 0(t2)\n\t", regName[r]);
asminstruction *outputAsm8(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_8);
    fprintf(fp, "%s ", _asmstr[ins->op]);
    fprintf(fp, "%s,", regName[ins->dest]);
    fprintf(fp, "%s", ins->src[0].symbol);
    fprintf(fp, "\n");
}

asminstruction *createAsm7(riscvAsm asmop, int r0, int r1, int index) {
    assert(getAsmState(asmop) == ASM_7);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].reg = r1;
    ins->src[0].index = index;
    return ins;
}
// fprintf(fp, "sd      %s, 0(t2)\n\t", regName[r]);
asminstruction *outputAsm7(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_7);
    fprintf(fp, "%s ", _asmstr[ins->op]);
    fprintf(fp, "%s,", regName[ins->dest]);
    fprintf(fp, "%d", ins->src[0].index);
    fprintf(fp, "(%s)", regName[ins->src[0].reg]);
    fprintf(fp, "\n");
}
// op symbol:la symbol,call symbol
asminstruction *createAsm1(riscvAsm asmop, char *symbol) {
    assert(getAsmState(asmop) == ASM_1);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->src[0].symbol = symbol;
    return ins;
}
asminstruction *outputAsm1(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_1);
    fprintf(fp, "%s ", _asmstr[ins->op]);
    fprintf(fp, "%s ", ins->src[0].symbol);
    fprintf(fp, "\n");
}
// op reg,reg,reg
asminstruction *createAsm2(riscvAsm asmop, int r0, int r1, int r2) {
    assert(getAsmState(asmop) == ASM_2);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].reg = r1;
    ins->src[1].reg = r2;
    return ins;
}
asminstruction *outputAsm2(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_2);
    fprintf(fp, "%s ", _asmstr[ins->op]);
    fprintf(fp, "%s,", regName[ins->dest]);
    fprintf(fp, "%s,", regName[ins->src[0].reg]);
    fprintf(fp, "%s", regName[ins->src[1].reg]);
    fprintf(fp, "\n");
}
// op reg,reg,imm
asminstruction *createAsm3(riscvAsm asmop, int r0, int r1, int imm) {
    assert(getAsmState(asmop) == ASM_3);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].reg = r1;
    ins->src[1].imm = imm;
    return ins;
}
asminstruction *outputAsm3(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_3);
    fprintf(fp, "%s ", _asmstr[ins->op]);
    fprintf(fp, "%s,", regName[ins->dest]);
    fprintf(fp, "%s,", regName[ins->src[0].reg]);
    fprintf(fp, "%d", ins->src[1].imm);
    fprintf(fp, "\n");
}
// op reg,imm :li
asminstruction *createAsm4(riscvAsm asmop, int r0, int imm) {
    assert(getAsmState(asmop) == ASM_4);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].imm = imm;
    return ins;
}
asminstruction *outputAsm4(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_4);
    fprintf(fp, "%s ", _asmstr[ins->op]);
    fprintf(fp, "%s,", regName[ins->dest]);
    fprintf(fp, "%d", ins->src[0].imm);
    fprintf(fp, "\n");
}
// op reg,reg :mv
asminstruction *createAsm5(riscvAsm asmop, int r0, int r1) {
    assert(getAsmState(asmop) == ASM_5);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].reg = r1;
    return ins;
}
asminstruction *outputAsm5(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_5);
    if (ins->op == ASM_sextw) {
        fprintf(fp, "sext.w ", _asmstr[ins->op]);
        fprintf(fp, "%s,", regName[ins->dest]);
        fprintf(fp, "%s", regName[ins->src[0].reg]);
        fprintf(fp, "\n");
    } else {
        fprintf(fp, "%s ", _asmstr[ins->op]);
        fprintf(fp, "%s,", regName[ins->dest]);
        fprintf(fp, "%s", regName[ins->src[0].reg]);
        fprintf(fp, "\n");
    }
}
/**
 * @brief op reg,reg,symbol :bge ble
 * @param[in] asmop
 * @param[in] r0
 * @param[in] r1
 * @param[in] *symbol
 */
asminstruction *createAsm6(riscvAsm asmop, int r0, int r1, char *symbol) {
    assert(getAsmState(asmop) == ASM_6);
    asminstruction *ins = createAsmInst();
    ins->op = asmop;
    ins->dest = r0;
    ins->src[0].reg = r1;
    ins->src[1].symbol = symbol;
    return ins;
}
asminstruction *outputAsm6(FILE *fp, asminstruction *ins) {
    assert(getAsmState(ins->op) == ASM_6);
    fprintf(fp, "%s ", _asmstr[ins->op]);
    fprintf(fp, "%s,", regName[ins->dest]);
    fprintf(fp, "%s,", regName[ins->src[0].reg]);
    fprintf(fp, "%s", ins->src[1].symbol);
    fprintf(fp, "\n");
}

void outputAsmInst(FILE *fp, asminstruction *ins) {
    // AsmState t = getAsmState(ins);
}
AsmState getAsmState(riscvAsm asmop) {
    switch (asmop) {
        case ASM_call:
        case ASM_J:
            return ASM_1;
        case ASM_li:
            return ASM_4;
        case ASM_add:
        case ASM_sub:
        case ASM_mul:
        case ASM_div:
        case ASM_rem:
            return ASM_2;
        case ASM_addi:
        case ASM_subi:
        case ASM_muli:
        case ASM_divi:
            return ASM_3;
        case ASM_mv:
        case ASM_sextw:
        case ASM_seqz:
        case ASM_negw:
            return ASM_5;
        case ASM_beq:
        case ASM_ble:
        case ASM_bne:
        case ASM_blt:
        case ASM_bgt:
        case ASM_bge:
            return ASM_6;
        case ASM_fsw:
        case ASM_flw:
        case ASM_sw:
        case ASM_lw:
        case ASM_sd:
        case ASM_ld:
            return ASM_7;
        case ASM_la:
            return ASM_8;
        case ASM_label:
            return ASM_9;
        case ASM_ret:
            return ASM_10;
        case ASM_fmv:
        case ASM_fcvtds:
        case ASM_fcvtsw:
        case ASM_fcvtws:
        case ASM_fneg:
        case ASM_fmvs:
            return ASM_11;
        case ASM_feq:
        case ASM_fne:
        case ASM_fle:
        case ASM_flt:
        case ASM_fge:
        case ASM_fgt:
        case ASM_fadd:
        case ASM_fsub:
        case ASM_fdiv:
        case ASM_fmul:
            return ASM_12;
        case ASM_debug:
            return ASM_13;
        default:
            assert(0);
    }
}

void outputAllAsmInst(FILE *fp, Function *fnc) {
    asminstruction *head, *ptr, *tmp;
    head = (asminstruction *)fnc->asmlist;
    DL_FOREACH_SAFE(head, ptr, tmp) {
        switch (getAsmState(ptr->op)) {
            case ASM_1:
                outputAsm1(fp, ptr);
                break;
            case ASM_2:
                outputAsm2(fp, ptr);
                break;
            case ASM_3:
                outputAsm3(fp, ptr);
                break;
            case ASM_4:
                outputAsm4(fp, ptr);
                break;
            case ASM_5:
                outputAsm5(fp, ptr);
                break;
            case ASM_6:
                outputAsm6(fp, ptr);
                break;
            case ASM_7:
                outputAsm7(fp, ptr);
                break;
            case ASM_8:
                outputAsm8(fp, ptr);
                break;
            case ASM_9:
                outputAsm9(fp, ptr);
                break;
            case ASM_10:
                outputAsm10(fp, ptr);
                break;
            case ASM_11:
                outputAsm11(fp, ptr);
                break;
            case ASM_12:
                outputAsm12(fp, ptr);
                break;
            case ASM_13:
                outputAsm13(fp, ptr);
                break;
            default:
                assert(0);
        }
        DL_DELETE(head, ptr);
        free(ptr);
    }
}
