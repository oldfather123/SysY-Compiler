#include <stdio.h>

#include "include/sysy.h"

char*        outputFile;
char*        inputFile;
extern FILE* asmFile;
bool         isOptimimze;
// 默认开启优化
void         parseCommand(int argc, char* argv[]) {
    for (size_t i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'o') {
                outputFile = argv[i + 1];
                i++;
            } else if (argv[i][1] == 'O') {
                isOptimimze = true;
            }
        } else {
            inputFile = argv[i];
        }
    }
}

int main(int argc, char* argv[]) {
    sysy_init();
    isOptimimze = false;
    parseCommand(argc, argv);
    initPassManager();
    fprintf(stderr, "init successful\n");
    asmFile = fopen(outputFile, "w");
    if (!asmFile) {
        fprintf(stderr, "failed to create asm \'%s\'\n", outputFile);
        exit(-1);
    }
    compileFile(inputFile);
    fprintf(stderr, "parse successful\n");
    if (isOptimimze) {
        optRiscvCodeGenerator();
    } else {
        riscvCodeGenerator();
    }
    // compileFile("../test/1.c");
}
