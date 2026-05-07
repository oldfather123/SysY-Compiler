//
// Created by 牛奕博 on 2023/5/7.
//

#ifndef ANUC_SCHEDULEBEFORERA_H
#define ANUC_SCHEDULEBEFORERA_H

#include <vector>
#include <queue>
#include <set>
#include "core.h"
#include "rtti.h"
#include "livenessAnalysis.h"

using namespace std;
namespace anuc {
    struct SNode {
        Instruction *instruction;
        set<SNode *> predLefts;
        set<SNode *> succLefts;
        unsigned predLeft{0};
        unsigned succLeft{0};
        unsigned regPress{0};
        SNode *predChain{nullptr};
        SNode *succChain{nullptr};
    };

    //寄存器分配前的调度器，双向同时调度，用来削弱局部寄存器压力
    //终结指令还没有考虑
    class ScheduleBRPass {
    private:
        using infoChange = map<Instruction *, LivenessInfo>;

        //基本块寄存器压力追踪器！
        //显示溢出的变量的总数量
        //思路为扫描当前块所有指令， 遇到超过寄存器压力的时候就记录。
        struct BasicblockRegPressTracker {
            int max = 2;
            set<Value *> spillVar; //溢出的变量
            BasicblockRegPressTracker(int max = 2) : max(max) {}

            void updateSpillVarNum(infoChange &livenessInfo) {
                set<Value *>().swap(spillVar);
                for (auto p = livenessInfo.begin(); p != livenessInfo.end(); ++p) {
                    int varNum = p->second.regliveOut.size();
                    int spillNum = varNum - max;
                    int i = 0;
                    if (spillNum > 0) {
                        //找找有没有已经溢出过的变量
                        for (auto v = p->second.regliveOut.begin(); v != p->second.regliveOut.end(); ++v)
                            if (spillVar.count(*v))++i;
                        for (auto v = p->second.regliveOut.begin(); v != p->second.regliveOut.end() && i < spillNum; ++v) {
                            if (!spillVar.count(*v)) {
                                spillVar.insert(*v);
                                ++i;
                            }
                        }
                    }
                }
            }

            int getSpillNum() { return spillVar.size(); }
        };

        //自顶向下寄存器压力追踪器！
        struct TopDownRegPressTracker {
            //记录溢出变量顺序
            vector<int> spillNum;
            //记录info更新顺序
            vector<infoChange> infoChangedLog;
            BasicblockRegPressTracker &bbrt;

            TopDownRegPressTracker(infoChange &livenessInfo, BasicblockRegPressTracker &bbrt) : bbrt(bbrt) {
                bbrt.updateSpillVarNum(livenessInfo);
                infoChangedLog.push_back(livenessInfo);
                spillNum.push_back(bbrt.spillVar.size());
            }

            void tryMoveDown(Instruction *top, Instruction *bottom, bool isPrint = false) {
                infoChange newChange = infoChangedLog.back();
                set<Value *> tempSet;
                //调换以后，本来在上面的指令的livein变成的最终的liveout加上自己的use再减去def
                newChange[top].liveOut.swap(newChange[bottom].liveOut);
                newChange[bottom].liveIn.swap(newChange[top].liveIn);
                //现在top在下面了
                tempSet = newChange[top].liveOut;
                for (auto u = top->getBegin(); u != top->getEnd(); ++u) tempSet.insert((*u)->value);
                if (auto res = top->getResult()) tempSet.erase(res);
                newChange[top].liveIn = tempSet;
                newChange[bottom].liveOut.swap(tempSet);
                LivenessAnalysis::calculateRegAndFReg(newChange);
                bbrt.updateSpillVarNum(newChange);
                infoChangedLog.push_back(newChange);
                spillNum.push_back(bbrt.getSpillNum());
                if (isPrint) {
                    cout << "----------------------------" << "第" << infoChangedLog.size() - 1 << "次调度尝试！"
                         << "----------------------------" << endl;
                    bottom->print();
                    cout << "  live in:  ";
                    for (auto v: newChange[bottom].liveIn) cout << v->toString() << "  ";
                    cout << "\n  live out:  ";
                    for (auto v: newChange[bottom].liveOut) cout << v->toString() << "  ";
                    cout << "\n----------------------------" << endl;
                    top->print();
                    cout << "  live in:  ";
                    for (auto v: newChange[top].liveIn) cout << v->toString() << "  ";
                    cout << "\n  live out:  ";
                    for (auto v: newChange[top].liveOut) cout << v->toString() << "  ";
                    cout << "\n溢出变量：";
                    for (auto v: bbrt.spillVar) cout << v->toString() << "  ";
                    cout << "\n\n" << endl;
                }

            }

            infoChange &getInfo(int x) {
                return infoChangedLog[x];
            }

