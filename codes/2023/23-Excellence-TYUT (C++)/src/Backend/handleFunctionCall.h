//
// Created by 牛奕博 on 2023/7/17.
//

#ifndef ANUC_HANDLEFUNCTIONCALL_H
#define ANUC_HANDLEFUNCTIONCALL_H

#include <algorithm>
#include <set>
#include "core.h"
#include "rvValue.h"
#include "callGraph.h"
#include "map"
#include "lowInst.h"

using namespace std;
//处理函数调用
//在每一次call时候进行函数寄存器的传递和恢复
namespace anuc {
    class HandleFunctionCall {
        Module *m;
        IRBuilder *builder;
        RegTable *regTable;
        //调用者保存
        map<CallInst *, set<RvRegister *>> &callerSaved;


    public:
        HandleFunctionCall(Module *m, IRBuilder *builder, RegTable *regTable,
                           map<CallInst *, set<RvRegister *>> &callerSaved) :
                m(m), builder(builder), regTable(regTable), callerSaved(callerSaved) {}


        //将ir中的call转化
        void transformCallInst() {
            LIBuilder liBuilder(builder);
            RvRegister *sp = regTable->getReg(RvRegister::sp);

            for (auto cit = callerSaved.begin(); cit != callerSaved.end(); ++cit) {
                CallInst *s = cit->first;
                auto &frame = s->getParent()->getParent()->getFrame();
                liBuilder.SetInsertPoint(s->getPrev());
                Function *func = s->getParent()->getParent();
                auto &info = cit->second;
                vector<RvRegister *> saves;
                //进行调用者保存
                for (auto &i: cit->second) saves.emplace_back(i);
                int offset{0};
                //注意和16对齐
                int size = saves.size() * 8;
                if(size % 16 != 0) size += (16 - size % 16);
                if(!saves.empty()) liBuilder.CreateAddi(sp, sp, -size);
                //保存寄存器
                for (auto &x: saves) {
                    liBuilder.CreateStore(x, sp, offset, RVStore::sd);
                    offset += 8;
                }


                //函数传参
                //记得特殊处理常数
                //初始化传参寄存器
                vector<RvRegister *> integerArgReg;
                vector<RvRegister *> floatArgReg;
#define ARG_REG_GEN(X) integerArgReg.push_back(regTable->getReg(RvRegister::a##X)); \
floatArgReg.push_back(regTable->getReg(RvRegister::fa##X));
                ARG_REG_GEN(0)
                ARG_REG_GEN(1)
                ARG_REG_GEN(2)
                ARG_REG_GEN(3)
                ARG_REG_GEN(4)
                ARG_REG_GEN(5)
                ARG_REG_GEN(6)
                ARG_REG_GEN(7)

                //将函数参数放入传参寄存器
                for (int i = 0; i < s->getOperands()->size(); ++i) {
                    Value *x = s->getOperands(i)->value;
                    if (auto ci = dyn_cast<ConstantInt>(x)) {
                        liBuilder.CreateLi(integerArgReg[i], ci);
                        continue;
                    } else if (isa<ConstantFloat>(x)) {
                        continue;
                    }

                    //看看是float还是int
                    RvRegister *r = cast<RvRegister>(x);
                    if (RvRegister::isFReg(r)) {
                        continue;
                    }
                    liBuilder.CreateMv(integerArgReg[i], r);
                }
                //创建call函数
                liBuilder.CreateCall(s->getFunc());
                //将返回参数传递到a0
                if(s->getResult()) {
                    BaseReg *dest = cast<BaseReg>(s->getResult());
                    liBuilder.CreateMv(dest, regTable->getReg(RvRegister::a0));
                }

                offset = 0;
                //恢复寄存器
                for (auto &x: saves) {
                    liBuilder.CreateLoad(x, sp, offset, RVLoad::ld);
                    offset += 8;
                }
                if(!saves.empty()) liBuilder.CreateAddi(sp, sp, size);
                s->eraseFromParent();
            }
        }

    };
}


#endif //ANUC_HANDLEFUNCTIONCALL_H
