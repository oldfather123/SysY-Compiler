#pragma once
#ifndef CFA_H
#define CFA_H
#include"../IR/GCFG.h"
#include"AST.h"


GCFG* analyzeControlFlow(std::shared_ptr<ExprAST>ast);



#endif // !CHA_H

