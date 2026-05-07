#include "mem2reg.h"


// 本文件中与支配树相关的部分代码，参考了洛谷大佬@haochengw920大佬的实现，详见README.md

// 首先给逆后序遍历（真的吗）的方式每个basicblock一个序号，并且后序将这个visit数组，给一个basicblock到int的逆映射
void dfsTree(BasicBlockPtr bb,vector<BasicBlockPtr>&dfsNum, unordered_map<BasicBlockPtr, int>& visit){
    if(visit.count(bb)) return;
    visit[bb] = visit.size() - 1;
    for(auto& succ:(bb->succBasicBlocks)){
        if(!visit.count(succ)){
            dfsTree(succ, dfsNum, visit);
        }
    }
    dfsNum.push_back(bb);
}

#define N 4000
#define MAXM 500000
#define MAXN 4000
int n, m; // 每次会在domTree里面设定
int fa[MAXN], sum[MAXN];
int counter, id[MAXN], dfn[MAXN];
int sdom[MAXN], idom[MAXN];
vector<int> vec[MAXN];

class Graph {
private:
	struct Edge {
		int to, nex;
	} edge[MAXM];
	int idx = 0;
public:
	int head[MAXN];
	inline void addEdge(int u, int v) {
		edge[++idx].to = v;
		edge[idx].nex = head[u];
		head[u] = idx;
	}
	inline Edge operator [] (const int& x) const {
		return edge[x];
	}
    inline void reset(){
        for(int i=0;i<MAXN;i++){
            head[i]=0;
        }
        idx=0;
    }
}G, invG, directDomT;

inline void dfs(int u) {
	id[dfn[u] = ++counter] = u;
	for (int i = G.head[u]; i; i = G[i].nex) {
		int v = G[i].to;
		if (dfn[v]) continue;
		fa[v] = u; 
        dfs(v);
	}
}
struct Dsu {
	int fa[MAXN], mn[MAXN];
	inline Dsu() {
		for (int i = 1; i < MAXN; ++i)
			fa[i] = mn[i] = i;
	}
	inline int pushll(int x) {
		if (x == fa[x]) return x;
		int tmp = pushll(fa[x]);
		if (dfn[sdom[mn[fa[x]]]] < dfn[sdom[mn[x]]]) mn[x] = mn[fa[x]];
		return fa[x] = tmp;
	}
    inline void reset(){
        for(int i=0;i<MAXN;i++){
            fa[i]=i;
            mn[i]=i;
        }
    }
}d;

void clearAll() {
	for(int i=0;i<MAXN;i++){
        fa[i]=0;
        sum[i]=0;
        id[i]=0;
        dfn[i]=0;
        sdom[i]=0;
        idom[i]=0;
        vec[i].clear();
    }
    counter=0;
    G.reset();
    invG.reset();
    directDomT.reset();
    d.reset();
}

void Calc(int u, vector<BasicBlockPtr> &dfsNum) {
	for (int i = directDomT.head[u]; i; i = directDomT[i].nex) {
		int v = directDomT[i].to;
        dfsNum[u]->dominatedBlocks.insert(dfsNum[v]);
        dfsNum[v]->immediateDominator = dfsNum[u];
		Calc(v, dfsNum);
	}
}
inline void Solve(vector<BasicBlockPtr> &dfsNum) {
	dfs(1);
	for (int i = 1; i <= n; ++i) sdom[i] = i;
	for (int t = counter; t; --t) {
		int u = id[t];
		for (int v : vec[u]) {
			d.pushll(v);
			if (sdom[d.mn[v]] == u) idom[v] = u;
			else idom[v] = d.mn[v];
		}
		if (t == 1) continue;
		for (int i = invG.head[u]; i; i = invG[i].nex) {
			int v = invG[i].to;
			if (dfn[v] < dfn[sdom[u]]) sdom[u] = v;
			else if (dfn[v] > dfn[u]) {
				d.pushll(v);
				if (dfn[sdom[d.mn[v]]] < dfn[sdom[u]]) sdom[u] = sdom[d.mn[v]];

			}
		}
		vec[sdom[u]].push_back(u);
		d.fa[u] = fa[u];
	}
	for (int t = 2; t <= counter; ++t)
		if (idom[id[t]] != sdom[id[t]])
			idom[id[t]] = idom[idom[id[t]]];
	for (int i = 2; i <= n; ++i) directDomT.addEdge(idom[i], i);
	Calc(1, dfsNum);
}

