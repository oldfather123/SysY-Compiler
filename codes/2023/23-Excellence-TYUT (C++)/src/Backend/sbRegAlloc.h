//
// Created by 牛奕博 on 2023/7/13.
//

#ifndef ANUC_SBREGALLOC_H
#define ANUC_SBREGALLOC_H

#include "irBuilder.h"
#include "rvValue.h"
#include "lowInst.h"
#include "livenessInfo.h"
#include "blockDFSCalculator.h"
#include "blockDomTree.h"
#include "callGraph.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>
//进行寄存器分配
//浮点数先不管了
namespace anuc {
    class SBRegAlloc {
        Function *function;
        RegTable *regTable;
        IRBuilder *builder;
        //存储变量映射
        map<Value *, RvRegister *> tempMap;
        //存储正在使用
        set<RvRegister *> inUse;
        //支配树
        map<BasicBlock *, set<BasicBlock *>> domTree;
        set<PhiInst *> phis;

        //存储分配过的s寄存器
        set<RvRegister*> sRegs;


    public:
        //cmp为两种模式
        //s_mode模式为1会优先分配s寄存器，否则优先分配t寄存器
        struct CMP {
            static int s_mode;

            bool operator()(RvRegister *a, RvRegister *b) {
                switch (s_mode) {
                    case 1:
                        return a->getWeight() > b->getWeight();
                    default:
                        return a->getWeight() < b->getWeight();
                }
            }
        };

    private:


        //被调用者保存
        map<Function *, set<RvRegister*>> &calledSaved;
        //调用者保存
        map<CallInst*, set<RvRegister*>> &callerSaved;


        priority_queue<RvRegister *, vector<RvRegister *>, CMP> integerRegs;
        map<Instruction *, LiveOut> &liveInfo;
        map<RegisterVar *, int> saveFrame;

        //参数传递寄存器
        vector<RvRegister *> integerArgReg;
        vector<RvRegister *> floatArgReg;

        //处理call，记录call时需要保存的寄存器
        void handleCallInst(CallInst *s) {
            set<RvRegister *> saves;
            //查看liveOut，是否需要保存t0~t7，a0～a7，fa0～fa7
            auto &liveOut = liveInfo[s];
            //如过在liveOut且不为该指令的def中，需要保存

            RvRegister *defR{nullptr};
            Value *def = s->getResult();
            if(def) defR = tempMap[def];

            //遍历liveout
            for(auto &i :liveOut.integerReg) {
                auto it = tempMap.find(i);
                if(it == tempMap.end()) {
                    cerr << "寄存器分配在处理call时，发现一个在liveOut中且未分配的变量！" << endl;
                    exit(1);
                }
                RvRegister *r = it->second;
                if(defR &&r == defR) continue;
                if(RvRegister::isTReg(r) || RvRegister::isAReg(r)) {
                    saves.insert(r);
                }

            }

            callerSaved.insert({s, saves});
        }

        void handleArgs() {
            //处理函数参数，在a0~a7里
            vector<Value *> &args = function->getArgVals();
            for (auto v: args) {

                if (v->usesEmpty()) continue;
                if (isa<FloatType>(v->getType())) {

                }
                RvRegister *reg = getIntReg();
                if(!reg) {
                    exit(1);
                }
                inUse.insert(reg);
                tempMap.insert({v, reg});
                if(RvRegister::isSReg(reg))
                sRegs.insert(reg);

            }
        }

        void initRegs() {
            //如果函数不会再调用其他函数，可以优先使用t寄存器并且不保存
            //如果函数不被其他调用，可以随意使用s寄存器且不恢复
            if (cNode->calls.empty()) {
                CMP::s_mode = 0;
            } else {
                CMP::s_mode = 1;
            }
            vector<RvRegister *> &iRegs = regTable->getAllIntegerRegs();
            for (auto i: iRegs) {
                if (!inUse.count(i)) integerRegs.push(i);
            }

        }

        //分配寄存器，如果寄存器不足返回空指针
        RvRegister *getIntReg() {
            if(integerRegs.empty()) {
                cerr << "寄存器不足！已经占用寄存器数量：" << inUse.size();
                return nullptr;
            }
            auto r = integerRegs.top();
            integerRegs.pop();
            return r;
        }

