#pragma once

#include "mem2reg.h"
#include "simplifycodepass.h"
#include "constantfoldpass.h"
#include "strengthreductionpass.h"
#include "commonsubexpressioneliminationpass.h"
#include "inlinefunctionpass.h"
#include "finalpass.h"
#include "loopunrollpass.h"

namespace IR
{
    struct PassController
    {

        PassController() = default;

        void run(bool opt, IR::Module *module);
    };

    /*
    TODO:
    1. 添加常量折叠优化
    2. 添加常量传播优化
    3. 添加公共子表达式删除优化
    4. 添加循环不变代码外提优化
    5. 强度削弱
    6. add2mul
    */
}