// DFS计算能支配一个基本块的所有基本块
static unordered_set<BasicBlockPtr> doms;
void computeAllDoms(BasicBlockPtr bb) {
	doms.insert(bb);
	bb->allDominators = doms;
	for (auto next : bb->dominatedBlocks)
		computeAllDoms(next);
	doms.erase(bb);
}

void clearAllDoms(FunctionPtr func) {
    for(auto bb: func->basicBlocks) {
        bb->allDominators.clear();;
    }
}

void print_current_time(const std::string& label) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time);
    std::cerr << label << ": "
              << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S")
              << std::endl;
}

void domTree(FunctionPtr func){
    // auto start = std::chrono::high_resolution_clock::now();
    // print_current_time("Start time");

	for (auto bb : func->basicBlocks) {
		bb->immediateDominator = nullptr;
		bb->dominatedBlocks.clear();
		bb->dominanceFrontier.clear();
	}
    clearAll(); // 记得每次进来都要先清除，不然会出问题

	vector<BasicBlockPtr> dfsNum;
	dfsNum.push_back(nullptr);
	unordered_map<BasicBlockPtr, int> num2block;
	dfsTree(func->basicBlocks[0], dfsNum, num2block);
	dfsNum.push_back(nullptr);
	reverse(dfsNum.begin(), dfsNum.end());
	dfsNum.pop_back();
	num2block.clear();
	for (int i = 1; i < dfsNum.size(); i++) {
		num2block[dfsNum[i]] = i;
	}

	int blockN = dfsNum.size()-1;
	n = blockN;
	m = 0;
	
	for (int i = 1; i <= blockN; i++) {
		for (auto& pred : (dfsNum[i]->predBasicBlocks)) {
			m++;
			invG.addEdge(i, num2block[pred]);
			G.addEdge(num2block[pred], i);
		}
	}
	Solve(dfsNum);

    // auto end1 = std::chrono::high_resolution_clock::now();
    // print_current_time("End1 time");
    // auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start);
    // std::cerr << "First part time difference: " << duration1.count() << " milliseconds" << std::endl;

    //计算DF
    for(int i=1;i<dfsNum.size();i++){
        if(dfsNum[i]->predBasicBlocks.size()>=2){
            for(auto &pred:(dfsNum[i]->predBasicBlocks)){
                auto runner = pred;
                while(runner!=dfsNum[i]->immediateDominator){
                    runner->dominanceFrontier.insert(dfsNum[i]);
                    if(runner->immediateDominator){
                        runner = runner->immediateDominator;
                    }
                    else{
                        break;
                    }
                }
            }
        }
    }

    clearAllDoms(func);
    computeAllDoms(func->getEntryBlock());

    // auto end3 = std::chrono::high_resolution_clock::now();
    // print_current_time("All time");
    // auto duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start);
    // std::cerr << "All time difference: " << duration3.count() << " milliseconds" << std::endl;
}


bool isDominatorOf(BasicBlockPtr dominator, BasicBlockPtr block, BasicBlockPtr entry) {
    // dominator 必须是 entry 或者 dominator 和 block 之间存在某种直接支配关系
    while (block != nullptr && block != entry) {
        if (block == dominator) {
            return true;
        }
        block = block->immediateDominator;
    }
    return dominator == entry;
}