        //前序遍历基本块
        void preOrder(BasicBlock *b) {
            //把bb的前驱的liveOut所占用的寄存器加入inUse
            for (auto pred = b->predBegin(); pred != b->predEnd(); ++pred) {
                Instruction *i = (&*(*pred)->getBack());
                for (auto v: liveInfo[i].integerReg) {
                    auto it = tempMap.find(v);
                    if (it != tempMap.end()) inUse.insert(it->second);
                }
                for (auto v: liveInfo[i].floatReg) {
                    auto it = tempMap.find(v);
                    if (it != tempMap.end()) inUse.insert(it->second);
                }
            }

            for (auto i = b->getBegin(); i != b->getEnd(); ++i) {
                Instruction *s = &*i;
                if (PhiInst *phi = dyn_cast<PhiInst>(s)) phis.insert(phi);
                vector<Use *> *ops = s->getOperands();

                //遍历操作数，如果有操作数不在liveOut中，回收
                auto &integerLiveOut = liveInfo[s].integerReg;
                for (auto op = ops->begin(); op != ops->end(); ++op) {
                    RegisterVar *x = dyn_cast<RegisterVar>((*op)->value);
                    if (!x) continue;
                    //浮点数
                    if(isa<FloatType>(x->getType())) {}
                    //整数
                    else {
                        if (integerLiveOut.count(x)) continue;
                        //不在liveOut中，回收寄存器
                        //注意，phi使用的变量此时可能还未被分配，需要防止错误回收
                        auto it = tempMap.find(x);
                        if (it != tempMap.end()) {
                            inUse.erase(it->second);
                            integerRegs.push(it->second);
                        }

                    }
                }
                //给定义的变量分配寄存器
                Value *def = s->getResult();
                if (def && isa<RegisterVar>(def)) {
                    auto reg = getIntReg();

                    //如果reg已经在inuse中，重新分配
                    while (!inUse.insert(reg).second) {
                        reg = getIntReg();
                    }
                    //寄存器不足的情况，直接报错
                    if(!reg) {
                        s->print();
                        exit(1);
                    }
                    if (tempMap.find(def) != tempMap.end()) {
                        cerr << "我擦，有变量被定义过两次: " << def->toString() << endl;
                        continue;
                    }
                    tempMap.insert({def, reg});
                    //如果分配的为s寄存器，存入函数被调用者保存
                    if(RvRegister::isSReg(reg)) {
                        sRegs.insert(reg);
                    }
                    //如果def不在liveout中，直接回收
                    if(!integerLiveOut.count(cast<RegisterVar>(def))) {
                        inUse.erase(reg);
                        integerRegs.push(reg);
                    }
                }




                //如果为call，需要特殊处理，看看有无需要调用者保存的寄存器
                if (CallInst *call = dyn_cast<CallInst>(s))
                    handleCallInst(call);
            }

            //每遍历一个基本快，重置寄存器优先队列和use
            priority_queue<RvRegister *, vector<RvRegister *>, CMP> temp;
            integerRegs.swap(temp);
            set<RvRegister*> temp2;
            inUse.swap(temp2);
            vector<RvRegister *> &iRegs = regTable->getAllIntegerRegs();
            for (auto i: iRegs) integerRegs.push(i);
            //继续遍历支配树节点
            for (auto child: domTree[b])
                preOrder(child);
        }

        BasicBlock *insertMoveInst(BasicBlock *currentBlock, Value *result, Value *x, BasicBlock *b) {
            //如果b中只有一个后续， 不做处理
            if (b->getSucc().size() == 1) {
                return nullptr;
            }
                //否则插入关键边
            else {
                vector<BasicBlock *> &succ = b->getSucc();
                for (int i = 0; i < succ.size(); ++i) {
                    if (succ[i] != currentBlock) continue;
                    //插入新创建的基本块并插入
                    BasicBlock *nb = builder->GetBasicBlock("phi_block");
                    BasicBlock::insertBasicBlock(b, nb, currentBlock);
                    //修改尾部跳转指令
                    Instruction *j = &*b->getBack();
                    while (isa<RVzerocondbranch>(j) || isa<RVcondbranch>(j) ||
                           isa<RVbranch>(j)) {
                        for (auto op: *j->getOperands())
                            if (op->value == currentBlock)
                                op->replaceValueWith(nb);
                        j = j->getPrev();
                    }
                    //在新创建的基本块中插入跳转指令
                    Instruction *s = &*b->getBack();
                    RVbranch *ja = new RVbranch(nb, currentBlock);
                    nb->insertBackToChild(ja);
                    builder->InsertIntoPool(ja);
                    return nb;
                }
            }
            return nullptr;
        }


