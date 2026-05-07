#pragma once

#include "value.h"
#include "instruction.h"
#include <assert.h>

namespace IR
{
    // 常量折叠

    // czr: 这一块你们应该清楚，对于除了foldgep和foldload之外的函数，
    //      都是对两个Value对象操作，如果能常量折叠就返回一个ConstantInt32或者ConstantFloat对象
    //      如果不能就返回nullptr。注意类型要严格检查，不要出现int和float混用的情况。
    struct ConstantFolder
    {
        ConstantFolder() = default;
        bool isPointingConst(Value *val);
        // 二元运算符
        Value *FoldAddInt32(Value *LHS, Value *RHS);
        Value *FoldSubInt32(Value *LHS, Value *RHS);
        Value *FoldMulInt32(Value *LHS, Value *RHS);
        Value *FoldDivInt32(Value *LHS, Value *RHS);
        Value *FoldRemInt32(Value *LHS, Value *RHS);
        Value *FoldFAddFloat(Value *LHS, Value *RHS);
        Value *FoldFSubFloat(Value *LHS, Value *RHS);
        Value *FoldFMulFloat(Value *LHS, Value *RHS);
        Value *FoldFDivFloat(Value *LHS, Value *RHS);
        // 二元逻辑运算符
        Value *FoldLtInt32(Value *LHS, Value *RHS);  // <
        Value *FoldLeInt32(Value *LHS, Value *RHS);  // <=
        Value *FoldGtInt32(Value *LHS, Value *RHS);  // >
        Value *FoldGeInt32(Value *LHS, Value *RHS);  // >=
        Value *FoldEqInt32(Value *LHS, Value *RHS);  // ==
        Value *FoldNeInt32(Value *LHS, Value *RHS);  // !=
        Value *FoldFLtFloat(Value *LHS, Value *RHS); // <
        Value *FoldFLeFloat(Value *LHS, Value *RHS); // <=
        Value *FoldFGtFloat(Value *LHS, Value *RHS); // >
        Value *FoldFGeFloat(Value *LHS, Value *RHS); // >=
        Value *FoldFEqFloat(Value *LHS, Value *RHS); // ==
        Value *FoldFNeFloat(Value *LHS, Value *RHS); // !=
        // 一元运算符
        Value *FoldNegInt32(Value *Operand);  // -
        Value *FoldFNegFloat(Value *Operand); // -
        Value *FoldNotInt32(Value *Operand);  // !
        // 类型转换
        Value *FoldFPtoSI(Value *Operand);
        Value *FoldSItoFP(Value *Operand);
        // 位运算符
        Value *FoldAndInt32(Value *LHS, Value *RHS);
        Value *FoldOrInt32(Value *LHS, Value *RHS);
        Value *FoldXorInt32(Value *LHS, Value *RHS);

        // 访问操作
        Value *FoldGEP(Value *Base, std::vector<Value *> Indices);
        Value *FoldLoad(BasicType *type, Value *Ptr);

        static ConstantFolder *get()
        {
            return new ConstantFolder();
        }
    };
}