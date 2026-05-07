
#include "Module.h"
#include <iostream>
#include <map>
#include <unordered_set>
using namespace std;

bool GVN(FunctionPtr func);
void runGVN(Module& module);