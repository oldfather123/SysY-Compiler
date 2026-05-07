#include "riscv.h"

struct BaseOffs {
    MOperand base;
    MOperand offs;
    BaseOffs(MOperand base, MOperand offs) : base(base), offs(offs) {}
    bool operator<(const BaseOffs& other) const {
        return base < other.base || offs < other.offs;
    }
    bool operator==(const BaseOffs& other) const {
        return base == other.base && offs == other.offs;
    }
};

void instructionReorder(MProg* mProj) {
    for(auto mFunc : mProj->mFuncs) {
        for(auto mb : mFunc->mBlocks) {
            map<MInst*, vector<MInst*>> adjList;
            map<MInst*, int> inDegree;
            queue<MInst*> instQue;
            map<MOperand, MInst*> defInst;
            map<MOperand, MInst*> useInst;
            map<BaseOffs, MInst*> ldrInst;
            map<BaseOffs, MInst*> strInst;
            vector<MInst*> window;
            vector<MInst*> pushInst;
            vector<MInst*> popInst;
            vector<MInst*> newOrder;
            int windowSize = 10;
            for(auto mi = mb->firInst; mi; mi = mi->next) {
                if(mi->tag == MI_FUNC_CALL) {
                    goto nextBlock;
                } else if(mi->tag == MI_PUSH || mi->tag == MI_FPUSH) {
                    pushInst.push_back(mi);
                } else if(mi->tag == MI_POP || mi->tag == MI_FPOP) {
                    popInst.push_back(mi);
                } else if(mi->tag != MI_BRANCH && mi->tag != MI_RETURN) {
                    for(auto def : getDefs(mi)) {
                        if(defInst.find(def) != defInst.end()) {
                            adjList[defInst[def]].push_back(mi);
                            inDegree[mi]++;
                        }
                        if(useInst.find(def) != useInst.end()) {
                            adjList[useInst[def]].push_back(mi);
                            inDegree[mi]++;
                        }
                    }
                    for(auto use : getUses(mi)) {
                        if(defInst.find(use) != defInst.end()) {
                            adjList[defInst[use]].push_back(mi);
                            inDegree[mi]++;
                        }
                        if(useInst.find(use) != useInst.end()) {
                            adjList[useInst[use]].push_back(mi);
                            inDegree[mi]++;
                        }
                    }
                    for(auto def : getDefs(mi)) {
                        defInst[def] = mi;
                    }
                    for(auto use : getUses(mi)) {
                        useInst[use] = mi;
                    }
                    if(auto ldrMI = dynamic_cast<MI_Load*>(mi)) {
                        BaseOffs baseOffs(ldrMI->base, ldrMI->offset);
                        if(strInst.find(baseOffs) != strInst.end()) {
                            adjList[strInst[baseOffs]].push_back(mi);
                            inDegree[mi]++;
                        }
                        if(ldrInst.find(baseOffs) != ldrInst.end()) {
                            adjList[ldrInst[baseOffs]].push_back(mi);
                            inDegree[mi]++;
                        }
                        ldrInst[baseOffs] = mi;
                        if(ldrMI->isRv64 && ldrMI->offset.isImm()) {
                            BaseOffs nextBaseOffs(ldrMI->base, makeImm(ldrMI->offset.value + 4));
                            if(strInst.find(nextBaseOffs) != strInst.end()) {
                                adjList[strInst[nextBaseOffs]].push_back(mi);
                                inDegree[mi]++;
                            }
                            if(ldrInst.find(nextBaseOffs) != ldrInst.end()) {
                                adjList[ldrInst[nextBaseOffs]].push_back(mi);
                                inDegree[mi]++;
                            }
                            ldrInst[nextBaseOffs] = mi;
                        }
                    } else if(auto fldrMI = dynamic_cast<MI_FLoad*>(mi)) {
                        BaseOffs baseOffs(fldrMI->base, fldrMI->offset);
                        if(strInst.find(baseOffs) != strInst.end()) {
                            adjList[strInst[baseOffs]].push_back(mi);
                            inDegree[mi]++;
                        }
                        if(ldrInst.find(baseOffs) != ldrInst.end()) {
                            adjList[ldrInst[baseOffs]].push_back(mi);
                            inDegree[mi]++;
                        }
                        ldrInst[baseOffs] = mi;
                        if(fldrMI->isRv64 && fldrMI->offset.isImm()) {
                            BaseOffs nextBaseOffs(fldrMI->base, makeImm(fldrMI->offset.value + 4));
                            if(strInst.find(nextBaseOffs) != strInst.end()) {
                                adjList[strInst[nextBaseOffs]].push_back(mi);
                                inDegree[mi]++;
                            }
                            if(ldrInst.find(nextBaseOffs) != ldrInst.end()) {
                                adjList[ldrInst[nextBaseOffs]].push_back(mi);
                                inDegree[mi]++;
                            }
                            ldrInst[nextBaseOffs] = mi;
                        }
                    } else if(auto strMI = dynamic_cast<MI_Store*>(mi)) {
                        BaseOffs baseOffs(strMI->base, strMI->offset);
                        if(strInst.find(baseOffs) != strInst.end()) {
                            adjList[strInst[baseOffs]].push_back(mi);
                            inDegree[mi]++;
                        }
                        if(ldrInst.find(baseOffs) != ldrInst.end()) {
                            adjList[ldrInst[baseOffs]].push_back(mi);
                            inDegree[mi]++;
                        }
                        strInst[baseOffs] = mi;
                        if(strMI->isRv64 && strMI->offset.isImm()) {
                            BaseOffs nextBaseOffs(strMI->base, makeImm(strMI->offset.value + 4));
                            if(strInst.find(nextBaseOffs) != strInst.end()) {
                                adjList[strInst[nextBaseOffs]].push_back(mi);
                                inDegree[mi]++;
                            }
                            if(ldrInst.find(nextBaseOffs) != ldrInst.end()) {
                                adjList[ldrInst[nextBaseOffs]].push_back(mi);
                                inDegree[mi]++;
                            }
                            strInst[nextBaseOffs] = mi;
                        }
                    } else if(auto fstrMI = dynamic_cast<MI_FStore*>(mi)) {
                        BaseOffs baseOffs(fstrMI->base, fstrMI->offset);
                        if(strInst.find(baseOffs) != strInst.end()) {
                            adjList[strInst[baseOffs]].push_back(mi);
                            inDegree[mi]++;
                        }
                        if(ldrInst.find(baseOffs) != ldrInst.end()) {
                            adjList[ldrInst[baseOffs]].push_back(mi);
                            inDegree[mi]++;
                        }
                        strInst[baseOffs] = mi;
                        if(fstrMI->isRv64 && fstrMI->offset.isImm()) {
                            BaseOffs nextBaseOffs(fstrMI->base, makeImm(fstrMI->offset.value + 4));
                            if(strInst.find(nextBaseOffs) != strInst.end()) {
                                adjList[strInst[nextBaseOffs]].push_back(mi);
                                inDegree[mi]++;
                            }
                            if(ldrInst.find(nextBaseOffs) != ldrInst.end()) {
                                adjList[ldrInst[nextBaseOffs]].push_back(mi);
                                inDegree[mi]++;
                            }
                            strInst[nextBaseOffs] = mi;
                        }
                    }
                    window.push_back(mi);
                }
                // if(window.size() == windowSize) {
                //     for(auto mi : window) {
                //         if(inDegree[mi] == 0) {
                //             instQue.push(mi);
                //         }
                //     }
                //     while(!instQue.empty()) {
                //         auto mi = instQue.front();
                //         newOrder.push_back(mi);
                //         instQue.pop();
                //         for(auto nextMI : adjList[mi]) {
                //             --inDegree[nextMI];
                //             if(inDegree[nextMI] == 0) {
                //                 instQue.push(nextMI);
                //             }
                //         }
                //     }
                //     adjList.clear();
                //     inDegree.clear();
                //     defInst.clear();
                //     useInst.clear();
                //     ldrInst.clear();
                //     strInst.clear();
                //     window.clear();
                // }
            }
            if(window.size() > 0) {
                for(auto mi : window) {
                    if(inDegree[mi] == 0) {
                        instQue.push(mi);
                    }
                }
                while(!instQue.empty()) {
                    auto mi = instQue.front();
                    newOrder.push_back(mi);
                    instQue.pop();
                    for(auto nextMI : adjList[mi]) {
                        --inDegree[nextMI];
                        if(inDegree[nextMI] == 0) {
                            instQue.push(nextMI);
                        }
                    }
                }
            }
            mb->cleanEveryButTerm();
            for(auto mi : pushInst) {
                mb->push(mi);
            }
            for(auto mi : newOrder) {
                mb->push(mi);
            }
            for(auto mi : popInst) {
                mb->push(mi);
            }
        nextBlock:
            continue;
        }
    }
}