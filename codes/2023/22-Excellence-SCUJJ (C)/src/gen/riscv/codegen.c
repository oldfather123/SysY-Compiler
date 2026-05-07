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
#include "asm.h"
#include "riscv.h"

FILE*  asmFile;
FILE*  asmFile2;
/**
 * @brief 线性寄存器分配
 * @param[in] *fnc
 */
void   linearScanRegisterAllocPass(Function* fnc);

/**
 * @brief 得到符号的大小
 * @param[in] sym
 */
size_t getSymbolSize(Symbol* sym) {
    if (sym->type & IS_ARRAY) {
        return getArraySize2(sym->arrayTy);
    } else {
        return 4;
    }
}

/*
未初始化
        .section	.sbss,"aw",@nobits
        .align	2
        .type	c, @object
        .size	c, 4
c:
        .zero	4
 */
/**
 * @brief 创建一个全局变量
 * @param[in] fp
 * @param[in] sym
 */
void createGlobalVariable(FILE* fp, Symbol* sym) {
    char*  name = getToken(sym->tok)->str;
    size_t size = getSymbolSize(sym) * 2;
    // 测试大小
    fprintf(fp, ".globl	%s\n", name);
    if (sym->isInit) {
        // fprintf(fp, ".section	.data,\" aw \"\n");
        if ((sym->type & IS_COSNT) && (!(sym->type & IS_ARRAY))) {
            fprintf(fp, ".section	.rodata\n");
        } else if ((sym->type & IS_COSNT) && (!(sym->type & IS_ARRAY))) {
            assert(0);
        } else {
            fprintf(fp, ".data\n");
        }
    } else {
        // fprintf(fp, ".section	.bss,\" aw\",@nobits\n");
        fprintf(fp, ".bss\n");
    }
    fprintf(fp, ".align 2\n");
    fprintf(fp, ".type %s,@object\n", name);
    if (sym->type & IS_ARRAY) {
        fprintf(fp, ".size %s,%lld\n", name, size / 2);
    } else {
        fprintf(fp, ".size %s,%lld\n", name, size);
    }

    fprintf(fp, ".%s:\n", name);
    if (sym->isInit) {
        if (sym->type & IS_ARRAY) {
            size_t i;
            for (i = 0; i < size / 4; i++) {
                // fprintf(fp, ".word %d\n", ((int*)sym->arrayVal)[i]);
            }
        } else if (sym->type & TY_FLOAT) {
            fprintf(fp, ".word %d\n", *((int*)&sym->f));
        } else if (sym->type & TY_INT) {
            fprintf(fp, ".word %d\n", sym->i);
        }

    } else {
        // 用于指定一个占据4个字节的空数据项,该数据项的值始终为02。这个伪指令通常用于初始化内存中的数据
        if (sym->type & IS_ARRAY) {
            fprintf(fp, ".zero %lld\n", size / 2);
        } else {
            fprintf(fp, ".zero %lld\n", size);
        }
    }
}

// 将值加载到指定寄存器
void loadReg(Function* fnc, FILE* fp, Reg r, bool isMem, Value* v) {
    if (isMem) {
        processMemoryLoad(fnc, fp, r, v);
        // assert(0);
    } else if (ValueIsImm(v)) {
        loadImm(fnc, fp, r, v);
    } else if (v->ty & IS_SSA) {
        fprintf(fp, "mv    %s, %s\n\t", regName[r], regName[v->reg]);
    } else {
        assert(0);
    }
}
/**
 * @brief 将立即数加载到寄存器中
 * @param[in] *fnc
 * @param[in] *fp
 * @param[in] r
 * @param[in] *v
 */
void loadImm(Function* fnc, FILE* fp, Reg r, Value* v) {
    if (!ValueIsInt(v)) {
        ValueIsInt(v);
    }
    assert(ValueIsInt(v));
    int i = getImmIntValue(v);
    // riscv 支持 12bit 立即数
    // li 伪指令
    fprintf(fp, "li %s,%d\n", regName[r], i);
    // if (i >= -2048 && i <= 2047) {
    //     fprintf(fp, "addi   %s, zero, %d\n", regName[r], v);
    // }else{

    // }
}
extern asminstruction* asmlist;
extern FILE*           asmFile2;
void                   genFunctionAsm(Function* fnc, FILE* fp) {
    fnc->loc1 = 300;
    asmlist = NULL;
    asmgen_functionProlog(fnc, stdout);
    m1111(fnc, asmFile2);
    asmm1111(fnc, stdout);
    fnc->asmlist = asmlist;
}
void             outputAllAsmInst(FILE* fp, Function* fnc);
extern Function* debugFnc;

void             outputFunctionAsm(Function* fnc, FILE* fp) {
    // 没有判断是否是叶子函数
    if (strcmp("main", fnc->name) == 0) {
        fprintf(fp,
                            ".section	"
                                        ".text.startup,\"ax\",@progbits\n");
    } else {
        fprintf(fp, ".text\n");
    }
    fprintf(fp, ".align	1\n");
    fprintf(fp, ".globl %s\n", fnc->name);
    fprintf(fp, ".type %s, @function\n", fnc->name);
    fprintf(fp, "%s :\n", fnc->name);
    debugFnc = fnc;
    outputAllAsmInst(fp, fnc);
}
FILE*       irfp;
int         testNum;

