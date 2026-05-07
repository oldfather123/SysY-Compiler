//
// Created by 牛奕博 on 2023/7/17.
//

#ifndef ANUC_CALLGRAPH_H
#define ANUC_CALLGRAPH_H
#include "core.h"
#include "rvValue.h"
#include <map>
#include <set>
namespace anuc {
    //存储函数信息： 函数--函数内部使用的s寄存器
    struct FuncInfo {
        Function *func;
        //需要保存的寄存器
        set<RvRegister *> saveRegs;
    };



    struct CallNode {
        Function *func{nullptr};
        set<CallNode*> calls;
        set<CallNode*> calledBy;
        CallNode(Function *func):func(func) {}
    };
}



#endif //ANUC_CALLGRAPH_H
