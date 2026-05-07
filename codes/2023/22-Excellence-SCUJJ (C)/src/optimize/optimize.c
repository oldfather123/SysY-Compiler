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

void outputIR(char *filename, Function *fnc) {
    FILE *fp = fopen(filename, "w");
    printBasicAllBlock(fp, fnc);
    fclose(fp);
}

void optimize(Function *fnc) { executeFunctionPass(fnc); }
