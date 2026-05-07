// 公共子表达式删除
#pragma once
#include "Module.h"


bool impossibleToCSE(InsID type);
bool isMemoryRelated(InsID type);
bool cse(FunctionPtr func);
void runCSE(Module& mod);