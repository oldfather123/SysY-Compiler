//
// Created by 牛奕博 on 2023/6/28.
//

#ifndef ANUC_SBREGSPILL_H
#define ANUC_SBREGSPILL_H
//ssa-based寄存器分配，简称sb分配
//策略为保留两个寄存器，先溢出后接合
//这里是负责溢出的
//思考一下能不能优化移动指令
#include "irBuilder.h"
#include "rvValue.h"
#include "lowInst.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>
#include "livenessInfo.h"

using namespace std;
namespace anuc {
    static int floatRegNum = 0;
    static int integerRegNum = 13;
    class SBRegSpill {
        Function *function;
        IRBuilder *builder;
        RegTable *regTable;
        //溢出的变量
        set<Value *> spillValue;
        //程序某一点的活跃信息
        map<Instruction *, LiveOut> &liveInfo;
        map<Value*, Value*> &spillArgs;
        //扫描所有指令，溢出寄存器数量超过的指令
        //保留s0 s1用于溢出
        void spill() {
            //优先队列用于比较的结构体
            //小于号是大顶堆
            struct cmp {
                bool operator()(RegisterVar *x, RegisterVar *y) {
                    return x->getUsesLength() > y->getUsesLength();
                }
            };


            //遍历每一条指令，寄存器压力为liveOut + 操作数
            for (auto bit = function->getBegin(); bit != function->getEnd(); ++bit) {
                BasicBlock *b = &*bit;
                for (auto iit = b->getBegin(); iit != b->getEnd(); ++iit) {
                    Instruction *s = &*iit;
                    LiveOut &liveOut = liveInfo[s];
                    set<RegisterVar *> integerSpillRegs;
                    //遍历操作数
                    for (int i = 0; i < s->getOperands()->size(); ++i) {
                        RegisterVar *op = dyn_cast<RegisterVar>(s->getOperands(i)->value);
                        if (!op) continue;
                        if (isa<FloatType>(op->getType())) {

                        } else {
                            if (spillValue.count(op)) continue;
                            integerSpillRegs.insert(op);
                        }
                    }
                    int integerRegPress{0};
                    //将liveOut整数寄存器插入
                    for (auto &i: liveOut.integerReg) {
                        if (spillValue.count(i)) continue;
                        integerSpillRegs.insert(i);
                    }

                    //整数寄存器压力
                    integerRegPress += integerSpillRegs.size();
                    //处理整形寄存器
                    if (integerRegPress > integerRegNum) {
                        priority_queue<RegisterVar *, vector<RegisterVar *>, cmp> liveVars;
                        //把寄存器都丢入优先队列
                        for (auto &i: integerSpillRegs)
                            liveVars.push(i);
                        //溢出需要溢出的寄存器
                        int spillNum = integerRegPress - integerRegNum;
                        while (spillNum > 0) {
                            Value *spillVar = liveVars.top();
                            liveVars.pop();
                            spillValue.insert(spillVar);
                            spillNum--;
                        }
                    }

                    //处理浮点寄存器
                    if (liveOut.floatReg.size() > floatRegNum) {
                        int regPress = liveOut.floatReg.size();
                        //计算实际的寄存器数量（可能有的已经溢出）
                        for (auto v = liveOut.floatReg.begin(); v != liveOut.floatReg.end(); ++v)
                            if (spillValue.count(*v)) --regPress;
                        if (regPress > floatRegNum) {
                            //把里面的寄存器丢入优先队列
                            priority_queue<RegisterVar *, vector<RegisterVar *>, cmp> liveVars;
                            for (auto v = liveOut.floatReg.begin(); v != liveOut.floatReg.end(); ++v) {
                                if (!spillValue.count(*v)) {
                                    liveVars.push(*v);
                                }
                            }
                            //溢出需要溢出的寄存器
                            int spillNum = regPress - floatRegNum;
                            while (spillNum > 0) {
                                Value *spillVar = liveVars.top();
                                liveVars.pop();
                                spillValue.insert(spillVar);
                                spillNum--;
                            }
                        }
                    }
                }
            }
        }

