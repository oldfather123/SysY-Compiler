// Referrence from Compiler-2023-YatCC:
// https://gitlab.eduxiji.net/educg-group-17291-1894922/202310558201558-3109/-/blob/main/src/opt/mem2reg.cpp

#include "mem2reg.h"
#include "CFGAnalysis.h"
#include "domTreeAnalysis.h"
#include <chrono>

struct valBB {
    ValuePtr val;
    BasicBlockPtr BB;
    valBB(ValuePtr iv, BasicBlockPtr iBB) : val{ iv }, BB{ iBB } {};
};

void mem2reg(FunctionPtr func, DominateAnalysisResult& dom) {
    for(int i = 0; i < func->getBasicBlocks().size(); i++) {
        if(!dom.reachable(func->getBasicBlock(i))) {
            func->getBasicBlock(i)->eraseFromParent();
        }
    }
    auto cfg = runCFGAnalysis(func);
    // cout << "start dfs" << endl;
    auto dfMap = computeDominanceFrontier(dom, cfg);
    // cout << "end dfs" << endl;
    auto entry = func->getEntryBlock();
    unordered_map<ValuePtr, vector<BasicBlockPtr>> defBB;
    unordered_map<ValuePtr, vector<BasicBlockPtr>> useBB;

    // 去除对应alloca的entry
    // 可以被promote的alloca是只被load和store使用的alloca，在该比赛中就是float和int，实际上ptr应该也不会出现，但是我忘了为啥这样写了，就留着吧
    vector<InstructionPtr> newEntryInst;
    for(auto& inst : entry->instructionsRef()) {
        if(inst->insId == InsID::Alloca) {
            // auto tI = ((AllocaInstruction*)(inst.get()));
            if(inst->type->isFloat() || inst->type->isInt() || inst->type->isPtr()) {
                // 记录需要删除的alloca变量
                defBB[inst.get()] = {};
                useBB[inst.get()] = {};
            } else {
                newEntryInst.emplace_back(inst.get());
            }
        } else {
            newEntryInst.emplace_back(inst.get());
        }
    }
    // 记录store的块
    for(auto& bb : (func->basicBlocks)) {
        for(auto& inc : (bb->instructionsRef())) {
            if(inc->insId == InsID::Store && defBB.find(((StoreInstruction*)(inc.get()))->getPtr()) != defBB.end()) {
                defBB[((StoreInstruction*)(inc.get()))->getPtr()].emplace_back(bb.get());
            }
        }
    }
    // 记录load的块
    for(auto& bb : (func->basicBlocks)) {
        for(auto& inc : (bb->instructionsRef())) {
            if(inc->insId == InsID::Load && useBB.find(((LoadInstruction*)(inc.get()))->getPtr()) != useBB.end()) {
                useBB[((LoadInstruction*)(inc.get()))->getPtr()].emplace_back(bb.get());
            }
        }
    }

    unordered_map<BasicBlockPtr, ValuePtr> inworkList;

    // 记录在哪些基本块中插入哪些变量的phi
    unordered_map<BasicBlockPtr, ValuePtr> inserted;
    for(auto& bb : func->getBasicBlocks()) {
        inworkList[bb.get()] = nullptr;
        inserted[bb.get()] = nullptr;
    }
    queue<BasicBlockPtr> workList;

    // A variable (virtual register) is live at some point in the program if it has
    //  previously been defined by an instruction and will be used by an
    //  instruction in the future. It is dead otherwise.

    // 对于每个store过的变量
    for(auto& valDef : defBB) {
        queue<BasicBlockPtr> liveInBBWorkList;
        unordered_map<BasicBlockPtr, bool> liveInBB;
        // 从vector变成unordered_map提升效率    定义(Store)该变量的基本块
        unordered_map<BasicBlockPtr, bool> defMap;
        for(int i = 0; i < valDef.second.size(); i++) {
            defMap[valDef.second[i]] = true;
        }

        // 对每个使用load过这个变量的bb
        for(auto& use : (useBB[valDef.first])) {
            // 如果同个基本块内没有store的话，则放入liveInBBWorkList里面
            if(defMap.find(use) != defMap.end()) {
                bool flag = true;
                for(auto& I : (use->instructionsRef())) {
                    // 如果先遇到load该变量的指令，则放入liveinbbworklist
                    // 如果先遇到store该变量的指令，则不需要放入，因为之后用的就是该store的值了
                    // 如果啥都没遇到，则不可能(
                    if(I->insId == InsID::Store) {
                        auto tempI = (StoreInstruction*)(I.get());
                        if(tempI->getPtr() != valDef.first) {
                            continue;
                        }
                        flag = false;
                        break;
                    } else if(I->insId == InsID::Load) {
                        auto tempI = (LoadInstruction*)(I.get());
                        if(tempI->getPtr() != valDef.first) {
                            continue;
                        }
                        break;
                    }
                }
                if(flag) {
                    liveInBBWorkList.push(use);
                }
            } else {
                liveInBBWorkList.push(use);
            }
        }
        // 将有use和没有use的livebb找出来
        while(!liveInBBWorkList.empty()) {
            auto tempBB = liveInBBWorkList.front();
            liveInBBWorkList.pop();
            if(liveInBB.find(tempBB) != liveInBB.end()) {
                continue;
            }
            liveInBB[tempBB] = true;
            for(auto& pred : (tempBB->predBasicBlocks)) {
                // 如果没有use但有def的，不是livebb
                if(defMap.find(pred) != defMap.end()) {
                    continue;
                }
                liveInBBWorkList.push(pred);
            }
        }

        for(auto& t : (valDef.second)) {
            inworkList[t] = valDef.first;
            workList.push(t);
        }
        // unordered_map<BasicBlockPtr,bool> finish;

        while(!workList.empty()) {
            auto now = workList.front();
            workList.pop();
            // cerr<<"block :"<<now->label->name<<endl;

            for(auto& df : dfMap[now]) {
                // cerr<<"df: "<<df.first->label->name<<endl;
                if(inserted[df] != valDef.first && liveInBB.find(df) != liveInBB.end()) {

                    auto mBegin = df->instructionsRef().begin();
                    auto phi = InstructionPtr(new PhiInstruction(valDef.first));
                    phi->setBasicBlock(df);
                    df->instructionsRef().emplace(mBegin, phi);
                    inserted[df] = valDef.first;
                    // finish[df.first] = true;
                    if(inworkList[df] != valDef.first) {
                        inworkList[df] = valDef.first;
                        workList.push(df);
                    }
                }
            }
        }
    }

    // 每个变量一个stack
    unordered_map<ValuePtr, vector<valBB>> stak;
    for(auto& varDef : defBB) {
        // 默认初始化为0
        if(varDef.first->type->isInt())
            stak[varDef.first] = { valBB(Const::getConst(Type::getInt(), 0), entry) };
        else {
            stak[varDef.first] = { valBB(Const::getConst(Type::getFloat(), float(0)), entry) };
        }
    }

    stack<BasicBlockPtr> list;
    vector<InstructionPtr> newInst;
    list.push(entry);

    unordered_map<BasicBlockPtr, bool> visited;
    visited[entry] = true;

    while(!list.empty()) {
        auto bb = list.top();
        list.pop();
        newInst.clear();
        for(auto& inst : (bb->instructionsRef())) {
            if(inst->insId == InsID::Load) {
                auto to = inst.get();
                auto from = dynamic_cast<LoadInstruction*>(inst.get())->getPtr();
                // cerr<<"load:   "<<to->name<<" "<<from->name<<endl;
                if(stak.find(from) != stak.end()) {
                    int stakIdx = stak[from].size() - 1;
                    while(!dom.dominate(stak[from][stakIdx].BB, bb)) {
                        stakIdx--;
                    }
                    //  TODO: might contain bug
                    to->replaceWith(stak[from][stakIdx].val);
                } else {
                    newInst.emplace_back(inst.get());
                }
            }
            // 定义
            else if(inst->insId == InsID::Store) {
                auto des = ((StoreInstruction*)(inst.get()))->getPtr();
                auto value = ((StoreInstruction*)(inst.get()))->getValue();
                if(stak.find(des) != stak.end()) {
                    stak[des].emplace_back(valBB(value, bb));
                    // TODO: might contain bug
                } else {
                    newInst.emplace_back(inst.get());
                }
            }
            // 定义
            else if(inst->insId == InsID::Phi) {
                auto val = ((PhiInstruction*)(inst.get()))->val;
                auto reg = inst.get();
                // cerr<<"phi:   "<<reg->name<<" "<<val->name<<endl;
                if(stak.find(val) != stak.end()) {
                    stak[val].emplace_back(valBB(reg, bb));
                }
                newInst.emplace_back(inst.get());
                // phi指令不用删除
            } else {
                newInst.emplace_back(inst.get());
            }
        }

        for(auto succ : bb->succBasicBlocks) {
            // cerr<<succ.first->label->name<<endl;
            for(auto& ins : succ->instructionsRef()) {
                if(ins->insId == InsID::Phi) {
                    auto tempIns = (PhiInstruction*)(ins.get());
                    auto val = tempIns->val;
                    auto reg = tempIns;

                    if(stak.find(val) != stak.end()) {
                        int stakIdx = stak[val].size() - 1;
                        while(!dom.dominate(stak[val][stakIdx].BB, bb)) {
                            stakIdx--;
                        }
                        auto newVal = stak[val][stakIdx].val;
                        tempIns->addIncoming(bb, newVal);
                    }
                } else {
                    break;
                }
            }
        }

        bb->setInstructions(newInst);
        for(auto& succ : (bb->succBasicBlocks)) {
            if(visited.find(succ) == visited.end()) {
                list.push(succ);
                visited[succ] = true;
            }
        }
    }
    // entry->setInstructions(newEntryInst);
    for(auto it = entry->instructionsRef().begin(); it != entry->instructionsRef().end();) {
        if((*it)->insId == InsID::Alloca) {
            if((*it)->type->isFloat() || (*it)->type->isInt() || (*it)->type->isPtr()) {
                it = entry->instructionsRef().erase(it);
            } else {
                it++;
            }
        } else {
            it++;
        }
    }
}
void runMem2Reg(Module& mod) {
    cout << YELLOW <<"Begin mem2reg"<< RESET << endl;
    auto start = std::chrono::high_resolution_clock::now();
    for(auto& func : mod.globalFunctions) {
        if(func->isLib)
            continue;
        auto cfg = runCFGAnalysis(func.get());
        auto dom = runDominateTreeAnalysis(func.get(), cfg);
        mem2reg(func.get(), dom);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << GREEN << "Mem2reg finished in " << RESET  << duration.count() << " ms"<< endl;
}