static void functionGenerator(Function* fnc) {
    LOOPALLBB({LOOPBBALLINST({ testNum++; })})

    if (!irfp) {
        irfp = fopen("ir.txt", "w");
    } else {
        irfp = fopen("ir.txt", "a");
    }
    printBasicAllBlock(irfp, fnc);
    fclose(irfp);
    fnc->loc = 540000;
    linearScanRegisterAllocPass(fnc);
    genFunctionAsm(fnc, asmFile);
}
// 用于优化版本
extern asminstruction* asmlist;
void                   optasmgen_functionProlog(Function* fnc, FILE* fp);
void                   optasmgen_functionEpilogue(Function* fnc, FILE* fp);
void                   optasmm1111(Function* fnc, FILE* fp);
void                   optlinearScanRegisterAllocPass(Function* fnc);
void                   peepholeOptimizationPass(Function* fnc);
static void            optGenFunctionAsm(Function* fnc, FILE* fp) {
    fnc->loc1 = 600;
    asmlist = NULL;
    optasmgen_functionProlog(fnc, stdout);
    optasmm1111(fnc, stdout);
    fnc->asmlist = asmlist;
}
void        peepholeOptimizationPass(Function* fnc);
void        TailRecursionPass(Function* fnc);
static void optfunctionGenerator(Function* fnc) {
    TailRecursionPass(fnc);
    optlinearScanRegisterAllocPass(fnc);
    optGenFunctionAsm(fnc, asmFile);
    peepholeOptimizationPass(fnc);
}
void         processCon(FILE* fp);
extern char* outputFile;
/**
 * @brief 将IR编译为riscv汇编代码
 */
void         riscvCodeGenerator() {
    asmFile2 = fopen("2.asm", "w");
    // asmFile = stdout;

    fprintf(asmFile, ".option pic\n");
    // fprintf(asmFile,
    //                 ".attribute arch, "
    //                         "\"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_"
    //                         "zifencei2p0\"\n");
    fprintf(asmFile, ".attribute unaligned_access, 0\n");
    fprintf(asmFile, ".attribute stack_align, 16\n");

    Symbol* symbol = NULL;

    STACK_EACH(globalStack, symbol) {
        if ((symbol->type & IS_FNC) && (!symbol->fnc->isExtern)) {
            functionGenerator(symbol->fnc);
        }
    }
    STACK_EACH(globalStack, symbol) {
        if ((symbol->type & IS_FNC) && (!symbol->fnc->isExtern)) {
            outputFunctionAsm(symbol->fnc, asmFile);
        }
    }

    fprintf(stderr, "asmfnc successful\n");

    STACK_EACH(globalStack, symbol) {
        if (!(symbol->type & IS_FNC)) {
            if (symbol->type & IS_ARRAY && symbol->isInit) continue;
            createGlobalVariable(asmFile, symbol);
        }
    }
    processCon(asmFile);
    processZeroArray(asmFile);
    fclose(asmFile);
    fclose(asmFile2);
    fprintf(stderr, "asm successful");
}
void optRiscvCodeGenerator() {
    asmFile2 = fopen("2.asm", "w");
    // asmFile = stdout;

    fprintf(asmFile, ".option pic\n");
    // fprintf(asmFile,
    //                 ".attribute arch, "
    //                         "\"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_"
    //                         "zifencei2p0\"\n");
    fprintf(asmFile, ".attribute unaligned_access, 0\n");
    fprintf(asmFile, ".attribute stack_align, 16\n");

    Symbol* symbol = NULL;

    STACK_EACH(globalStack, symbol) {
        if ((symbol->type & IS_FNC) && (!symbol->fnc->isExtern)) {
            optfunctionGenerator(symbol->fnc);
        }
    }
    STACK_EACH(globalStack, symbol) {
        if ((symbol->type & IS_FNC) && (!symbol->fnc->isExtern)) {
            outputFunctionAsm(symbol->fnc, asmFile);
        }
    }

    fprintf(stderr, "asmfnc successful\n");

    STACK_EACH(globalStack, symbol) {
        if (!(symbol->type & IS_FNC)) {
            if (symbol->type & IS_ARRAY && symbol->isInit) continue;
            createGlobalVariable(asmFile, symbol);
        }
    }

    // int a[10000] = {};
    STACK_EACH(globalStack, symbol) {
        if (!(symbol->type & IS_FNC)) {
            if (symbol->type & IS_ARRAY && symbol->isInit && symbol->iszero) {
                symbol->isInit = 0;
                createGlobalVariable(asmFile, symbol);
            }
        }
    }

    optprocessCon(asmFile);
    optprocessZeroArray(asmFile);
    fclose(asmFile);
    fclose(asmFile2);
    fprintf(stderr, "asm successful");
}