        //寄存器重排
        //利用s1作为中间寄存器完成重排
        //@srcAndDest pair <源寄存器 目标寄存器>
        //一个寄存器先在源寄存器出现，后在目标寄存器出现，说明存在交换问题，特殊处理
        //算法实现为给目标寄存器顺序编号，对每一对<源 目标>， 目标寄存器编号存在，且目标寄存器编号 > 当前指令编号
        //特殊处理
        /*                               0   1   2
         *   phi r1 <= r2   0           r2  r3  r1
         *   phi r2 <= r3   1
         *   phi r3 <= r1   2
         *
         *
         *
         * */
        void permutation(vector<pair<RvRegister *, RvRegister *>> &destWithSrc, BasicBlock *liveInBB) {
            RvRegister *s1 = regTable->getReg(RvRegister::s1);
            LIBuilder liBuilder(builder);
            Instruction *j = &*liveInBB->getBack();
            //找到插入点插入交换 用s1作为临时寄存器
            while (isa<RVzerocondbranch>(j) || isa<RVcondbranch>(j) ||
                   isa<RVbranch>(j))
                j = j->getPrev();
            liBuilder.SetInsertPoint(j);

            //给源寄存器编号
            //并且建立映射
            map<RvRegister *, int> regToNum;
            for (int i = 0; i < destWithSrc.size(); ++i) {
                regToNum.insert({destWithSrc[i].second, i});
            }

            //存储重新映射过的变量
            set<RvRegister *> reSet;
            //遍历，查找会产生交换问题的值
            for (int i = 0; i < destWithSrc.size(); ++i) {
                RvRegister *dest = destWithSrc[i].first;
                RvRegister *src = destWithSrc[i].second;
                auto mit = regToNum.find(dest);
                if (mit == regToNum.end()) continue;
                if (regToNum[dest] <= i) continue;
                //源寄存器编号小于目标寄存器编号，存在问题
                //利用s0作为临时寄存器重新映射两个值
                reSet.insert(dest);
            }
            //记录s0的占用状态，处理嵌套问题
            bool used = false;
            //插入移动变量完成输入
            for (int i = 0; i < destWithSrc.size(); ++i) {
                RvRegister *dest = destWithSrc[i].first;
                RvRegister *src = destWithSrc[i].second;
                //如果dest被重新映射, 增加一个对s0赋值
                if (reSet.count(dest)) {
                    if (used == true) {
                        //如果s0被占用了，就把这个指令往后丢
                        destWithSrc.push_back(destWithSrc[i]);
                        regToNum[src] = i + destWithSrc.size() - 1;
                        ++i;
                        continue;
                    }
                    //创建对s0的赋值
                    liBuilder.CreateMv(s1, dest);
                    used = true;
                }
                    //不在count中，目标寄存器大于当前指令编号，说明他的前置已经被丢到后面了，把他也丢到后面
                else if (regToNum.find(dest) != regToNum.end() &&
                         regToNum[dest] > i) {
                    destWithSrc.push_back(destWithSrc[i]);
                    regToNum[src] = i + destWithSrc.size() - 1;
                    ++i;
                    continue;
                }

                //如果src被重新映射，将src改为s0
                if (reSet.count(src)) {
                    src = s1;
                    used = false;
                }
                liBuilder.CreateMv(dest, src);
            }
        }

