#pragma once

#include "../utility/System.h"
#include "../frontend/Token.h"
#include "../frontend/AST.h"
#include "../midend/MIR.h"
#include "../backend/LIR.h"
#include "../backend/Assembly.h"

extern vector<string> source;
extern vector<Token> tokens;
extern unique_ptr<ASTNode> ast;
extern MIR mir;
extern LIR lir;
extern ProgramAssembly assembly;

void PrintSource();
void PrintTokens();
void PrintAST();
void PrintMIR();
void PrintLIR();
void PrintAssembly();