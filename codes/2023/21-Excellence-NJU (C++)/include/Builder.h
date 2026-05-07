//
// Created by q1w2e3r4 on 23-7-9.
//

#include "BasicBlock.h"
#include "ValueRef.h"
#include "Function.h"
#include "GlobalUnit.h"
#include "IRInstruction.h"
#ifndef COMPILER_BUILDER_H
#define COMPILER_BUILDER_H

/*
 * Builder类
 * 主要负责基本块的构建，切换功能
 * 其可以记录下当前作用的基本块，在生成IR指令时，我们通过builder向对应基本块中添加指令
 * 另外，builder可自动将基本块追加到对应函数的block_list中，具体做法见函数对应描述
 *
*/

class Builder {
public:
    BasicBlock * nowBlock;
    Function * curFunc;
    GlobalUnit * globalUnit;

    Builder(GlobalUnit * gu){ this->globalUnit = gu; nowBlock = nullptr; }

    void AppendCode(IRInstruction* instruction);

    BasicBlock * AppendBasicBlock(Function * func, std::string& blockName);
    // 这个是添加基本块，选择添加到哪个函数里

    void PositionBuilderAtEnd(BasicBlock * block); //切换到对应的基本块(若为nullptr,则切换到全局块)

    void LinkBlock(BasicBlock *succ);
};


#endif //COMPILER_BUILDER_H