            //返回最佳尝试次数
            int getBestTrys() {
                int min = 32767;
                int index{0};
                for (int i = 0; i < spillNum.size(); ++i) {
                    if (spillNum[i] < min) {
                        min = spillNum[i];
                        index = i;
                    }
                }
                return index;
            }
        };

        //自底向上寄存器压力追踪器！
        struct BottomUpRegPressTracker {
            //记录溢出变量顺序
            vector<int> spillNum;
            //记录info更新顺序
            vector<infoChange> infoChangedLog;
            BasicblockRegPressTracker &bbrt;

            BottomUpRegPressTracker(infoChange &livenessInfo, BasicblockRegPressTracker &bbrt) : bbrt(bbrt) {
                bbrt.updateSpillVarNum(livenessInfo);
                infoChangedLog.push_back(livenessInfo);
                spillNum.push_back(bbrt.spillVar.size());
            }

            //尝试向上移动指令并且追踪(更新)压力！
            //上下紧挨着的两条指令
            void tryMoveUp(Instruction *bottom, Instruction *top, bool isPrint = false) {
                infoChange newChange = infoChangedLog.back();
                set<Value *> tempSet;
                //调换以后，本来在上面的指令的livein变成的最终的liveout加上自己的use再减去def
                newChange[top].liveOut.swap(newChange[bottom].liveOut);
                newChange[bottom].liveIn.swap(newChange[top].liveIn);
                //现在top在下面了
                tempSet = newChange[top].liveOut;
                for (auto u = top->getBegin(); u != top->getEnd(); ++u) tempSet.insert((*u)->value);
                if (auto res = top->getResult()) tempSet.erase(res);


                newChange[top].liveIn = tempSet;
                newChange[bottom].liveOut.swap(tempSet);
                LivenessAnalysis::calculateRegAndFReg(newChange);
                bbrt.updateSpillVarNum(newChange);
                infoChangedLog.push_back(newChange);
                spillNum.push_back(bbrt.getSpillNum());
                if (isPrint) {
                    cout << "----------------------------" << "第" << infoChangedLog.size() - 1 << "次调度尝试！"
                         << "----------------------------" << endl;
                    bottom->print();
                    cout << "  live in:  ";
                    for (auto v: newChange[bottom].liveIn) cout << v->toString() << "  ";
                    cout << "\n  live out:  ";
                    for (auto v: newChange[bottom].liveOut) cout << v->toString() << "  ";
                    cout << "\n----------------------------" << endl;
                    top->print();
                    cout << "  live in:  ";
                    for (auto v: newChange[top].liveIn) cout << v->toString() << "  ";
                    cout << "\n  live out:  ";
                    for (auto v: newChange[top].liveOut) cout << v->toString() << "  ";
                    cout << "\n溢出变量：";
                    for (auto v: bbrt.spillVar) cout << v->toString() << "  ";
                    cout << "\n\n" << endl;
                }

            }

            infoChange &getInfo(int x) {
                return infoChangedLog[x];
            }

            //返回最佳尝试次数
            int getBestTrys() {
                int min = 32767;
                int index{0};
                for (int i = 0; i < spillNum.size(); ++i) {
                    if (spillNum[i] < min) {
                        min = spillNum[i];
                        index = i;
                    }
                }
                return index;
            }
        };


        LivenessAnalysis &livenessAnalysis;

        //小顶堆
        struct isSuccNull {
            bool operator()(SNode *first, SNode *second) {
                return first->succLeft > second->succLeft;
            }
        };

        struct isPredNull {
            bool operator()(SNode *first, SNode *second) {
                return first->predLeft > second->predLeft;
            }
        };

        //挑选先调度的节点
        //先比较寄存器压力，优先调度寄存器压力高的
        //若寄存器压力相同，比较指令延迟(还没做）
        struct nodeElect {
            bool operator()(SNode *first, SNode *second) {
                return first->regPress < second->regPress;
            }
        };

        //以后记得改！
        Instruction *topBoundary{nullptr}; //上边界
        Instruction *bottomBoundary{nullptr}; //下边界

        //顶部未就绪节点
        priority_queue<SNode *, vector<SNode *>, isPredNull> topNodes;
        //底部未就绪节点
        priority_queue<SNode *, vector<SNode *>, isSuccNull> bottomNodes;
        //顶部调度区域
        priority_queue<SNode *, vector<SNode *>, nodeElect> topScope;
        //底部调度区域
        priority_queue<SNode *, vector<SNode *>, nodeElect> bottomScope;
        //已调度节点
        set<SNode *> scheduled;
        //调度模式
        enum moveMode {
            TopDown, BottomUp
        };
        //当前块内指令-下标映射
        vector<Instruction *> instructions;
        map<Instruction *, int> instToIndex;
        //当前块内活跃信息
        map<Instruction *, LivenessInfo> livenessInfo;
        //映射 inst to sdnode
        map<Instruction *, SNode *> instToSNode;
        BasicBlock *basicBlock;
    public:
        ScheduleBRPass(LivenessAnalysis &la) : livenessAnalysis(la) {}

