#include "Module.h"

bool DCE(FunctionPtr func);
void runDCE(Module& mod);
void runDeadArrElemim(Module& mod);