        void phiResolver(vector<PhiInst *> &list) {
            set<BasicBlock *> blocks;
            int opSize = list.back()->getOperands()->size();
            //对来自同一个block的变量进行重排
            for (int i = 0; i < opSize; i += 2) {
                vector<pair<RvRegister *, RvRegister *>> destWithSrc;
                BasicBlock *liveInBB = cast<BasicBlock>(list.back()->getOperands(i + 1)->value);
                for (auto &phi: list)
                    destWithSrc.push_back({cast<RvRegister>(phi->getResult()),
                                           cast<RvRegister>(phi->getOperands(i)->value)});
                permutation(destWithSrc, liveInBB);
            }
            return;
        }

        //消除所有phi指令!
        void phiResolution() {
            //关键路径插入基本快，处理拷贝丢失
            for (auto phi: phis) {
                Value *result = phi->getResult();
                for (int j = 0; j < phi->getOperands()->size(); j += 2) {
                    Value *x = phi->getOperands(j)->value;
                    if (x != result) {
                        auto nb = insertMoveInst(phi->getParent(), result,
                                                 x, cast<BasicBlock>(phi->getOperands(j + 1)->value));
                        if (nb)
                            phi->getOperands(j + 1)->replaceValueWith(nb);
                    }

                }
            }
            //插入移动指令，处理交换问题
            set<BasicBlock *> phiBB;
            for (auto phi: phis) phiBB.insert(phi->getParent());
            for (auto b: phiBB) {
                vector<PhiInst *> list;
                for (auto i = b->getBegin(); i != b->getEnd(); ++i) {
                    PhiInst *phi = dyn_cast<PhiInst>(&*i);
                    if (!phi) break;
                    list.push_back(phi);
                }
                //集中解决同一基本块下的phi
                phiResolver(list);
            }
            //删除所有phi
            for (auto phi: phis) phi->eraseFromParent();

        }

        //在函数头部插入对于寄存器的分配
        void handleArgs2() {
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

            LIBuilder liBuilder(builder);
            Instruction *insertPoint = &*function->getEnrty()->getFront();
            liBuilder.SetInsertPoint(insertPoint);
            vector<Value *> &args = function->getArgVals();
            int i = 0;
            int f = 0;
            //为参数分配寄存器
            for (auto v: args) {
                //如果发现参数被溢出，改为创建store
                if (tempMap.find(v) == tempMap.end()) continue;
                //浮点数
                if (isa<FloatType>(v->getType())) {
                    continue;
                }
                //整数
                liBuilder.CreateMv(tempMap[v], integerArgReg[i++]);

            }
        }

        CallNode *cNode;

    public:
        SBRegAlloc(Function *function, IRBuilder *builder,
                   RegTable *regTable, map<Instruction *, LiveOut> &liveInfo,
                   map<Function *, set<RvRegister*>> &calledSaved
                   , map<CallInst*, set<RvRegister*>> &callerSaved,
                   CallNode *cNode) :
                function(function), builder(builder),
                regTable(regTable), liveInfo(liveInfo),
                calledSaved(calledSaved),
                callerSaved(callerSaved), cNode(cNode) {

            //初始化Builder，将函数插入callerSaved
            builder->SetCurrentFunc(function);
        }

        void run() {
            //cout << function->getName() << endl;
            // 先计算支配树
            BasicBlock *entry = function->getEnrty();
            vector<BasicBlock *> postOrder;
            blockDFSCalulator::calculateBBPostOrder(postOrder, entry);
            BlockDomTree dt;
            dt.blockDomTreeCalulator(postOrder);
            domTree = dt.getDominatesTreeWithIChild();

            //处理函数参数，初始化待分配的寄存器
            //选择寄存器分配模式
            initRegs();
            handleArgs();
            //遍历支配书
            preOrder(function->getEnrty());
            //将操作数更换为寄存器
            for (auto &it: tempMap) {
                it.first->replaceAllUseWith(it.second);
                if (it.first->getDef() && it.first->getDef()->getResult()) {
                    it.first->getDef()->setResult(it.second);
                }
            }
            //处理phi
            phiResolution();
            //返回函数，若不为空，则需要使用a0保存返回值

            //在函数开头插入变量移动
            handleArgs2();

            //记录函数被调用者保存
            calledSaved.insert({function, sRegs});


        }


    };

}


#endif //ANUC_SBREGALLOC_H