        void build(BasicBlock *basicBlock) {
            this->basicBlock = basicBlock;
            //构造DAG图和活跃信息
            livenessAnalysis.getBlockLivenessInfo(basicBlock, livenessInfo);
            int index = 0;
            //初始化所有SDNode
            //存储store
            bottomBoundary = &*basicBlock->getBack();
            vector<pair<int, SNode *>> storeList;
            set<Instruction *> scheduledInsts;
            for (auto i = basicBlock->getBegin(); i != basicBlock->getEnd(); ++i) {
                Instruction *inst = &*i;
                if (isa<AllocateInst>(inst) || isa<PhiInst>(inst)) {
                    topBoundary = inst;
                    continue;
                }
                if (inst == bottomBoundary) continue;
                SNode *snode = new SNode;
                instructions.push_back(inst);
                if (isa<StoreInst>(inst)) storeList.push_back({index, snode});
                instToIndex.insert({inst, index++});
                instToSNode.insert({inst, snode});
                snode->instruction = inst;
                scheduledInsts.insert(inst);
            }
            for (auto i = instructions.begin(); i != instructions.end(); ++i) {
                Instruction *inst = *i;
                SNode *snode = instToSNode[inst];
                if (!snode) cerr << "snode err!" << endl;
                //这块记得把load和store连起来
                //对于每一个load，通过查找最近的store
                if (LoadInst *li = dyn_cast<LoadInst>(inst)) {
                    int index = instToIndex[inst];
                    for (auto p = storeList.rbegin(); p != storeList.rend(); ++p) {
                        if (p->first >= index) continue;
                        SNode *storeNode = p->second;
                        StoreInst *si = cast<StoreInst>(storeNode->instruction);
                        if (si->getPtr() == li->getPtr() && p->first < index) {
                            SNode *loadNode = instToSNode[li];
                            loadNode->predChain = storeNode;
                            storeNode->succChain = loadNode;
                            break;
                        }
                    }
                } else if (StoreInst *si = dyn_cast<StoreInst>(inst)) {
                    int index = instToIndex[inst];
                    for (auto p = storeList.rbegin(); p != storeList.rend(); ++p) {
                        if (p->first >= index) continue;
                        SNode *storeNode2 = p->second;
                        StoreInst *si2 = cast<StoreInst>(storeNode2->instruction);
                        if (si->getPtr() == si2->getPtr() && p->first < index) {
                            SNode *storeNode = instToSNode[si];
                            storeNode2->succChain = storeNode;
                            storeNode->predChain = storeNode2;
                            break;
                        }
                    }
                }
                //前驱节点
                for (auto u = inst->getBegin(); u != inst->getEnd(); ++u) {
                    if (Instruction *def = (*u)->value->getDef()) {
                        if (!scheduledInsts.count(def)) continue;
                        SNode *pred = instToSNode[def];
                        snode->predLefts.insert(pred);
                        snode->predLeft++;
                        pred->succLefts.insert(snode);
                        pred->succLeft++;
                    }
                }
                //后继节点
                if (Value *result = inst->getResult())
                    for (auto u = result->getUsesBegin(); u != result->getUsesEnd(); ++u) {
                        Instruction *userInst = cast<Instruction>((*u).user);
                        if (!scheduledInsts.count(userInst)) continue;
                        auto succ = instToSNode[userInst];
                        snode->succLefts.insert(succ);
                        snode->succLeft++;
                        succ->predLefts.insert(snode);
                        succ->predLeft++;
                    }
                snode->predLeft = snode->predLefts.size();
                snode->succLeft = snode->succLefts.size();
                topNodes.push(snode);
                bottomNodes.push(snode);
                snode->regPress = livenessInfo[inst].regliveOut.size();
            }
            //上下边界以后记得改
            topBoundary = &*basicBlock->getBegin();
            bottomBoundary = &*basicBlock->getBack();
        }