void removeUnusedBasicBlocks(FunctionPtr func) {
    // 使用集合记录需要保留的基本块
    std::unordered_set<BasicBlockPtr> validBlocks;
    validBlocks.insert(func->basicBlocks[0]);

    // 队列用于递归删除没有前驱的基本块
    std::queue<BasicBlockPtr> blocksToDelete;

    // 遍历基本块，记录所有有前驱的基本块
    for (int i = 1; i < func->basicBlocks.size(); i++) {
        auto& bb = func->basicBlocks[i];
        if (!bb->predBasicBlocks.empty()) {
            validBlocks.insert(bb);
        }
        else {
            blocksToDelete.push(bb);
        }
    }

    // 递归删除没有前驱的基本块
    while (!blocksToDelete.empty()) {
        BasicBlockPtr bb = blocksToDelete.front();
        blocksToDelete.pop();
        
        for (auto& succ : bb->succBasicBlocks) {
            succ->predBasicBlocks.erase(bb);
            if (succ->predBasicBlocks.empty() && validBlocks.find(succ) == validBlocks.end()) {
                blocksToDelete.push(succ);
            }
        }
    }

    // 更新函数的基本块列表
    func->basicBlocks.erase(
        std::remove_if(func->basicBlocks.begin(), func->basicBlocks.end(),
            [&validBlocks](BasicBlockPtr bb) { return validBlocks.find(bb) == validBlocks.end(); }),
        func->basicBlocks.end()
    );
}


struct ValueBlockPair {
    ValuePtr val;
    BasicBlockPtr bb;
    ValueBlockPair(ValuePtr val, BasicBlockPtr bb) : val{val}, bb{bb} {}
};


