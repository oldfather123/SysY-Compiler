
// 寄存器分配中的部分代码，参考了现代编译原理-c语言描述（虎书）以及2023年参赛队伍@Yat-CC的实现

#include "riscv.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <stdlib.h>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace 
{
    int K = 28; // r0~r12, lr
    const int s_K = 32; // s0~s7 for scalar
    // yyy: 机器寄存器集合，每个寄存器都预先指派了一种颜色
    Set<VOper> precolored;
    // yyy: 临时寄存器集合，其中的元素既没有precolored，也没有被处理
    Set<VOper> initial;
    Set<VOper> simplifyWorklist;
    Set<VOper> freezeWorklist;
    Set<VOper> spillWorklist;
    Set<VOper> spilledNodes;
    // yyy: 已经合并的寄存器集合
    Set<VOper> coalescedNodes;
    // yyy: 已成功着色的结点的结合
    Set<VOper> coloredNodes;
    Set<VOper> selectSet;
    // yyy: 在simplify中存储被删结点的栈
    vector<VOper> selectStack;
    // yyy：比如A call B，那么A自己保存ra的信息
    vector<uint8> needs_save;
    vector<uint8> s_needs_save;
    Set<VOper> alreadySpilled;

    // yyy: 已经合并的Move指令集合
    Set<VI_Move *, VI_Move_Pointer_Cmp> coalescedMoves;
    // yyy: 源操作数和目标操作数冲突的move指令
    Set<VI_Move *, VI_Move_Pointer_Cmp> constrainedMoves;
    Set<VI_Move *, VI_Move_Pointer_Cmp> frozenMoves;
    // yyy: 有可能合并的传送指令集合
    Set<VI_Move *, VI_Move_Pointer_Cmp> worklistMoves;
    // yyy: 还未做好合并准备的传送指令
    Set<VI_Move *, VI_Move_Pointer_Cmp> activeMoves;

    Set<VI_FMove *, VI_VMove_Pointer_Cmp> s_coalescedMoves;
    Set<VI_FMove *, VI_VMove_Pointer_Cmp> s_constrainedMoves;
    Set<VI_FMove *, VI_VMove_Pointer_Cmp> s_frozenMoves;
    Set<VI_FMove *, VI_VMove_Pointer_Cmp> s_worklistMoves;
    Set<VI_FMove *, VI_VMove_Pointer_Cmp> s_activeMoves;

    typedef Pair<VOper, VOper> Edge;
    Set<Edge> adjSet;
    Map<VOper, Set<VOper>> adjList;
    Map<VOper, int> degree;

    // yyy: 记录了move中寄存器和其关联的VI
    Map<VOper, Set<VI_Move *, VI_Move_Pointer_Cmp>> moveList;
    Map<VOper, Set<VI_FMove *, VI_VMove_Pointer_Cmp>> s_moveList;
    // yyy: (u,v)合并时(src应该是u)，v被放入coalescedNodes,则alias[v]=u 、别名
    Map<VOper, VOper> alias;
    Map<VOper, int32> color;
    Map<VOper, int32> s_color;
    // yyy: 表示这个寄存器被use或者被def了几次，见build代码，后来被select_spill启发式中用到
    Map<VOper, uint32> use_def_count;
    Map<VOper, uint32> loop_depth;
    Map<VOper, int32> spill_load_offset;
    Map<VOper, int32> spill_store_offset;

    int32 spilledStackSize = 0;
    bool need_to_reserve_s11 = false;


    static vector<Set<VOper>> live_in;
    static vector<Set<VOper>> live_out;
    static vector<Set<VOper>> use_set;
    static vector<Set<VOper>> def_set;

    // yyy: 分析活跃变量
    void livenessAnalysis(Func_Asm *func_asm, bool stage) {
        // stage=0即通用寄存器分配，stage=1即浮点寄存器分配

        live_in.clear();
        live_out.clear();
        use_set.clear();
        def_set.clear();

        live_in.resize(func_asm->mbs.size());
        live_out.resize(func_asm->mbs.size());
        use_set.resize(func_asm->mbs.size());
        def_set.resize(func_asm->mbs.size());
        for (int i = 0; i < func_asm->mbs.size(); i++){
            auto machineInst = func_asm->mbs[i]->inst;
            while(machineInst){
                // yyy: 参考算法中要求的是 def定值前没被引用   use 引用前没被定值
                vector<VOper> defs = stage ? s_get_defs(machineInst) : get_defs(machineInst) ;
                vector<VOper> uses;
                if(!stage){
                    uses = get_uses(machineInst);
                }
                else if(stage){ 
                    uses = s_get_uses(machineInst);
                }
                for (auto use : uses){
                    // yyy: use 前没被def过
                    if ((use.tag == VREG || use.tag == VFREG) && def_set[i].find(use) == def_set[i].end())
                    {
                        use_set[i].insert(use);
                    }
                }
                for (auto def : defs){
                    def_set[i].insert(def);
                }
                machineInst = machineInst->next;
            }
        }
        // yyy: 计算活跃变量的迭代算法
        bool changed = true;
        // yyy: when some value of live_in changed
        while (changed){
            changed = false;
            for (int i = 0; i < func_asm->mbs.size(); i++){
                Set<VOper> old_in = live_in[i];
                Set<VOper> old_out = live_out[i];
                // yyy: live_in[block B] = use[B] union (live_out[B] - def[B])
                live_in[i] = use_set[i];
                std::set_difference(live_out[i].begin(), live_out[i].end(), def_set[i].begin(), def_set[i].end(), std::inserter(live_in[i], live_in[i].end()));
                // yyy: live_out[B] = union live_in[S] (s is a successor of B)
                live_out[i].clear();
                for (auto succ : func_asm->mbs[i]->succs){
                    std::set_union(live_out[i].begin(), live_out[i].end(), live_in[succ->index].begin(), live_in[succ->index].end(), std::inserter(live_out[i], live_out[i].end()));
                }
                if ((!changed) && ((live_in[i] != old_in) || (live_out[i] != old_out))) changed = true;
            }
        }
    }

#define in(it, set) (((set).find(it)) != ((set).end()))

    bool need_alloc(VOper opr, bool stage){
        if(stage){
            return (opr.tag == FREG || opr.tag == VFREG);
        }  
        return (opr.tag == REG || opr.tag == VREG);
    }

    int fastPow(int base, int exponent) {
        int res = 1;
        while (exponent > 0) {
            if (exponent & 1) res = res * base;
            base = base * base;
            exponent >>= 1;
        }
        return res;
    }

    void build(Func_Asm *func_asm, bool stage);
    void addEdge(VOper u, VOper v);
    void makeWorklist(int regBound);
    Set<VOper> adjacent(VOper n);
    Set<VI_Move *> nodeMoves(VOper n);
    Set<VI_FMove *> s_nodeMoves(VOper n);
    bool moveRelated(VOper n, bool is_general);
    void simplify();
    void enableMoves(Set<VOper> nodes);
    void s_enableMoves(Set<VOper> nodes);
    void decrementDegree(VOper m, int regBound);
    void coalesce();
    void s_coalesce();
    void addWorklist(VOper u, int regBound);
    bool preCheck(VOper t, VOper r, int regBound);
    bool conservative(Set<VOper> nodes, int regBound);
    VOper getAlias(VOper n);
    void combine(VOper u, VOper v, int regBound);
    void freeze(bool stage);
    void freezeMoves(VOper u);
    void s_freezeMoves(VOper u);
    void selectSpill();
    void s_selectSpill();
    void assignColors();
    void s_assignColors();
    void colorRegister(VOper &m);
    void s_colorRegister(VOper &m);
    void allocateRegister(Func_Asm *func_asm);
    void s_allocateRegister(Func_Asm *func_asm);
    void colorRegisterAll(Func_Asm *func_asm);
    void s_colorRegisterAll(Func_Asm *func_asm);
    void recomputeOffset(Func_Asm *func_asm);
    void saveRegisters(Func_Asm *func_asm);
    void s_saveRegisters(Func_Asm *func_asm);
    void allocateStack(Func_Asm *func_asm);
    void clearAll();
    void clearBeforeAllocation();

    // yyy: 构造冲突图，将每个节点分成move-related（是一条move语句的dst or src）和non-move-related
    // yyy: 对于指令MOV A, B，如果A和B能够分配到同一个寄存器，则可以消除这条MOVE指令，从而减少冲突。
    // yyy: 详情参考虎书p180
    void build(Func_Asm *func_asm, bool stage) {
        for (auto b : func_asm->mbs){
            auto live = live_out[b->index];
            // yyy: 从后往前分析，对于每条指令，从活跃变量集中删除该指令的def，并添加该指令的use
            // for (auto I = b->last_inst; I; I = I->prev)
            for (auto I = func_asm->mbs[b->index]->last_inst; I; I = I->prev){
                auto defs = stage? s_get_defs(I) : get_defs(I);
                auto uses = stage? s_get_uses(I) : get_uses(I);
                // yyy: If the node is move-related, we prefer to coalescing instead of simplifing
                if (!stage&&I->tag == VI_MOVE||stage&&I->tag == VI_FMOVE && defs.size() > 0){
                    if (need_alloc(defs[0],stage) && uses.size() > 0 && need_alloc(uses[0],stage)){
                        for (auto use : uses){
                            if (!need_alloc(use, stage)) continue;
                            live.erase(use);
                            if(stage){
                                s_moveList[use].insert((VI_FMove *)I);
                            }else{
                                moveList[use].insert((VI_Move *)I);
                            }
                        }

                        for (auto def : defs){
                            if(stage){
                                s_moveList[def].insert((VI_FMove *)I);
                            }else{
                                moveList[def].insert((VI_Move *)I);
                            }
                        }
                        if(stage){
                            s_worklistMoves.insert((VI_FMove *)I);
                        }else{
                            worklistMoves.insert((VI_Move *)I);
                        }
                    }
                }
                for (auto def : defs){
                    if (need_alloc(def,stage)){   
                        if (b->loop_depth > loop_depth[def]){
                            loop_depth[def] = b->loop_depth;
                        }
                        // yyy : 构建RIG图
                        for (auto l : live){
                            addEdge(l, def);
                        }
                        live.insert(def);
                        use_def_count[def]++;
                    }
                }
                Set<VOper> new_live;
                for (auto u : uses){
                    if (need_alloc(u,stage)){
                        use_def_count[u]++;
                        if (b->loop_depth > loop_depth[u]){
                            loop_depth[u] = b->loop_depth;
                        }
                        new_live.insert(u);
                    }
                }
                for (auto l : live){
                    if (find(defs.begin(), defs.end(), l) == defs.end()){
                        new_live.insert(l);
                    }
                }
                live = new_live;
            }
        }
    }

    void addEdge(VOper u, VOper v) {   
        auto p = Edge{u, v};
        if (!adjSet.count(p) && !(u == v)){
            adjSet.insert({u, v});
            adjSet.insert({v, u});
            if ((u.tag == VREG && (v.tag == REG || v.tag == VREG)) || (u.tag == VFREG && (v.tag == FREG || v.tag == VFREG))){
                adjList[u].insert(v);
                degree[u]++;
            }
            if ((v.tag == VREG && (u.tag == REG || u.tag == VREG)) || (v.tag == VFREG && (u.tag == FREG || u.tag == VFREG))){
                adjList[v].insert(u);
                degree[v]++;
            }
        }
    }

    void makeWorklist(int regBound) {
        while (!initial.empty()){
            auto n = *(initial.begin());
            initial.erase(n);
            if (degree[n] >= regBound){
                spillWorklist.insert(n);
            }
            else if (moveRelated(n, regBound == K ? true : false)){
                freezeWorklist.insert(n);
            }
            else{
                simplifyWorklist.insert(n);
            }
        }
    }

    Set<VOper> adjacent(VOper n) {
        Set<VOper> ret;
        for(auto it:adjList[n]){
            if(!selectSet.count(it) && !coalescedNodes.count(it)) ret.insert(it);
        }
        return ret;
    }

    Set<VI_Move *> nodeMoves(VOper n) {
        Set<VI_Move *> ret;
        for(auto it:moveList[n]){
            if(activeMoves.count(it) || worklistMoves.count(it)) ret.insert(it);
        }
        return ret;
    }

    Set<VI_FMove *> s_nodeMoves(VOper n) {
        Set<VI_FMove *> ret;
        for(auto it:s_moveList[n]){
            if(s_activeMoves.count(it) || s_worklistMoves.count(it)) ret.insert(it);
        }
        return ret;
    }

    bool moveRelated(VOper n, bool is_general) {
        if(is_general) return !nodeMoves(n).empty();
        else return !s_nodeMoves(n).empty();
    }

    // yyy: Kempe's Simplification
    void simplify(int regBound) {
        auto n = *(simplifyWorklist.begin());
        simplifyWorklist.erase(n);
        selectStack.push_back(n);
        selectSet.insert(n);
        for (auto m : adjacent(n)){
            decrementDegree(m, regBound);
        }
    }

    void decrementDegree(VOper m, int regBound) {
        int d = degree[m];
        degree[m]--;
        // yyy: m is in spillWorklist,k = 28, zero,sp,gp,tp这四个不算吧
        if (d == K){
            auto adj = adjacent(m);
            adj.insert(m);
            if(regBound == K) enableMoves(adj);
            else if(regBound == s_K) s_enableMoves(adj);
            spillWorklist.erase(m);
            if (moveRelated(m, d == K ? true : false)) freezeWorklist.insert(m);
            else simplifyWorklist.insert(m);
        }
    }

    void enableMoves(Set<VOper> nodes) {
        for (auto n : nodes){
            for (auto m : nodeMoves(n)){
                if (activeMoves.count(m)){
                    activeMoves.erase(m);
                    worklistMoves.insert(m);
                }
            }
        }
    }

    void s_enableMoves(Set<VOper> nodes) {
        for (auto n : nodes){
            for (auto m : s_nodeMoves(n)){
                if (s_activeMoves.count(m)){
                    s_activeMoves.erase(m);
                    s_worklistMoves.insert(m);
                }
            }
        }
    }

    // yyy: 当合并一条传送指令时，它涉及的那两个结点可能不再是传送有关的，可以将他们加入simpliy_worklist中
    void coalesce() {
        auto m = *(worklistMoves.begin());
        VOper u = getAlias(m->src);
        VOper v = getAlias(m->dst);
        auto edge = Edge{u, v};

        bool precolored = (v.tag == REG);
        if (precolored){
            swap(u, v);
        }
        worklistMoves.erase(m);
        
        int prePass = false;
        for (auto t : adjacent(v)){
            if (precolored || preCheck(t, u, K)){
                prePass = true;
                break;
            }
        }

        // yyy: join = adjacent(u) union adjacent(v) 
        Set<VOper> join;
        auto set_u = adjacent(u);
        auto set_v = adjacent(v);
        std::set_union(set_u.begin(), set_u.end(), set_v.begin(), set_v.end(), std::inserter(join, join.begin()));

        if (u == v){
            coalescedMoves.insert(m);
            addWorklist(u, K);
        }
        else if (precolored || adjSet.count(edge)){
            constrainedMoves.insert(m);
            addWorklist(u, K);
            addWorklist(v, K);
        }
        else if ((precolored && prePass) || (!precolored && conservative(join, K))){
            coalescedMoves.insert(m);
            combine(u, v, K);
            addWorklist(u, K);
        }
        else{
            activeMoves.insert(m);
        }
    }

    void s_coalesce() {
        auto m = *(s_worklistMoves.begin());
        VOper u = getAlias(m->src);
        VOper v = getAlias(m->dst);
        auto edge = Edge{u, v};

        bool precolored = (v.tag == FREG);
        if (precolored){
            swap(u, v);
        }
        s_worklistMoves.erase(m);

        int prePass = false;
        for (auto t : adjacent(v)){
            if (precolored || preCheck(t, u, s_K)){
                prePass = true;
                break;
            }
        }

        Set<VOper> join;
        auto set_u = adjacent(u);
        auto set_v = adjacent(v);
        std::set_union(set_u.begin(), set_u.end(), set_v.begin(), set_v.end(), std::inserter(join, join.begin()));

        if (u == v){
            s_coalescedMoves.insert(m);
            addWorklist(u, s_K);
        }
        else if (precolored || adjSet.count(edge)){
            s_constrainedMoves.insert(m);
            addWorklist(u, s_K);
            addWorklist(v, s_K);
        }
        else if ((precolored && prePass) || (!precolored && conservative(join, s_K))){
            s_coalescedMoves.insert(m);
            combine(u, v, s_K);
            addWorklist(u, s_K);
        }
        else{
            s_activeMoves.insert(m);
        }
    }

    void addWorklist(VOper u, int regBound) {
        if ((regBound == K && u.tag != REG) || (regBound == s_K && u.tag != FREG)){
            if (!moveRelated(u, regBound == K) && degree[u] < regBound){
                freezeWorklist.erase(u);
                simplifyWorklist.insert(u);
            }
        }
    }

    // yyy: 是合并一个预着色寄存器时所使用的启发式函数
    bool preCheck(VOper t, VOper r, int regBound) {
        auto p = Edge{t, r};
        return ((degree[t] < regBound) || adjSet.count(p));
    }

    // yyy: 实现保守合并启发式函数
    bool conservative(Set<VOper> nodes, int regBound) {
        int k = 0;
        for (auto n : nodes){
            if (degree[n] >= regBound){
                k++;
            }
        }
        return k < regBound;
    }

    VOper getAlias(VOper n) {
        if (coalescedNodes.count(n)) return getAlias(alias[n]);
        return n;
    }

    void combine(VOper u, VOper v, int regBound) {
        if (freezeWorklist.count(v)){
            freezeWorklist.erase(v);
        }
        else{
            spillWorklist.erase(v);
        }
        coalescedNodes.insert(v);
        alias[v] = u;

        if(regBound == K){
            std::set_union(moveList[u].begin(), moveList[u].end(), moveList[v].begin(), moveList[v].end(), std::inserter(moveList[u], moveList[u].begin()));
        }
        else if(regBound == s_K){
            std::set_union(s_moveList[u].begin(), s_moveList[u].end(), s_moveList[v].begin(), s_moveList[v].end(), std::inserter(s_moveList[u], s_moveList[u].begin()));
        }
        for (auto t : adjacent(v)){
            addEdge(t, u);
            decrementDegree(t, regBound);
        }
        if (degree[u] >= regBound && freezeWorklist.count(u)){
            freezeWorklist.erase(u);
            spillWorklist.insert(u);
        }
    }

    void freeze(bool stage){
        auto u = *(freezeWorklist.begin());
        freezeWorklist.erase(u);
        simplifyWorklist.insert(u);
        if(stage) freezeMoves(u);
        else s_freezeMoves(u);
    }

    // yyy: 冻结u结点的传送指令
    void freezeMoves(VOper u) {
        for (auto m : nodeMoves(u)){
            auto v = m->dst;
            if (activeMoves.count(m)){
                activeMoves.erase(m);
            }
            else{
                worklistMoves.erase(m);
            }
            frozenMoves.insert(m);
            if (nodeMoves(v).empty() && degree[v] < K)
            {
                freezeWorklist.erase(v);
                simplifyWorklist.insert(v);
            }
        }
    }

    void s_freezeMoves(VOper u) {
        for (auto m : s_nodeMoves(u)){
            auto v = m->dst;
            if (s_activeMoves.count(m)){
                s_activeMoves.erase(m);
            }
            else{
                s_worklistMoves.erase(m);
            }
            s_frozenMoves.insert(m);
            if (s_nodeMoves(v).empty() && degree[v] < s_K)
            {
                freezeWorklist.erase(v);
                simplifyWorklist.insert(v);
            }
        }
    }

    void selectSpill() {
        VOper m = {};
        double minCost = 1e40;
        int bestDepth = 0;
        bool isSpilledNow = false;
        Set<VOper> spilled;
        for (auto it: spillWorklist){
            if (alreadySpilled.count(it)) {
                isSpilledNow = true;
                spilled.insert(it);
                continue;
            }
            assert(degree[it] != 0);

            // yyy: 计算变量的cur_depth
            int curDepth = loop_depth[it];
            double nowCost = use_def_count[it] * 100 / degree[it];
            bool selected = false;
            if(bestDepth == curDepth) {
                // yyy: 选择cost最小的变量
                selected = nowCost < minCost;
            } else if(bestDepth < curDepth) {
                if(pow(4, curDepth - bestDepth) < (minCost / nowCost)) {
                    selected = true;
                }
            } else if(pow(4, bestDepth - curDepth) > (nowCost / minCost)) {
                selected = true;
            }
            if (selected){
                m = it;
                minCost = nowCost;
                bestDepth = curDepth;
            }
        }

        if (m.tag == ERRORTYPE && isSpilledNow) {
            m = *spilled.begin();
        }
        assert(m.tag != ERRORTYPE);
        spillWorklist.erase(m);
        simplifyWorklist.insert(m);
        freezeMoves(m);
    }

    void s_selectSpill() {
        VOper m = {};
        double minCost = 1e40;
        bool isSpilledNow = false;
        Set<VOper> spilled;
        for (auto it: spillWorklist){
            if (alreadySpilled.count(it)) {
                isSpilledNow = true;
                spilled.insert(it);
                continue;
            }
            assert(degree[it] != 0);
            double cost = use_def_count[it] * pow(10, loop_depth[it]) / degree[it];
            if (cost < minCost){
                m = it;
                minCost = cost;
            }
        }

        if (m.tag == ERRORTYPE && isSpilledNow) {
            m = *spilled.begin();
        }
        assert(m.tag != ERRORTYPE);
        spillWorklist.erase(m);
        simplifyWorklist.insert(m);
        s_freezeMoves(m);
    }

    void assignColors() {
        while (!selectStack.empty()){
            auto n = selectStack.back();
            selectStack.pop_back();
            vector<uint8> okColors;

            for (uint8 r = a0; r <= a7; r++){
                okColors.push_back(r);
            }
            okColors.push_back(t0);
            okColors.push_back(t1);
            okColors.push_back(t2);
            for (uint8 r = t3; r <= t6; r++){
                okColors.push_back(r);
            }
            okColors.push_back(s1);
            for (uint8 r = s2; r <= s9; r++){
                okColors.push_back(r);
            }
            okColors.push_back(s10);
            if(!need_to_reserve_s11) okColors.push_back(s11);
            okColors.push_back(fp);
            okColors.push_back(ra);

            for (auto w : adjList[n]){
                auto alias = getAlias(w);
                if (coloredNodes.count(alias) || precolored.count(alias)){
                    auto it = find(okColors.begin(),okColors.end(),color[alias]);
                    if(it != okColors.end()) okColors.erase(it);
                }
            }
            if (okColors.empty()){
                spilledNodes.insert(n);
            }
            else{
                coloredNodes.insert(n);
                auto c = okColors.front();
                color[n] = c;
                if (is_callee_save(c)){
                    if(find(needs_save.begin(),needs_save.end(),c)==needs_save.end()){
                        needs_save.push_back(c);
                    }
                }
            }
        }
        for (auto n : coalescedNodes){
            color[n] = color[getAlias(n)];
        }
    }

    void s_assignColors() {
        while (!selectStack.empty()){
            auto n = selectStack.back();
            selectStack.pop_back();
            vector<uint8> okColors;
            for (uint8 r = f10; r <= f17; r++){
                okColors.push_back(r);
            }
            for (uint8 r = f0; r <= f7; r++){
                okColors.push_back(r);
            }
            for (uint8 r = f28; r <= f31; r++){
                okColors.push_back(r);
            }
            for (uint8 r = f8; r <= f9; r++){
                okColors.push_back(r);
            }
            for (uint8 r = f18; r <= f27; r++){
                okColors.push_back(r);
            }
            for (auto w : adjList[n]){
                auto alias = getAlias(w);
                if (coloredNodes.count(alias) || precolored.count(alias)){
                    auto it = find(okColors.begin(),okColors.end(),s_color[alias]);
                    if(it != okColors.end()) okColors.erase(it);
                }
            }
            if (okColors.empty()){
                spilledNodes.insert(n);
            }
            else{
                coloredNodes.insert(n);
                auto c = okColors.front();
                s_color[n] = c;
                if (s_is_callee_save(c)){
                    if(find(s_needs_save.begin(),s_needs_save.end(),c)==s_needs_save.end()){
                        s_needs_save.push_back(c);
                    } 
                }
            }
        }
        for (auto n : coalescedNodes){
            s_color[n] = s_color[getAlias(n)];
        }
    }

    void colorRegister(VOper &m) {
        if (m.tag == VREG){
            uint8 reg = color[m];
            m.value = reg;
            m.tag = REG;
        }
    }

    void s_colorRegister(VOper &m) {
        if (m.tag == VFREG){
            uint8 reg = s_color[m];
            m.value = reg;
            m.tag = FREG;
        }
    }

    void allocateRegister(Func_Asm *func_asm)
    {
        clearAll();
        livenessAnalysis(func_asm, false);
        if(func_asm->vreg_num >= 60000) {
            need_to_reserve_s11 = true;
        }
        for (int i = 0; i < REG_COUNT; i++) {
            auto r = create_reg(i);
            precolored.insert(r);
            color[r] = i;
        }
        // yyy: vreg_num 的initial
        for (int i = 0; i < func_asm->vreg_num; i++) {
            initial.insert(create_vreg(i));
        }
        build(func_asm, false);
        makeWorklist(K);
        bool stopFlag = true;
        while(stopFlag) {
            stopFlag = false;
            if (!simplifyWorklist.empty()) simplify(K); // yyy: 每次一个地从图中删除度数<k的与传送无关的节点,只剩下预分配节点后停止 
            else if (!worklistMoves.empty()) coalesce();
            else if (!freezeWorklist.empty()) freeze(true); // yyy: 如果简化和合并都不能进行，找一个度数较低且move相关的节点，冻结关联的所有传送指令，这样会让其他节点被看成传送无关的，有更多的简化
            else if (!spillWorklist.empty()) selectSpill();

            if(!simplifyWorklist.empty() || !worklistMoves.empty() || !freezeWorklist.empty() || !spillWorklist.empty()){
                stopFlag = true;
            }
        }
        assignColors();
        if (!spilledNodes.empty()) {
            // yyy: RewriteProgram函数，为溢出的临时变量分配存储单元，并插入访问这些单元的存取指令，同时重新调用分配算法（当然这个图更小）
            for (VOper n : spilledNodes) {
                alreadySpilled.insert(n);
                spilledStackSize += 8;
                // yyy: 为每个n的def后插入一条存储指令，use之前插入一条取数指令
                for (auto bb : func_asm->mbs) {
                    bool marked = false;
                    for (auto I = bb->inst; I; I = I->next) {
                        auto defs = get_defs(I);
                        auto uses = get_uses(I);
                        bool insertedStore = false;
                        bool insertedUse = false;
                        if (find(defs.begin(), defs.end(), n) != defs.end()) {
                            auto str = new VI_Store;
                            str->mem_tag = MEM_SAVE_SPILL;
                            str->reg = n;
                            str->base = create_reg(sp);
                            str->offset = create_imm(-(spilledStackSize));
                            str->is_dw = true;
                            insert((VI *)str, I->next);
                            I = I->next;
                            insertedStore = true;
                            spill_store_offset[n] = -(spilledStackSize);
                        }
                        if (find(uses.begin(),uses.end(),n)!=uses.end()) {
                            if(I->tag == VI_STORE && spill_load_offset.count(n)) {
                                auto store = (VI_Store *)I;
                                if(store->mem_tag == MEM_SAVE_SPILL) {
                                    I->mark();
                                    marked = true;
                                    continue;
                                }
                            }
                            auto ldr = new VI_Load;
                            ldr->mem_tag = MEM_LOAD_SPILL;
                            ldr->reg = create_vreg(func_asm->vreg_num++);
                            ldr->base = create_reg(sp);
                            ldr->offset = create_imm(-(spilledStackSize));
                            ldr->is_dw = true;
                            alreadySpilled.insert(ldr->reg);
                            replace_uses(I, &n, &ldr->reg, func_asm);
                            insert((VI *)ldr, I);
                            insertedUse = true;
                            spill_load_offset[ldr->reg] = -(spilledStackSize);
                        }
                        assert(!(insertedUse && insertedStore));
                    }
                    if(marked) bb->erase_marked_values();
                }
            }
            func_asm->stack_size += spilledStackSize;
            func_asm->reg_spill_size += spilledStackSize;
        }
    }

    void s_allocateRegister(Func_Asm *func_asm)
    {
        clearAll();
        livenessAnalysis(func_asm, true);
        for (int i = 0; i < FREG_COUNT; i++) {
            auto r = create_freg(i);
            precolored.insert(r);
            s_color[r] = i;
        }
        for (int i = 0; i < func_asm->vfreg_num; i++) {
            initial.insert(create_vfreg(i));
        }
        build(func_asm, true);
        makeWorklist(s_K);
        bool stopFlag = true;
        while(stopFlag){
            stopFlag = false;
            if (!simplifyWorklist.empty()) simplify(s_K);
            else if (!s_worklistMoves.empty()) s_coalesce();
            else if (!freezeWorklist.empty()) freeze(false);
            else if (!spillWorklist.empty()) s_selectSpill();

            if(!simplifyWorklist.empty() || !s_worklistMoves.empty() || !freezeWorklist.empty() || !spillWorklist.empty()){
                stopFlag = true;
            }
        }
        s_assignColors();
        if (!spilledNodes.empty()) {
            for (VOper n : spilledNodes) {
                alreadySpilled.insert(n);
                spilledStackSize += 8;
                for (auto bb : func_asm->mbs) {
                    bool marked = false;
                    for (auto I = bb->inst; I; I = I->next) {
                        auto defs = s_get_defs(I);
                        auto uses = s_get_uses(I);
                        bool insertedStore = false;
                        bool insertedUse = false;
                        if (find(defs.begin(),defs.end(),n)!=defs.end()) {
                            auto str = new VI_VStore;
                            str->mem_tag = MEM_SAVE_SPILL;
                            str->reg = n;
                            str->base = create_reg(sp);
                            str->offset = create_imm(-(spilledStackSize));
                            str->is_dw = true;
                            alreadySpilled.insert(str->reg);
                            insert((VI *)str, I->next);
                            I = I->next;
                            insertedStore = true;
                            spill_store_offset[n] = -(spilledStackSize);
                        }
                        if (find(uses.begin(),uses.end(),n)!=uses.end()) {
                            if(I->tag == VI_VSTORE && spill_load_offset.count(n)) {
                                auto store = (VI_VStore *)I;
                                if(store->mem_tag == MEM_SAVE_SPILL) {
                                    I->mark();
                                    marked = true;
                                    continue;
                                }
                            }
                            auto ldr = new VI_VLoad;
                            ldr->mem_tag = MEM_LOAD_SPILL;
                            ldr->reg = create_vfreg(func_asm->vfreg_num++);
                            ldr->base = create_reg(sp);
                            ldr->offset = create_imm(-(spilledStackSize));
                            ldr->is_dw = true;
                            alreadySpilled.insert(ldr->reg);
                            s_replace_uses(I, &n, &ldr->reg, func_asm);
                            insert((VI *)ldr, I);
                            insertedUse = true;
                            spill_load_offset[ldr->reg] = -(spilledStackSize);
                        }
                        assert(!(insertedUse && insertedStore));
                    }
                    if(marked) bb->erase_marked_values();
                }
            }
            func_asm->stack_size += spilledStackSize;
            func_asm->sreg_spill_size += spilledStackSize;
        }
    }

    void colorRegisterAll(Func_Asm *func_asm) {
        for (auto mb : func_asm->mbs){
            auto machineInst = mb->inst;
            while(machineInst){
                switch (machineInst->tag){
                    case VI_MOVE:{
                        colorRegister(((VI_Move *)machineInst)->dst);
                        colorRegister(((VI_Move *)machineInst)->src);
                        break;
                    }
                    case VI_BINARY:{
                        colorRegister(((VI_Binary *)machineInst)->dst);
                        colorRegister(((VI_Binary *)machineInst)->lhs);
                        colorRegister(((VI_Binary *)machineInst)->rhs);
                        break;
                    }
                    case VI_COMPARE:{
                        colorRegister(((VI_Compare *)machineInst)->lhs);
                        colorRegister(((VI_Compare *)machineInst)->rhs);
                        break;
                    }
                    case VI_SLT:{
                        colorRegister(((VI_Slt *)machineInst)->dst);
                        colorRegister(((VI_Slt *)machineInst)->lhs);
                        colorRegister(((VI_Slt *)machineInst)->rhs);
                        break;
                    }
                    case VI_SEQZ:{
                        colorRegister(((VI_Seqz *)machineInst)->dst);
                        colorRegister(((VI_Seqz *)machineInst)->src);
                        break;
                    } 
                    case VI_SNEZ:{
                        colorRegister(((VI_Snez *)machineInst)->dst);
                        colorRegister(((VI_Snez *)machineInst)->src);
                        break;
                    } 
                    case VI_LOAD:
                    case VI_STORE:
                    {
                        colorRegister(((VI_Load *)machineInst)->reg);
                        colorRegister(((VI_Load *)machineInst)->base);
                        colorRegister(((VI_Load *)machineInst)->offset);
                        break;
                    }
                    case VI_BRANCH:{
                        if (((VI_Branch *)machineInst)->cond != NO_CONDITION){
                            colorRegister(((VI_Branch *)machineInst)->lhs);
                            colorRegister(((VI_Branch *)machineInst)->rhs);
                        }
                        break;
                    }
                    case VI_RETURN:{
                        auto return_addr = create_reg(ra);
                        colorRegister(return_addr);
                        break;
                    } 

                    case VI_VLOAD:
                    case VI_VSTORE:
                    {
                        auto vmv = ((VI_VLoad *)machineInst);
                        if (vmv->base.tag == REG || vmv->base.tag == VREG){
                            colorRegister(vmv->base);
                        }
                        if (vmv->offset.tag == REG || vmv->offset.tag == VREG){
                            colorRegister(vmv->offset);
                        }
                        break;
                    }
                    case VI_FCMP_SET:{
                        colorRegister(((VI_Fcmp_Set *)machineInst)->dst);
                        break;
                    }
                    case VI_VCVT:{
                        auto fcvt = (VI_VCvt *)machineInst;
                        if (fcvt->from_type == S32){
                            colorRegister(fcvt->src);
                        }
                        if (fcvt->to_type == S32){
                            colorRegister(fcvt->dst);
                        }
                        break;
                    }
                    case VI_FMOVE:{
                        if (((VI_FMove *)machineInst)->from_s32){
                            colorRegister(((VI_FMove *)machineInst)->src);
                        }
                        break;
                    }
                    case VI_VBINARY:
                    case VI_VPUSH:
                    case VI_VPOP:
                    case VI_FUNC_CALL:
                    case VI_PUSH:
                    case VI_POP:
                        break;

                    default:
                        assert(false && "colorRegisterAll stage: unknown VI fault.\n");
                }
                machineInst = machineInst->next;
            }
        }
    }

    void s_colorRegisterAll(Func_Asm *func_asm) {
        for (auto mb : func_asm->mbs){
            auto machineInst = mb->inst;
            while(machineInst){
                switch (machineInst->tag){
                case VI_FMOVE:{
                    s_colorRegister(((VI_FMove *)machineInst)->dst);
                    if (!((VI_FMove *)machineInst)->from_s32){
                        s_colorRegister(((VI_FMove *)machineInst)->src);
                    }
                    break;
                }
                case VI_VBINARY:{
                    s_colorRegister(((VI_VBinary *)machineInst)->dst);
                    s_colorRegister(((VI_VBinary *)machineInst)->lhs);
                    s_colorRegister(((VI_VBinary *)machineInst)->rhs);
                    break;
                }
                case VI_FCMP_SET:{
                    s_colorRegister(((VI_Fcmp_Set *)machineInst)->lhs);
                    s_colorRegister(((VI_Fcmp_Set *)machineInst)->rhs);
                    break;
                }
                case VI_VLOAD:
                case VI_VSTORE:
                {
                    s_colorRegister(((VI_VLoad *)machineInst)->reg);
                    break;
                }
                case VI_VCVT:{
                    auto fcvt = (VI_VCvt *)machineInst;
                    if (fcvt->from_type == F32){
                        s_colorRegister(fcvt->src);
                    }
                    if (fcvt->to_type == F32){
                        s_colorRegister(fcvt->dst);
                    }
                    break;
                }
                case VI_VPUSH:
                case VI_VPOP:

                case VI_RETURN:
                case VI_FUNC_CALL:
                case VI_PUSH: // we don't push/pop virtual regs, for now
                case VI_POP:
                case VI_BRANCH:
                case VI_MOVE:
                case VI_BINARY:
                case VI_COMPARE:
                case VI_SLT:
                case VI_SEQZ:
                case VI_SNEZ:
                case VI_LOAD:
                case VI_STORE:
                    break;

                default:
                    assert(false && "s_colorRegisterAll stage: unknown VI fault.\n");
                }
                machineInst = machineInst->next;
            }
        }
    }

    void recomputeOffset(Func_Asm *func_asm) {
        int stack_arg_size = 0;
        for (auto mb : func_asm->mbs) {
            for (auto I = mb->inst; I; I = I->next) {
                if (I->tag == VI_FUNC_CALL) {
                    auto call = (VI_Func_Call *)I;
                    if (call->arg_stack_size > stack_arg_size) {
                        stack_arg_size = call->arg_stack_size;
                    }
                }
            }
        }
        // yyy: 存储最大的arg_stack_size
        if(stack_arg_size > 0) func_asm->stack_size += stack_arg_size;

        // yyy: .attribute stack_align, 16，16字节
        int save_stack_size = (func_asm->reg_needs_save + func_asm->sreg_needs_save) * 8;
        if ((func_asm->stack_size + save_stack_size) % 16 != 0) {
            func_asm->stack_size += 16 - ((func_asm->stack_size + save_stack_size) % 16);
        }
        // alloc
        for (auto base : func_asm->local_array_bases) {
            if(base->mb == NULL) continue;
            auto sub = (VI_Binary *)base;
            int offset_relative_to_sp = func_asm->stack_size - sub->rhs.value;
            auto offset = create_imm(offset_relative_to_sp);
            if(!can_be_imm12(offset_relative_to_sp)) {
                generate_li(sub->dst, offset_relative_to_sp, sub->mb, sub);
                offset = sub->dst;
            }
            auto array_offset = new VI_Binary(BINARY_ADD, sub->dst, create_reg(sp), offset);
            array_offset->is_dw = true;
            insert(array_offset, sub);
            sub->erase_from_parent();
        }
        int all_spill_stack_size = func_asm->reg_spill_size + func_asm->sreg_spill_size;
        for (auto bb : func_asm->mbs){
            for (auto I = bb->inst; I; I = I->next){
                switch (I->tag)
                {
                    case VI_VLOAD:
                    case VI_LOAD:{
                        auto ldr = (VI_Load *)I;
                        if (ldr->mem_tag == MEM_LOAD_SPILL){
                            int32 offset_value = ((ldr->offset.tag == ERRORTYPE) ? 0 : ldr->offset.value);
                            int32 offset_relative_to_sp = all_spill_stack_size + stack_arg_size + offset_value;
                            offset_relative_to_sp -= (I->tag==VI_VLOAD)? func_asm->reg_spill_size : 0;
                            ldr->base.value = sp;
                            ldr->offset = create_imm(offset_relative_to_sp);
                        }
                        else if (ldr->mem_tag == MEM_LOAD_ARG){
                            int32 offset_value = ((ldr->offset.tag == ERRORTYPE) ? 0 : ldr->offset.value);
                            int32 offset_relative_to_sp = func_asm->stack_size + offset_value + save_stack_size;
                            ldr->base.value = sp;
                            ldr->offset = create_imm(offset_relative_to_sp);
                        }
                        if(ldr->offset.tag == IMM && !can_be_imm12(ldr->offset.value)) {
                            generate_li(ldr->reg, ldr->offset.value, ldr->mb, ldr);
                            auto cal_new_base = new VI_Binary(BINARY_ADD, ldr->reg, ldr->base, ldr->reg);
                            cal_new_base->is_dw = true;
                            insert(cal_new_base, ldr);
                            ldr->offset = create_imm(0);
                            ldr->base = ldr->reg;
                        }
                    } break;

                    case VI_VSTORE:
                    case VI_STORE:{
                        auto str = (VI_Store *)I;
                        if (str->mem_tag == MEM_SAVE_SPILL){
                            int32 offset_value = ((str->offset.tag == ERRORTYPE) ? 0 : str->offset.value);
                            int32 offset_relative_to_sp = all_spill_stack_size + stack_arg_size + offset_value;
                            offset_relative_to_sp -= (I->tag==VI_VSTORE)? func_asm->reg_spill_size :0;
                            str->base.value = sp;
                            str->offset = create_imm(offset_relative_to_sp);
                        }
                        if(str->offset.tag == IMM && !can_be_imm12(str->offset.value)) {
                            assert(need_to_reserve_s11);
                            auto new_offset = create_reg(s11);
                            generate_li(new_offset, str->offset.value, str->mb, str);
                            auto cal_new_base = new VI_Binary(BINARY_ADD, new_offset, str->base, new_offset);
                            cal_new_base->is_dw = true;
                            insert(cal_new_base, str);
                            str->base = cal_new_base->dst;
                            str->offset = create_imm(0);
                        }
                    } break;

                    default:
                        break;
                }
            }
        }
    }

    // yyy: 这里的工作是为函数调用指令添加一些保护现场的语句
    void saveRegisters(Func_Asm *func_asm) {
        // yyy: 在riscv中，ra寄存器是调用者保存的
        if(find(needs_save.begin(),needs_save.end(),ra)==needs_save.end()) needs_save.push_back(ra);
        func_asm->reg_needs_save = needs_save.size();
        for (auto r : needs_save) {
            print_operand(create_reg(r));
            //printf(" ");
            cerr << " ";
        }
        if (!needs_save.empty()) {
            std::sort(needs_save.begin(), needs_save.end());
            auto store = new VI_Push;
            for (auto r : needs_save) {
                store->operands.push_back(create_reg(r));
            }
            insert((VI *)store, func_asm->mbs[0]->inst);
            for (auto bb : func_asm->mbs) {
                if (bb->last_inst && bb->last_inst->tag == VI_RETURN) {
                    auto restore = new VI_Pop;
                    for (auto r : needs_save) {
                        restore->operands.push_back(create_reg(r));
                    }
                    insert((VI *)restore, bb->last_inst);
                }
            }
        }
    }

    void s_saveRegisters(Func_Asm *func_asm) {
        func_asm->sreg_needs_save = s_needs_save.size();
        for (auto r : s_needs_save) {
            print_operand(create_freg(r));
        }
        if (!s_needs_save.empty()) {
            std::sort(s_needs_save.begin(), s_needs_save.end());
            vector<VI_VPush *> store_list;
            vector<VI_VPop *> restore_list;
            auto store = new VI_VPush;
            auto restore = new VI_VPop;
            int i = 0;
            for(auto r : s_needs_save) {
                store->operands.push_back(create_freg(r));
                restore->operands.push_back(create_freg(r));
            }
            store_list.push_back(store);
            restore_list.push_back(restore);
            for(int j = 0; j < store_list.size(); j ++) {
                insert((VI *)store_list[j], func_asm->mbs[0]->inst);
            }
            for (auto bb : func_asm->mbs) {
                if (bb->last_inst && bb->last_inst->tag == VI_RETURN) {
                    for(int j = 0; j < restore_list.size(); j ++) {
                        insert((VI *)restore_list[j], bb->last_inst);
                    }
                }
            }
        }
    }

    // yyy: 在函数开始压栈保存，函数结束后恢复，此处位置对应save_registers函数
    void allocateStack(Func_Asm *func_asm) {
        if (func_asm->stack_size != 0){
            int cur_stack_size = func_asm->stack_size;
            VI_Binary *alloc_stack, *destroy_stack;
            auto offset_opr = create_imm(cur_stack_size);
            bool is_overflow = false;
            if(!can_be_imm12(cur_stack_size)) {
                is_overflow = true;
                offset_opr = create_reg(t0);
            }
            alloc_stack = new VI_Binary(BINARY_SUBTRACT, create_reg(sp), create_reg(sp), offset_opr);
            destroy_stack = new VI_Binary(BINARY_ADD, create_reg(sp), create_reg(sp), offset_opr);
            alloc_stack->is_dw = true;
            destroy_stack->is_dw = true;
            auto start_inst = func_asm->mbs[0]->inst;
            while (start_inst != NULL && (start_inst->tag == VI_PUSH || start_inst->tag == VI_VPUSH)) {
                start_inst = start_inst->next;
            }
            assert(start_inst != NULL);
            if(is_overflow)
                generate_li(create_reg(t0), cur_stack_size, start_inst->mb, start_inst);
            insert(alloc_stack, start_inst);
            for (auto bb : func_asm->mbs) {
                if (bb->last_inst && bb->last_inst->tag == VI_RETURN) {
                    auto last_inst = bb->last_inst;
                    while (last_inst != NULL && last_inst->prev != NULL && (last_inst->prev->tag == VI_POP || last_inst->prev->tag == VI_VPOP)) {
                        last_inst = last_inst->prev;
                    }
                    if(is_overflow)
                        generate_li(create_reg(t0), cur_stack_size, last_inst->mb, last_inst);
                    insert((VI *)destroy_stack, last_inst);
                }
            }
        }
    }

    void clearAll() {
        precolored.clear();
        initial.clear();
        simplifyWorklist.clear();
        freezeWorklist.clear();
        spillWorklist.clear();
        spilledNodes.clear();
        coalescedNodes.clear();
        coloredNodes.clear();
        coalescedMoves.clear();
        s_coalescedMoves.clear();
        constrainedMoves.clear();
        s_constrainedMoves.clear();
        frozenMoves.clear();
        s_frozenMoves.clear();
        selectSet.clear();
        worklistMoves.clear();
        s_worklistMoves.clear();
        activeMoves.clear();
        s_activeMoves.clear();
        adjSet.clear();
        adjList.clear();
        degree.clear();
        moveList.clear();
        s_moveList.clear();
        alias.clear();
        use_def_count.clear();
        loop_depth.clear();
        selectStack.clear();
    }

    void clearBeforeAllocation() {
        color.clear();
        s_color.clear();
        needs_save.clear();
        s_needs_save.clear();
    }

    void alloc(Func_Asm *func_asm)
    {
        spilledStackSize = 0;
        alreadySpilled.clear();
        spill_load_offset.clear();
        spill_store_offset.clear();
        do {
            allocateRegister(func_asm);
        } while(!spilledNodes.empty());
    }

    void s_alloc(Func_Asm *func_asm)
    {
        spilledStackSize = 0;
        alreadySpilled.clear();
        spill_load_offset.clear();
        spill_store_offset.clear();
        do {
            s_allocateRegister(func_asm);
        } while(!spilledNodes.empty());
    }
}

void register_allocation(Program_Asm *program_asm) {
    for (auto f : program_asm->functions){
        clearBeforeAllocation();
        alloc(f);
        s_alloc(f);
        // yyy: 在start block 和 含有return 的block中保存现场
        if(f->call_function_num){
            saveRegisters(f);
            s_saveRegisters(f);
        }
        recomputeOffset(f);
        allocateStack(f);
        colorRegisterAll(f);
        s_colorRegisterAll(f);
    }
}