        void schedule() {
            //构造顶部调度队列
            while (true) {
                if (topNodes.empty() && bottomNodes.empty() && topScope.empty() && bottomScope.empty()) break;
                if (topScope.empty() && bottomScope.empty()) {
                    //构造顶部调度队列
                    while (!topNodes.empty()) {
                        SNode *node = topNodes.top();
                        topNodes.pop();
                        if (node->predLeft) break;
                        if (scheduled.insert(node).second) topScope.push(node);
                    }
                    //构造底部调度队列
                    while (!bottomNodes.empty()) {
                        SNode *node = bottomNodes.top();
                        bottomNodes.pop();
                        if (node->succLeft) break;
                        if (scheduled.insert(node).second) bottomScope.push(node);
                    }
                }
                SNode *pickNode;
                moveMode mode;
                auto pickTopNode = [&]() -> SNode * {
                    SNode *node = topScope.top();;
                    topScope.pop();
                    mode = TopDown;
                    for (auto succ: node->succLefts) succ->predLeft--;
                    return node;
                };
                auto pickBottomNode = [&]() -> SNode * {
                    SNode *node = bottomScope.top();;
                    bottomScope.pop();
                    mode = BottomUp;
                    for (auto pred: node->predLefts) pred->succLeft--;
                    return node;
                };
                if (bottomScope.empty() && topScope.empty()) continue;
                else if (bottomScope.size() == 1) pickNode = pickBottomNode();
                else if (topScope.size() == 1) pickNode = pickTopNode();
                else if (!bottomScope.empty() && topScope.empty()) pickNode = pickBottomNode();
                else if (bottomScope.empty() && !topScope.empty()) pickNode = pickTopNode();
                else {
                    //从顶部和底部队列顶选择一个调度节点
                    SNode *topNode = topScope.top();
                    SNode *bottomNode = bottomScope.top();
                    //从两个节点中再选一个最终调度节点
                    nodeElect elect;
                    if (elect(topNode, bottomNode)) pickNode = pickBottomNode();
                    else pickNode = pickTopNode();
                }
                //调度选中指令
                moveNode(pickNode, mode);
            }
            //更新全局活跃变量信息
            map<Instruction *, LivenessInfo> &globalLivenessInfo = livenessAnalysis.getLivenessInfo();
            for (auto inst: instructions) globalLivenessInfo[inst] = livenessInfo[inst];
        }

    private:
        void moveNode(SNode *snode, moveMode mode) {
            BasicblockRegPressTracker bbrt;
            Instruction *sinst = snode->instruction;//当前调度指令
            if (isa<StoreInst>(sinst)) return;
            //自底向上移动
            if (mode == BottomUp) {
                //自底向上追踪器初始化
                BottomUpRegPressTracker burt(livenessInfo, bbrt);
                //获取当前指令前驱
                set<Instruction *> defInsts;
                if (snode->predLefts.empty()) return;
                for (auto pred = snode->predLefts.begin(); pred != snode->predLefts.end(); ++pred)
                    defInsts.insert((*pred)->instruction);
                if (snode->predChain) defInsts.insert(snode->predChain->instruction);
                //尝试自底向上调度，用追踪器追踪并记录溢出变量变化
                auto targetInst = sinst->getPrev();
                while (targetInst->getPrev()) {
                    if (targetInst == topBoundary || defInsts.count(targetInst)) break;
                    burt.tryMoveUp(sinst, targetInst, 0);
                    targetInst = targetInst->getPrev();
                }
                //获得最佳调度次数并且实施
                int tryNums = burt.getBestTrys();
                if (!tryNums) return;
                targetInst = sinst;
                for (int i = 0; i < tryNums; ++i) targetInst = targetInst->getPrev();
                sinst->removeFromParent();
                basicBlock->insertIntoChild(sinst, targetInst);
                //更新活跃信息和instructions
                auto info = burt.getInfo(tryNums);
                livenessInfo.swap(info);
            }
            if (mode == TopDown) {
                TopDownRegPressTracker tdrt(livenessInfo, bbrt);
                //int currentIndex = instToIndex[sinst];  //当前调度指令下标
                set<Instruction *> useInsts; //获得当前指令后继
                if (snode->succLefts.empty()) return;
                for (auto succ = snode->succLefts.begin(); succ != snode->succLefts.end(); ++succ)
                    useInsts.insert((*succ)->instruction);
                if (snode->succChain) useInsts.insert(snode->succChain->instruction);
                auto targetInst = sinst->getNext();
                //int index = currentIndex + 1;
                while (targetInst->getNext()) {
                    if (targetInst == bottomBoundary || useInsts.count(targetInst)) break;
                    tdrt.tryMoveDown(sinst, targetInst, 0);
                    targetInst = targetInst->getNext();
                }
                //获得最佳调度次数并且实施
                int tryNums = tdrt.getBestTrys();
                if (!tryNums) return;
                targetInst = sinst;
                for (int i = 0; i < tryNums; ++i) targetInst = targetInst->getNext();
                sinst->removeFromParent();
                basicBlock->insertIntoChild(sinst, targetInst->getNext());
                auto info = tdrt.getInfo(tryNums);
                livenessInfo.swap(info);
            }
        }
    };
}


#endif //ANUC_SCHEDULEBEFORERA_H