void mem2reg(FunctionPtr func) {
    removeUnusedBasicBlocks(func);

    auto entry = func->basicBlocks[0];
    unordered_map<ValuePtr, vector<BasicBlockPtr>> definitionBlocks;
    unordered_map<ValuePtr, vector<BasicBlockPtr>> usageBlocks;

    // 处理entry块，去除对应alloca的指令
    vector<InstructionPtr> newEntryInstructions;
    for (auto& inst : entry->instructions) {
        if (inst->type == Alloca) {
            auto allocInst = static_cast<AllocaInstruction*>(inst.get());
            if (allocInst->des->type->isFloat() || allocInst->des->type->isInt() || allocInst->des->type->isPtr()) {
                definitionBlocks[allocInst->des] = {};
                usageBlocks[allocInst->des] = {};
            }
            else {
                newEntryInstructions.emplace_back(inst);
            }
        }
        else {
            newEntryInstructions.emplace_back(inst);
        }
    }
    entry->instructions = newEntryInstructions;

    // 记录store和load的块
    for (auto& bb : func->basicBlocks) {
        for (auto& inst : bb->instructions) {
            if (inst->type == Store) {
                auto storeInst = static_cast<StoreInstruction*>(inst.get());
                if (definitionBlocks.find(storeInst->des) != definitionBlocks.end()) {
                    definitionBlocks[storeInst->des].emplace_back(bb);
                }
            }
            else if (inst->type == Load) {
                auto loadInst = static_cast<LoadInstruction*>(inst.get());
                if (usageBlocks.find(loadInst->from) != usageBlocks.end()) {
                    usageBlocks[loadInst->from].emplace_back(bb);
                }
            }
        }
    }

    unordered_map<BasicBlockPtr, ValuePtr> workListEntries;
    unordered_map<BasicBlockPtr, ValuePtr> insertedPhis;
    queue<BasicBlockPtr> workList;

    // 对每个store过的变量
    for (auto& valDef : definitionBlocks) {
        queue<BasicBlockPtr> liveInBlocksQueue;
        unordered_map<BasicBlockPtr, bool> liveInBlocks;
        unordered_map<BasicBlockPtr, bool> defMap;

        for (auto& bb : valDef.second) {
            defMap[bb] = true;
        }

        for (auto& use : usageBlocks[valDef.first]) {
            bool needsLiveIn = true;
            for (auto& inst : use->instructions) {
                if (inst->type == Store) {
                    auto storeInst = static_cast<StoreInstruction*>(inst.get());
                    if (storeInst->des == valDef.first) {
                        needsLiveIn = false;
                        break;
                    }
                }
                else if (inst->type == Load) {
                    auto loadInst = static_cast<LoadInstruction*>(inst.get());
                    if (loadInst->from == valDef.first) {
                        break;
                    }
                }
            }
            if (needsLiveIn) {
                liveInBlocksQueue.push(use);
            }
        }

        while (!liveInBlocksQueue.empty()) {
            auto tempBB = liveInBlocksQueue.front();
            liveInBlocksQueue.pop();
            if (liveInBlocks.find(tempBB) != liveInBlocks.end()) {
                continue;
            }
            liveInBlocks[tempBB] = true;
            for (auto& pred : tempBB->predBasicBlocks) {
                if (defMap.find(pred) == defMap.end()) {
                    liveInBlocksQueue.push(pred);
                }
            }
        }

        for (auto& bb : valDef.second) {
            workListEntries[bb] = valDef.first;
            workList.push(bb);
        }

        while (!workList.empty()) {
            auto currentBB = workList.front();
            workList.pop();
            for (auto& df : currentBB->dominanceFrontier) {
                if (insertedPhis[df] != valDef.first && liveInBlocks.find(df) != liveInBlocks.end()) {
                    auto phi = make_shared<PhiInstruction>(df, valDef.first);
                    df->instructions.insert(df->instructions.begin(), phi);
                    insertedPhis[df] = valDef.first;
                    if (workListEntries[df] != valDef.first) {
                        workListEntries[df] = valDef.first;
                        workList.push(df);
                    }
                }
            }
        }
    }

    unordered_map<ValuePtr, vector<ValueBlockPair>> stackMap;

    for (auto& varDef : definitionBlocks) {
        auto initialVal = varDef.first->type->isInt() ? Const::createConstant(Type::getInt(), 0) : Const::createConstant(Type::getFloat(), 0.0f);
        stackMap[varDef.first] = {ValueBlockPair(initialVal, entry)};
    }

    stack<BasicBlockPtr> basicBlockStack;
    unordered_map<BasicBlockPtr, bool> visitedBlocks;
    basicBlockStack.push(entry);
    visitedBlocks[entry] = true;

    while (!basicBlockStack.empty()) {
        auto bb = basicBlockStack.top();
        basicBlockStack.pop();
        vector<InstructionPtr> newInstructions;
        for (auto& inst : bb->instructions) {
            if (inst->type == Load) {
                auto loadInst = static_cast<LoadInstruction*>(inst.get());
                auto from = loadInst->from;
                if (stackMap.find(from) != stackMap.end()) {
                    auto& stack = stackMap[from];
                    int idx = stack.size() - 1;
                    while (!isDominatorOf(stack[idx].bb, bb, entry)) {
                        idx--;
                    }
                    replaceVarByVar(loadInst->to, stack[idx].val);
                    rmInsUse(inst.get(), from);
                }
                else {
                    newInstructions.emplace_back(inst);
                }
            }
            else if (inst->type == Store) {
                auto storeInst = static_cast<StoreInstruction*>(inst.get());
                auto des = storeInst->des;
                auto value = storeInst->value;
                if (stackMap.find(des) != stackMap.end()) {
                    stackMap[des].emplace_back(ValueBlockPair(value, bb));
                    if (!rmInsUse(inst.get(), des) || !rmInsUse(inst.get(), value)) {
                        assert(false && "cannot rm Store InstructionUse");
                    }
                }
                else {
                    newInstructions.emplace_back(inst);
                }
            }
            else if (inst->type == Phi) {
                auto phiInst = static_cast<PhiInstruction*>(inst.get());
                auto val = phiInst->val;
                auto reg = phiInst->reg;
                if (stackMap.find(val) != stackMap.end()) {
                    stackMap[val].emplace_back(ValueBlockPair(reg, bb));
                }
                newInstructions.emplace_back(inst);
            }
            else {
                newInstructions.emplace_back(inst);
            }
        }

        for (auto& succ : bb->succBasicBlocks) {
            for (auto& inst : succ->instructions) {
                if (inst->type == Phi) {
                    auto phiInst = static_cast<PhiInstruction*>(inst.get());
                    auto val = phiInst->val;
                    if (stackMap.find(val) != stackMap.end()) {
                        auto& stack = stackMap[val];
                        int idx = stack.size() - 1;
                        while (!isDominatorOf(stack[idx].bb, bb, entry)) {
                            idx--;
                        }
                        phiInst->addFrom(stack[idx].val, bb);
                    }
                }
                else {
                    break;
                }
            }
        }

        bb->instructions = newInstructions;
        for (auto& succ : bb->succBasicBlocks) {
            if (visitedBlocks.find(succ) == visitedBlocks.end()) {
                basicBlockStack.push(succ);
                visitedBlocks[succ] = true;
            }
        }
    }
}