        //将溢出的变量放到栈上
        void frameAlloca() {
            map<Value *, Value *> valueToPtr;
            map<Value *, int> &frame = function->getFrame();
            //把所有溢出关联的phi指令都找出来
            //构造变量关联图
            //set<PhiInst *> phiList;
            vector<PhiInst *> phiList;
            for (auto v = spillValue.begin(); v != spillValue.end(); ++v) {
                Value *var = *v;
                if(!var->getDef()) continue;
                if (PhiInst *phi = dyn_cast<PhiInst>(var->getDef())) phiList.push_back(phi);
                for (auto u = var->getUsesBegin(); u != var->getUsesEnd(); ++u) {
                    Instruction *inst = cast<Instruction>((&*u)->user);
                    PhiInst *phi = dyn_cast<PhiInst>(inst);
                    if (!phi) continue;
                    phiList.push_back(phi);
                }
            }
            //广度优先搜索， 构造phi关联图
            set<PhiInst *> visited;
            if(!phiList.empty()) {
                for(auto &p: phiList) {
                    if(!visited.insert(p).second) continue;
                    queue<PhiInst *> workQueue;
                    workQueue.push(p);
                    //创建一个指针用来保留溢出变量的地址，并保存至函数栈中
                    Value *d = p->getResult();
                    Type *ptrTy = builder->GetPointerType(d->getType());
                    Value *ptr = new RegisterVar(ptrTy, builder->GetNewVarName());
                    builder->InsertIntoPool(ptr);
                    int size = d->getType()->getByteSize();
                    frame.insert({ptr, size});
                    while(!workQueue.empty()) {
                        PhiInst *front = workQueue.front();
                        workQueue.pop();
                        Value *dest = front->getResult();
                        auto it = valueToPtr.find(dest);
                        if(it != valueToPtr.end()) it->second = ptr;
                        else {
                            valueToPtr.insert({dest, ptr});
                        }
                        for (auto u = dest->getUsesBegin(); u != dest->getUsesEnd(); ++u) {
                            Instruction *inst = cast<Instruction>((&*u)->user);
                            PhiInst *phi = dyn_cast<PhiInst>(inst);
                            if (!phi) continue;
                            if(!visited.insert(phi).second) continue;
                            workQueue.push(phi);
                        }
                        //auto operands = front->getOperands();
                        for(int i = 0; i < front->getOperands()->size(); i +=2) {
                            Value *op = front->getOperands(i)->value;
                            auto it = valueToPtr.find(op);
                            if(it != valueToPtr.end()) it->second = ptr;
                            else {
                                valueToPtr.insert({op, ptr});
                            }
                            for (auto u = op->getUsesBegin(); u != op->getUsesEnd(); ++u) {
                                Instruction *inst = cast<Instruction>((&*u)->user);
                                PhiInst *phi = dyn_cast<PhiInst>(inst);
                                if (!phi) continue;
                                if(!visited.insert(phi).second) continue;
                                workQueue.push(phi);
                            }
                        }

                    }
                }

            }


            for (auto v: spillValue) {
                //看看是不是函数参数
                if (valueToPtr.find(v) != valueToPtr.end()) {
                    if(!v->getDef()) {
                        auto ptr = valueToPtr.find(v)->second;
                        spillArgs.insert({v, ptr});
                        int size = v->getType()->getByteSize();
                        frame.insert({ptr, size});
                    }
                    continue;
                }
                //创建一个指针用来保留溢出变量的地址，并保存至函数栈中
                Type *ptrTy = builder->GetPointerType(v->getType());
                Value *ptr = new RegisterVar(ptrTy, builder->GetNewVarName());
                builder->InsertIntoPool(ptr);
                int size = v->getType()->getByteSize();
                frame.insert({ptr, size});
                valueToPtr.insert({v, ptr});
                if(!v->getDef()) {
                    spillArgs.insert({v, ptr});
                }

            }

            //在函数头部插入对于参数的store
            BasicBlock *entry = function->getEnrty();
            Instruction *insertPoint = &*entry->getBegin();
            auto args = function->getArgVals();
            for(auto &arg: args) {
                auto it = spillArgs.find(arg);
                if(it == spillArgs.end()) continue;
                Instruction *ls = new LowStore(entry, builder->GetConstantInt32(0),
                                               it->first, it->second);
                entry->insertIntoBackChild(insertPoint, ls);
                builder->InsertIntoPool(ls);
            }


            for(auto it = valueToPtr.begin(); it != valueToPtr.end(); ++it) {
                Value *var = it->first;
                Value *ptr = it->second;
                //在变量的定义和使用处插入store，phi不加store
                Instruction *def = var->getDef();
                if (def && !isa<PhiInst>(def)) {
                    BasicBlock *parent = def->getParent();
                    Instruction *ls = new LowStore(parent, builder->GetConstantInt32(0), def->getResult(), ptr);
                    parent->insertIntoBackChild(def, ls);
                    builder->InsertIntoPool(ls);
                }
                //变量使用处插入load，phi不加
                for (auto u = var->getUsesBegin(); u != var->getUsesEnd();) {
                    Use *cu = &*u;
                    ++u;
                    Instruction *inst = cast<Instruction>((&*cu)->user);
                    //跳过lowstore/PHI
                    if (isa<LowStore>(inst) || isa<PhiInst>(inst)) continue;
                    BasicBlock *parent = inst->getParent();
                    //s0、s1用于变量溢出，查看被使用的变量是否已经被溢出占用s0/s1
                    RvRegister *s = regTable->getReg(RvRegister::s0);;
                    for (int i = 0; i < inst->getOperands()->size(); ++i) {
                        Use *op = inst->getOperands(i);
                        if (op->value != var && spillValue.count(op->value)) {
                            //说明一个操作数已经被溢出占用了s0，将该操作数溢出到s1
                            s = regTable->getReg(RvRegister::s1);
                        }
                    }
                    Instruction *ll = new LowLoad(parent, s, builder->GetConstantInt32(0), ptr);
                    cu->replaceValueWith(s);
                    parent->insertIntoChild(ll, inst);
                    builder->InsertIntoPool(ll);
                }
            }
            //删掉所有phi
            for (auto &phi: visited) {
                //phi->print();
                phi->eraseFromParent();
            }
        }

        //对栈上变量位置进行重定位
        void reAlloca() {
            map<Value *, int> &frame = function->getFrame();
            int offset = 0;
            int pre = 0;
            for (auto it = frame.begin(); it != frame.end(); ++it) {
                offset += pre;
                pre = it->second;
                it->second = offset;
            }
            function->setFrameSize(offset + pre);

            //打印frame偏移表
        }

    public:
        SBRegSpill(Function *function, IRBuilder *builder,
                   RegTable *regTable, map<Instruction *, LiveOut> &liveInfo,
                   map<Value*, Value*> &spillArgs) :
                function(function), builder(builder), regTable(regTable), liveInfo(liveInfo),
                spillArgs(spillArgs){}

        void run() {
            //把所有虚拟寄存器都找出来
            //printLiveOut();
            //找出溢出变量
            spill();
            //插入load/sotre
            frameAlloca();
            //在函数栈上进行分配
            reAlloca();
            return;
        }
    };
}


#endif //ANUC_SBREGSPILL_H
