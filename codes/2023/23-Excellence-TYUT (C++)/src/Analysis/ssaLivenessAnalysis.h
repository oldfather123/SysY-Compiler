//
// Created by 牛奕博 on 2023/7/14.
//

#ifndef ANUC_SSALIVENESSANALYSIS_H
#define ANUC_SSALIVENESSANALYSIS_H

#include "irBuilder.h"
#include "rvValue.h"
#include "lowInst.h"
#include "livenessInfo.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>

namespace anuc {
    class SSALivenessAnalysis {
        Function *function;
        RegTable *regTable;
    private:
        //程序某一点的活跃信息
        map<Instruction *, LiveOut> liveInfo;

        void scanBlock(BasicBlock *bb, RegisterVar *x, set<BasicBlock *> &scannedBlocks) {
            if (!scannedBlocks.insert(bb).second) return;
            Instruction *s = &*bb->getBack();
            scanLiveOut(s, x, scannedBlocks);
        }

        void scanLiveIn(Instruction *inst, RegisterVar *x, set<BasicBlock *> &scannedBlocks) {
            BasicBlock *parent = inst->getParent();
            if (inst == &*parent->getBegin()) {
                for (auto pre = parent->predBegin(); pre != parent->predEnd(); ++pre) {
                    //向前驱bb添加liveOut
                    scanBlock(*pre, x, scannedBlocks);
                }
            } else {
                scanLiveOut(inst->getPrev(), x, scannedBlocks);
            }
        }

        void scanLiveOut(Instruction *inst, RegisterVar *x, set<BasicBlock *> &scannedBlocks) {
            if (liveInfo.find(inst) == liveInfo.end())
                liveInfo.insert({inst, LiveOut()});
            //将liveOut信息加入liveInfo
            if (isa<FloatType>(x->getType())) liveInfo[inst].floatReg.insert(x);
            else  liveInfo[inst].integerReg.insert(x);

            //判断该语句是否为x的定义w点　
            if (!inst->getResult() ||
                dyn_cast<RegisterVar>(inst->getResult()) != x) {
                scanLiveIn(inst, x, scannedBlocks);
            }
        }

        void ssaLiveness(set<RegisterVar *> &regVars) {
            //遍历所有虚拟寄存器的所有user
            for (auto it = regVars.begin(); it != regVars.end(); ++it) {
                RegisterVar *regVar = cast<RegisterVar>(*it);
                for (auto u = regVar->getUsesBegin(); u != regVar->getUsesEnd(); ++u) {
                    set<BasicBlock *> scannedBlocks;
                    Instruction *user = cast<Instruction>((&*u)->user);
                    //如果该虚拟寄存器被phi使用，找到对应的bb
                    if (PhiInst *phi = dyn_cast<PhiInst>(user)) {
                        BasicBlock *pre;
                        for (int i = 0; i < phi->getOperands()->size(); i = i + 2) {
                            if (phi->getOperands(i)->value != regVar) continue;
                            pre = cast<BasicBlock>(phi->getOperands(i + 1)->value);
                            scanBlock(pre, regVar, scannedBlocks);
                            break;
                        }
                    } else scanLiveIn(user, regVar, scannedBlocks);
                }
            }
        }

    public:
        SSALivenessAnalysis(Function *function, RegTable *regTable) :
                function(function), regTable(regTable) {}

        void printLiveOut() {
            cout << function->getName() << endl;
            for (auto b = function->getBegin(); b != function->getEnd(); ++b) {
                auto block = &*b;
                for (auto i = block->getBegin(); i != block->getEnd(); ++i) {
                    cout << "--------------------" << endl;
                    auto inst = &*i;
                    inst->print();
                    auto info = liveInfo[inst];
                    cout << "  integer live out :";
                    for (auto v: info.integerReg) cout << v->toString() << "  ";
                    cout << "\n  float live out :";
                    for (auto v: info.floatReg) cout << v->toString() << "  ";
                    cout << "\n" << endl;
                }
            }
        }

        //通过ssa活跃变量分析法进行分析
        //特殊处理函数参数，函数参数的def被设置在函数开头第一条指令
        set<Value*> funcArgs;
        map<Instruction *, LiveOut> &computeLiveness() {
            set<RegisterVar *> regVars;
            //注意处理函数参数！
            //处理方法是将参数的def暂时全部设置到函数第一条指令！
            Instruction *head = &*function->getEnrty()->getFront();
            for(auto a: function->getArgVals()) {
                auto x = dyn_cast<RegisterVar>(a);
                if(!x) continue;
                regVars.insert(x);
                funcArgs.insert(x);
                a->setInst(head);
            }

            //找出所有虚拟寄存器
            for (auto bit = function->getBegin(); bit != function->getEnd(); ++bit) {
                BasicBlock *b = &*bit;
                for (auto iit = b->getBegin(); iit != b->getEnd(); ++iit) {
                    Instruction *i = &*iit;
                    if (i->getResult()) {
                        RegisterVar *x = dyn_cast<RegisterVar>(i->getResult());
                        //避开rv寄存器变量
                        if (!x) continue;
                        bool j = regVars.insert(x).second;
                        if (!j) {
                            cerr << "数据流分析中发现ssa中有一个变量被定义了两次!" << endl;
                            cout <<x->toString();
                        }

                    }
                }
            }
            ssaLiveness(regVars);
            for(auto a: function->getArgVals()) {
                a->setInst(nullptr);
            }
            return liveInfo;
        }
    };
}


#endif //ANUC_SSALIVENESSANALYSIS_H
