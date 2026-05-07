#include <stdio.h>
#include <stdlib.h>
#include "SymbolTable.h"
#include "AST.h"
#include"../IR/definition.h"

void preHandleExtension(SymbolTable* symbolTable);

Symbol* InitGetInt();
Symbol* InitGetCh();
Symbol* InitGetFloat();
Symbol* InitGetArray();
Symbol* InitGetFArray();
Symbol* InitPutInt();
Symbol* InitPutChar();
Symbol* InitPutFloat();
Symbol* InitPutArray();
Symbol* InitPutFArray();
Symbol* InitPutF();
Symbol* InitStartTime();
Symbol* InitStopTime();