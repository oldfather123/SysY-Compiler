// Referrence from Compiler-2023-YatCC: https://gitlab.eduxiji.net/educg-group-17291-1894922/202310558201558-3109/-/blob/main/src/backEnd/asm/register_allocation.h
// NOLINTBEGIN
#include "riscv.h"

namespace RA {

    // liveIn and liveOut for each machine blocks
    static Array<std::set<MOperand>> liveIn;   // new operands in a specific liveness period
    static Array<std::set<MOperand>> liveOut;  // operands used by successor liveness periods

    static Array<std::set<MOperand>> useSet;  // actually 'use without def' set
    static Array<std::set<MOperand>> defSet;

    void analyseLiveness(MFunc* func) {
        liveIn.clear();
        liveOut.clear();
        useSet.clear();
        defSet.clear();
        liveIn.setLen(func->mBlocks.len);
        liveOut.setLen(func->mBlocks.len);
        useSet.setLen(func->mBlocks.len);
        defSet.setLen(func->mBlocks.len);
        // 初始化定义和使用
        for(int mbi = 0; mbi < func->mBlocks.len; mbi++) {
            for(auto inst = func->mBlocks[mbi]->firInst; inst; inst = inst->next) {
                auto users = IGetDefs(inst);
                auto useds = IGetUses(inst, func->hasReturnValue);
                for(auto used : useds) {
                    if(used.tag == VIREG && defSet[mbi].find(used) == defSet[mbi].end()) {
                        useSet[mbi].insert(used);
                    }
                }
                for(auto user : users) {
                    defSet[mbi].insert(user);
                }
            }
        }
        // 使用不动点算法更新 liveIn 和 liveOut
        bool changed = true;
        while(changed) {
            changed = false;
            for(int i = 0; i < func->mBlocks.len; i++) {
                auto oldIn = liveIn[i];
                auto oldOut = liveOut[i];
                liveIn[i] = useSet[i];
                for(auto out : liveOut[i]) {
                    if(defSet[i].find(out) == defSet[i].end()) {
                        liveIn[i].insert(out);
                    }
                }
                liveOut[i].clear();
                for(auto succ : func->mBlocks[i]->succs) {
                    for(auto succ_in : liveIn[succ->id]) {
                        liveOut[i].insert(succ_in);
                    }
                }

                if(!changed) {
                    if((liveIn[i] != oldIn) || (liveOut[i] != oldOut)) {
                        changed = true;
                    }
                }
            }
        }
    }

    void s_analyse_liveness(MFunc* func_asm) {

        liveIn.clear();
        liveOut.clear();
        useSet.clear();
        defSet.clear();

        liveIn.setLen(func_asm->mBlocks.len);
        liveOut.setLen(func_asm->mBlocks.len);
        useSet.setLen(func_asm->mBlocks.len);
        defSet.setLen(func_asm->mBlocks.len);

        printf(">>> Begin s_analyse_liveness...\n");
        for(int i = 0; i < func_asm->mBlocks.len; i++) {
            for(auto I = func_asm->mBlocks[i]->firInst; I; I = I->next) {
                Array<MOperand> defs = FGetDefs(I);
                Array<MOperand> uses = FGetUses(I, func_asm->hasReturnValue);

                for(auto use : uses) {
                    if(use.tag == VFREG && defSet[i].find(use) == defSet[i].end()) {
                        useSet[i].insert(use);
                    }
                }

                for(auto def : defs) {
                    defSet[i].insert(def);
                }
            }
        }

        bool changed = true;
        while(changed) {
            changed = false;
            for(int i = 0; i < func_asm->mBlocks.len; i++) {
                std::set<MOperand> old_in = liveIn[i];
                std::set<MOperand> old_out = liveOut[i];

                // live_in[i] = use_set[i] union (live_out[i]-def_set[i])
                liveIn[i] = useSet[i];
                for(auto o : liveOut[i]) {
                    if(defSet[i].find(o) == defSet[i].end()) {
                        liveIn[i].insert(o);
                    }
                }

                liveOut[i].clear();
                for(auto succ : func_asm->mBlocks[i]->succs) {
                    for(auto succ_in : liveIn[succ->id]) {
                        liveOut[i].insert(succ_in);
                    }
                }

                if(!changed) {
                    if((liveIn[i] != old_in) || (liveOut[i] != old_out)) {
                        changed = true;
                    }
                }
            }
        }
        printf(">>> End s_analyse_liveness...\n");
    }
    int K = 28;          // r0~r12, lr
    const int s_K = 32;  // s0~s7 for scalar
    // const int max_push_num = 16;
    std::set<MOperand> precolored;
    std::set<MOperand> initial;
    std::set<MOperand> simplify_worklist;
    std::set<MOperand> freeze_worklist;
    std::set<MOperand> spill_worklist;
    std::set<MOperand> spilled_nodes;
    std::set<MOperand> coalesced_nodes;
    std::set<MOperand> colored_nodes;
    std::set<MOperand> select_set;
    Array<MOperand> select_stack;
    Array<uint8> needs_save;
    Array<uint8> s_needs_save;

    // keep track of already spilled nodes
    // avoid spilling repeatly
    std::set<MOperand> already_spilled;

    std::set<MI_Move*, MI_Move_Pointer_Cmp> coalesced_moves;
    std::set<MI_Move*, MI_Move_Pointer_Cmp> constrained_moves;
    std::set<MI_Move*, MI_Move_Pointer_Cmp> frozen_moves;
    std::set<MI_Move*, MI_Move_Pointer_Cmp> worklist_moves;
    std::set<MI_Move*, MI_Move_Pointer_Cmp> active_moves;

    std::set<MI_FMove*, MI_FMove_Pointer_Cmp> s_coalesced_moves;
    std::set<MI_FMove*, MI_FMove_Pointer_Cmp> s_constrained_moves;
    std::set<MI_FMove*, MI_FMove_Pointer_Cmp> s_frozen_moves;
    std::set<MI_FMove*, MI_FMove_Pointer_Cmp> s_worklist_moves;
    std::set<MI_FMove*, MI_FMove_Pointer_Cmp> s_active_moves;

    typedef std::pair<MOperand, MOperand> Edge;
    std::set<Edge> adj_set;
    // std::set< std::set<MOperand> > adj_list;
    std::map<MOperand, std::set<MOperand>> adj_list;
    std::map<MOperand, int> degree;  // @default 0?

    std::map<MOperand, std::set<MI_Move*, MI_Move_Pointer_Cmp>> move_list;
    std::map<MOperand, std::set<MI_FMove*, MI_FMove_Pointer_Cmp>> s_move_list;
    std::map<MOperand, MOperand> alias;
    std::map<MOperand, int32> color;
    std::map<MOperand, int32> s_color;
    std::map<MOperand, uint32> use_def_count;
    std::map<MOperand, uint32> loop_depth;
    std::map<MOperand, int32> spill_load_offset;
    std::map<MOperand, int32> spill_store_offset;

    int32 spilled_stack_size = 0;
    bool done_post_work = false;
    bool need_to_legalize_imm = false;
    bool need_to_reserve_s11 = false;

#define in(it, set) (((set).find(it)) != ((set).end()))

    bool need_alloc(MOperand opr) {
        return (opr.tag == IREG || opr.tag == VIREG);
    }

    bool s_need_alloc(MOperand opr) {
        return (opr.tag == FREG || opr.tag == VFREG);
    }

    void add_edge(MOperand u, MOperand v);
    void build(MFunc* func_asm);
    void s_build(MFunc* func_asm);
    std::set<MOperand> adjacent(MOperand n);
    // std::set<MI_Move *> node_moves(MOperand n);
    bool move_related(MOperand n);
    bool s_move_related(MOperand n);
    void make_worklist();
    void s_make_worklist();
    void enable_moves(std::set<MOperand> nodes);
    void s_enable_moves(std::set<MOperand> nodes);
    void decrement_degree(MOperand m);
    void s_decrement_degree(MOperand m);
    void simplify();
    void coalesce();
    void add_worklist(MOperand u);
    void s_simplify();
    void s_coalesce();
    void s_add_worklist(MOperand u);
    bool ok(MOperand t, MOperand r);
    bool s_ok(MOperand t, MOperand r);
    bool conservative(std::set<MOperand> nodes);
    bool s_conservative(std::set<MOperand> nodes);
    MOperand get_alias(MOperand n);
    void combine(MOperand u, MOperand v);
    void freeze();
    void freeze_moves(MOperand u);
    void s_combine(MOperand u, MOperand v);
    void s_freeze();
    void s_freeze_moves(MOperand u);
    void assign_colors();
    void s_assign_colors();
    void select_spill();
    void s_select_spill();
    void color_register_all(MFunc* func_asm);
    void color_sregister_all(MFunc* func_asm);
    void legalize_imm(MFunc* func_asm);
    void recompute_offset(MFunc* func_asm);
    void save_registers(MFunc* func_asm);
    void save_sregisters(MFunc* func_asm);
    int nearest_imm12(int x);
    void allocate_stack(MFunc* func_asm);
    void clear_all();
    void clear_before_allocation();

    void add_edge(MOperand u, MOperand v) {
        auto p = Edge{ u, v };
        if(!in(p, adj_set) && !(u == v)) {
            // printf("interference: {"); print_operand(u); printf(", "); print_operand(v); printf("}\n");
            adj_set.insert({ u, v });
            adj_set.insert({ v, u });

            if(u.tag == VIREG && (v.tag == IREG || v.tag == VIREG)) {
                adj_list[u].insert(v);
                degree[u]++;
            }

            if(v.tag == VIREG && (u.tag == IREG || u.tag == VIREG)) {
                adj_list[v].insert(u);
                degree[v]++;
            }
        }
    }

    void s_add_edge(MOperand u, MOperand v) {
        auto p = Edge{ u, v };
        if(!in(p, adj_set) && !(u == v)) {
            // printf("interference: {");
            // print_operand(u);
            // printf(", ");
            // print_operand(v);
            // printf("}\n");
            adj_set.insert({ u, v });
            adj_set.insert({ v, u });

            if(u.tag == VFREG && (v.tag == FREG || v.tag == VFREG)) {
                adj_list[u].insert(v);
                degree[u]++;
            }

            if(v.tag == VFREG && (u.tag == FREG || u.tag == VFREG)) {
                adj_list[v].insert(u);
                degree[v]++;
            }
        }
    }

    void build(MFunc* func_asm) {
        int cnt = 0;
        for(auto b : func_asm->mBlocks) {
            int idx = b->id;
            auto live = liveOut[b->id];
            for(auto I = func_asm->mBlocks[b->id]->lastInst; I; I = I->prev) {
                auto defs = IGetDefs(I);
                auto uses = IGetUses(I, func_asm->hasReturnValue);
                if(I->tag == MI_MOVE) {
                    if(need_alloc(defs[0]) && uses.size() > 0 && need_alloc(uses[0])) {
                        for(auto use : uses) {
                            if(!need_alloc(use))
                                continue;
                            live.erase(use);
                            move_list[use].insert((MI_Move*)I);
                        }

                        for(auto def : defs) {
                            move_list[def].insert((MI_Move*)I);
                        }

                        worklist_moves.insert((MI_Move*)I);
                    }
                }

                // @Bug, needs to be allocated?
                for(auto def : defs) {
                    if(need_alloc(def)) {
                        if(b->loopDepth > loop_depth[def]) {
                            loop_depth[def] = b->loopDepth;
                        }
                        for(auto l : live)
                            add_edge(l, def);
                        live.insert(def);
                        use_def_count[def]++;
                    }
                }

                // @Optimize
                std::set<MOperand> new_live;
                for(auto u : uses) {
                    if(need_alloc(u)) {
                        use_def_count[u]++;
                        if(b->loopDepth > loop_depth[u]) {
                            loop_depth[u] = b->loopDepth;
                        }
                        new_live.insert(u);
                    }
                }
                // @TODO: record the inserted MOperand and insert
                for(auto l : live) {
                    // if (need_alloc(def) && l != def) {
                    if(defs.find(l) == -1) {
                        new_live.insert(l);
                    }
                }
                live = new_live;
            }
        }
    }

    void s_build(MFunc* func_asm) {
        int cnt = 0;
        for(auto b : func_asm->mBlocks) {
            int idx = b->id;
            auto live = liveOut[b->id];
            for(auto I = func_asm->mBlocks[b->id]->lastInst; I; I = I->prev) {
                auto defs = FGetDefs(I);
                auto uses = FGetUses(I, func_asm->hasReturnValue);
                if(I->tag == MI_FMOVE && defs.size() > 0 && uses.size() > 0) {
                    if(s_need_alloc(defs[0]) && s_need_alloc(uses[0])) {
                        for(auto use : uses) {
                            if(!s_need_alloc(use))
                                continue;
                            live.erase(use);
                            s_move_list[use].insert((MI_FMove*)I);
                        }

                        for(auto def : defs) {
                            s_move_list[def].insert((MI_FMove*)I);
                        }
                        s_worklist_moves.insert((MI_FMove*)I);
                    }
                }

                // @Bug, needs to be allocated?
                for(auto def : defs) {
                    if(s_need_alloc(def)) {
                        if(b->loopDepth > loop_depth[def]) {
                            loop_depth[def] = b->loopDepth;
                        }
                        for(auto l : live)
                            s_add_edge(l, def);
                        live.insert(def);
                        use_def_count[def]++;
                    }
                }

                // @Optimize
                std::set<MOperand> new_live;
                for(auto u : uses) {
                    if(s_need_alloc(u)) {
                        use_def_count[u]++;
                        if(b->loopDepth > loop_depth[u]) {
                            loop_depth[u] = b->loopDepth;
                        }
                        new_live.insert(u);
                    }
                }
                for(auto l : live) {
                    if(defs.find(l) == -1) {
                        new_live.insert(l);
                    }
                }
                live = new_live;
            }
        }
    }

    // return adjacent set of n
    std::set<MOperand> adjacent(MOperand n) {
        std::set<MOperand> a;
        auto it = adj_list[n].begin();
        while(it != adj_list[n].end()) {
            if(!in(*it, select_set) && !in(*it, coalesced_nodes)) {
                a.insert(*it);
            }
            it++;
        }
        return a;
    }

    // return MI_Move that related to n
    std::set<MI_Move*> node_moves(MOperand n) {
        std::set<MI_Move*> s;
        auto it = move_list[n].begin();
        while(it != move_list[n].end()) {
            if(in(*it, active_moves) || in(*it, worklist_moves)) {
                s.insert(*it);
            }
            it++;
        }
        return s;
    }

    // return MI_VMove that related to n
    std::set<MI_FMove*> s_node_moves(MOperand n) {
        std::set<MI_FMove*> s;
        auto it = s_move_list[n].begin();
        while(it != s_move_list[n].end()) {
            if(in(*it, s_active_moves) || in(*it, s_worklist_moves)) {
                s.insert(*it);
            }
            it++;
        }
        return s;
    }

    // is n related to an active MI_Move
    bool move_related(MOperand n) {
        return !node_moves(n).empty();
    }

    // is n related to an active MI_VMove
    bool s_move_related(MOperand n) {
        return !s_node_moves(n).empty();
    }

    // divide initial into spill_worklist, freeze_worklist and simplify_worklist
    void make_worklist() {
        while(!initial.empty()) {
            auto n = *(initial.begin());
            initial.erase(n);
            if(degree[n] >= K) {
                spill_worklist.insert(n);
            } else if(move_related(n)) {
                freeze_worklist.insert(n);
            } else {
                simplify_worklist.insert(n);
            }
        }
    }

    // divide initial into spill_worklist, freeze_worklist and simplify_worklist
    void s_make_worklist() {
        while(!initial.empty()) {
            auto n = *(initial.begin());
            initial.erase(n);
            if(degree[n] >= s_K) {
                spill_worklist.insert(n);
            } else if(s_move_related(n)) {
                freeze_worklist.insert(n);
            } else {
                simplify_worklist.insert(n);
            }
        }
    }

    // move active_moves of nodes to worklist_moves
    void enable_moves(std::set<MOperand> nodes) {
        for(auto n : nodes) {
            for(auto m : node_moves(n)) {
                if(in(m, active_moves)) {
                    active_moves.erase(m);
                    worklist_moves.insert(m);
                }
            }
        }
    }

    // move active_moves of nodes to worklist_moves
    void s_enable_moves(std::set<MOperand> nodes) {
        for(auto n : nodes) {
            for(auto m : s_node_moves(n)) {
                if(in(m, s_active_moves)) {
                    s_active_moves.erase(m);
                    s_worklist_moves.insert(m);
                }
            }
        }
    }

    // decrease degree and try to erase m from spill_worklist
    void decrement_degree(MOperand m) {
        int d = degree[m]--;
        if(d == K) {
            auto adj = adjacent(m);
            adj.insert(m);
            enable_moves(adj);
            spill_worklist.erase(m);
            if(move_related(m)) {
                freeze_worklist.insert(m);
            } else {
                simplify_worklist.insert(m);
            }
        }
    }

    // decrease degree and try to erase m from spill_worklist
    void s_decrement_degree(MOperand m) {
        int d = degree[m]--;
        if(d == s_K) {
            auto adj = adjacent(m);
            adj.insert(m);
            s_enable_moves(adj);
            spill_worklist.erase(m);
            if(s_move_related(m)) {
                freeze_worklist.insert(m);
            } else {
                simplify_worklist.insert(m);
            }
        }
    }

    // move the front operand from simplify_worklist to select_stack
    void simplify() {
        auto n = *(simplify_worklist.begin());
        simplify_worklist.erase(n);
        // printf("simplifying ");
        // print_operand(n);
        // printf("\n");
        select_stack.push(n);
        select_set.insert(n);
        for(auto m : adjacent(n)) {
            decrement_degree(m);
        }
    }

    // move the front operand from simplify_worklist to select_stack
    void s_simplify() {
        auto n = *(simplify_worklist.begin());
        simplify_worklist.erase(n);
        // printf("simplifying ");
        // print_operand(n);
        // printf("\n");
        select_stack.push(n);
        select_set.insert(n);
        for(auto m : adjacent(n)) {
            s_decrement_degree(m);
        }
    }

    // delete 'mv dst, src' and replace 'dst' with 'src'
    void coalesce() {
        auto m = *(worklist_moves.begin());
        worklist_moves.erase(m);
        // printf("coalescing ");
        // print_operand(m->dst);
        // printf(" <- ");
        // print_operand(m->src);
        // printf("\n");
        auto u = get_alias(m->src);
        auto v = get_alias(m->dst);
        auto e = Edge{ u, v };
        if(v.tag == IREG) {  // @What?
            auto t = u;
            u = v;
            v = t;
        }

        // @Performance
        int okok = false;
        for(auto t : adjacent(v)) {
            if(ok(t, u)) {
                okok = true;
                break;
            }
        }

        auto join = adjacent(u);
        for(auto n : adjacent(v)) {
            join.insert(n);
        }

        if(u == v) {
            coalesced_moves.insert(m);
            add_worklist(u);
        } else if(v.tag == IREG || in(e, adj_set)) {
            constrained_moves.insert(m);
            add_worklist(u);
            add_worklist(v);
        } else if((u.tag == IREG && okok) || (u.tag != IREG && conservative(join))) {
            coalesced_moves.insert(m);
            combine(u, v);
            add_worklist(u);
        } else {
            active_moves.insert(m);
        }
    }

    // delete 'mv dst, src' and replace 'dst' with 'src'
    void s_coalesce() {
        auto m = *(s_worklist_moves.begin());
        s_worklist_moves.erase(m);
        auto u = get_alias(m->src);
        auto v = get_alias(m->dst);
        auto e = Edge{ u, v };
        if(v.tag == FREG) {  // @What?
            auto t = u;
            u = v;
            v = t;
        }

        // @Performance
        int okok = false;
        for(auto t : adjacent(v)) {
            if(s_ok(t, u)) {
                okok = true;
                break;
            }
        }

        auto join = adjacent(u);
        for(auto n : adjacent(v)) {
            join.insert(n);
        }

        if(u == v) {
            s_coalesced_moves.insert(m);
            s_add_worklist(u);
        } else if(v.tag == FREG || in(e, adj_set)) {
            s_constrained_moves.insert(m);
            s_add_worklist(u);
            s_add_worklist(v);
        } else if((u.tag == FREG && okok) || (u.tag != FREG && s_conservative(join))) {
            s_coalesced_moves.insert(m);
            s_combine(u, v);
            s_add_worklist(u);
        } else {
            s_active_moves.insert(m);
        }
    }

    // try to add u from freeze_worklist to simplify_worklist
    void add_worklist(MOperand u) {
        if(u.tag != IREG && !move_related(u) && degree[u] < K) {
            freeze_worklist.erase(u);
            simplify_worklist.insert(u);
        }
    }

    // try to add u from freeze_worklist to simplify_worklist
    void s_add_worklist(MOperand u) {
        if(u.tag != FREG && !s_move_related(u) && degree[u] < s_K) {
            freeze_worklist.erase(u);
            simplify_worklist.insert(u);
        }
    }

    bool ok(MOperand t, MOperand r) {
        auto p = Edge{ t, r };
        return ((degree[t] < K) || (t.tag == IREG) || (in(p, adj_set)));
    }

    bool s_ok(MOperand t, MOperand r) {
        auto p = Edge{ t, r };
        return ((degree[t] < s_K) || (t.tag == FREG) || (in(p, adj_set)));
    }

    // return 'k < K' where k is the number of nodes that 'degree[n] >= K'
    bool conservative(std::set<MOperand> nodes) {
        int k = 0;
        for(auto n : nodes) {
            if(degree[n] >= K) {
                k++;
            }
        }
        return k < K;
    }

    // return 'k < K' where k is the number of nodes that 'degree[n] >= K'
    bool s_conservative(std::set<MOperand> nodes) {
        int k = 0;
        for(auto n : nodes) {
            if(degree[n] >= s_K) {
                k++;
            }
        }
        return k < s_K;
    }

    MOperand get_alias(MOperand n) {
        if(in(n, coalesced_nodes)) {
            return get_alias(alias[n]);
        } else {
            return n;
        }
    }

    // use 'u' to replace 'v'
    void combine(MOperand u, MOperand v) {
        if(in(v, freeze_worklist)) {
            freeze_worklist.erase(v);
        } else {
            spill_worklist.erase(v);
        }
        coalesced_nodes.insert(v);
        alias[v] = u;

        // FIXME
        for(auto n : move_list[v]) {
            move_list[u].insert(n);
        }

        for(auto t : adjacent(v)) {
            add_edge(t, u);
            decrement_degree(t);
        }

        if(degree[u] >= K && in(u, freeze_worklist)) {
            freeze_worklist.erase(u);
            spill_worklist.insert(u);
        }
    }

    // use 'u' to replace 'v'
    void s_combine(MOperand u, MOperand v) {
        if(in(v, freeze_worklist)) {
            freeze_worklist.erase(v);
        } else {
            spill_worklist.erase(v);
        }
        coalesced_nodes.insert(v);
        alias[v] = u;

        // FIXME
        for(auto n : s_move_list[v]) {
            s_move_list[u].insert(n);
        }

        for(auto t : adjacent(v)) {
            s_add_edge(t, u);
            s_decrement_degree(t);
        }

        if(degree[u] >= s_K && in(u, freeze_worklist)) {
            freeze_worklist.erase(u);
            spill_worklist.insert(u);
        }
    }

    // move the front operand of freeze_worklist to simplify_worklist
    void freeze() {
        auto u = *(freeze_worklist.begin());
        freeze_worklist.erase(u);
        simplify_worklist.insert(u);
        freeze_moves(u);
    }

    // move the front operand of freeze_worklist to simplify_worklist
    void s_freeze() {
        auto u = *(freeze_worklist.begin());
        freeze_worklist.erase(u);
        simplify_worklist.insert(u);
        s_freeze_moves(u);
    }

    void freeze_moves(MOperand u) {
        for(auto m : node_moves(u)) {
            // @Order?
            auto u = m->src;
            auto v = m->dst;

            if(in(m, active_moves)) {
                active_moves.erase(m);
            } else {
                worklist_moves.erase(m);
            }

            frozen_moves.insert(m);
            if(node_moves(v).empty() && degree[v] < K) {
                freeze_worklist.erase(v);
                simplify_worklist.insert(v);
            }
        }
    }

    void s_freeze_moves(MOperand u) {
        for(auto m : s_node_moves(u)) {
            // @Order?
            auto u = m->src;
            auto v = m->dst;

            if(in(m, s_active_moves)) {
                s_active_moves.erase(m);
            } else {
                s_worklist_moves.erase(m);
            }

            s_frozen_moves.insert(m);
            if(s_node_moves(v).empty() && degree[v] < s_K) {
                freeze_worklist.erase(v);
                simplify_worklist.insert(v);
            }
        }
    }

    void assign_colors() {
        while(!select_stack.empty()) {
            auto n = select_stack.back();
            select_stack.pop();
            Array<uint8> ok_colors;

            // assign caller save first
            for(uint8 r = a0; r <= a7; r++) {  // a0~a7
                ok_colors.push(r);
            }
            ok_colors.push(t0);  // t0~t2
            ok_colors.push(t1);
            ok_colors.push(t2);
            for(uint8 r = t3; r <= t6; r++) {  // t3~t6
                ok_colors.push(r);
            }

            // assign callee save regs
            ok_colors.push(s1);
            for(uint8 r = s2; r <= s9; r++) {  // s2~s9
                ok_colors.push(r);
            }
            ok_colors.push(s10);
            if(!need_to_reserve_s11)
                ok_colors.push(s11);
            ok_colors.push(fp);  // s0(fp)
            ok_colors.push(ra);

            for(auto w : adj_list[n]) {
                auto a = get_alias(w);
                if(in(a, colored_nodes) || in(a, precolored)) {
                    ok_colors.remove(color[a]);
                }
            }
            if(ok_colors.empty()) {
                spilled_nodes.insert(n);
            } else {
                colored_nodes.insert(n);
                auto c = ok_colors.front();
                color[n] = c;

                // record callee-save registers
                if(IIsCalleeSave(c))
                    if(needs_save.find(c) == -1)
                        needs_save.push(c);
            }
        }
        for(auto n : coalesced_nodes) {
            color[n] = color[get_alias(n)];
        }
    }

    void s_assign_colors() {
        while(!select_stack.empty()) {
            auto n = select_stack.back();
            select_stack.pop();
            Array<uint8> ok_colors;

            // assign caller save first
            for(uint8 r = f10; r <= f17; r++) {  // fa0~fa7
                ok_colors.push(r);
            }
            for(uint8 r = f0; r <= f7; r++) {  // ft0~ft7
                ok_colors.push(r);
            }
            for(uint8 r = f28; r <= f31; r++) {  // ft8~ft11
                ok_colors.push(r);
            }

            // assign callee save regs
            for(uint8 r = f8; r <= f9; r++) {  // fs0~fs1
                ok_colors.push(r);
            }
            for(uint8 r = f18; r <= f27; r++) {  // fs2~fs11
                ok_colors.push(r);
            }

            // remove adjcent colors(heuristic algorithm)
            for(auto w : adj_list[n]) {
                auto a = get_alias(w);
                if(in(a, colored_nodes) || in(a, precolored)) {
                    ok_colors.remove(s_color[a]);
                }
            }
            if(ok_colors.empty()) {
                spilled_nodes.insert(n);
            } else {
                colored_nodes.insert(n);
                auto c = ok_colors.front();
                s_color[n] = c;

                // record callee-save registers
                if(FIsCalleeSave(c))
                    if(s_needs_save.find(c) == -1)
                        s_needs_save.push(c);
            }
        }
        for(auto n : coalesced_nodes) {
            s_color[n] = s_color[get_alias(n)];
        }
    }

    int get_exp(int m, int n) {  // m^n
        if(n == 0) {
            return 1;
        }
        if(n == 1) {
            return m;
        }
        if(n % 2 == 0) {
            return get_exp(m * m, n / 2);
        } else {
            return m * get_exp(m * m, n / 2);
        }
    }

    void select_spill() {
        // printf("spilled!\n");
        // auto m = *(spill_worklist.begin());
        MOperand m = {};
        double min_cost = 1e40;
        int optimal_depth = 0;
        bool is_already_spilled = false;
        std::set<MOperand> spilled;
        for(auto it = spill_worklist.begin(); it != spill_worklist.end(); it++) {
            // when regs already spilled, erase from spill_worklist
            if(in(*it, already_spilled)) {
                is_already_spilled = true;
                spilled.insert(*it);
                continue;
            }

            assert(degree[*it] != 0);

            int cur_depth = loop_depth[*it];
            double cost = use_def_count[*it] * 100 / degree[*it];
            bool selected = false;
            if(optimal_depth == cur_depth) {
                selected = cost < min_cost;
            } else if(optimal_depth < cur_depth) {
                if(get_exp(4, cur_depth - optimal_depth) < (min_cost / cost)) {
                    selected = true;
                }
            } else {
                if(get_exp(4, optimal_depth - cur_depth) > (cost / min_cost)) {
                    selected = true;
                }
            }

            // double cost = use_def_count[*it] * pow(10, loop_depth[*it]) / degree[*it];
            // double cost = pow(10, loop_depth[*it]) / degree[*it];
            // print_operand(*it);
            // printf(": use_def_count: %d, loop_depth: %d, degree: %d, cost: %f\n",
            //        use_def_count[*it], loop_depth[*it], degree[*it], cost);

            if(selected) {
                m = *it;
                min_cost = cost;
                optimal_depth = cur_depth;
            }
        }

        if(m.tag == ERRORTYPE && is_already_spilled) {
            m = *spilled.begin();
        }
        assert(m.tag != ERRORTYPE);

        spill_worklist.erase(m);
        simplify_worklist.insert(m);
        freeze_moves(m);
    }

    void s_select_spill() {
        // printf("spilled!\n");
        // auto m = *(spill_worklist.begin());
        MOperand m = {};
        double min_cost = 1e40;
        bool is_already_spilled = false;
        std::set<MOperand> spilled;
        for(auto it = spill_worklist.begin(); it != spill_worklist.end(); it++) {

            // when regs already spilled, erase from spill_worklist
            if(in(*it, already_spilled)) {
                is_already_spilled = true;
                spilled.insert(*it);
                continue;
            }

            assert(degree[*it] != 0);

            double cost = use_def_count[*it] * pow(10, loop_depth[*it]) / degree[*it];
            // double cost = pow(10, loop_depth[*it]) / degree[*it];
            // print_operand(*it);
            // printf(": use_def_count: %d, loop_depth: %d, degree: %d, cost: %f\n",
            //        use_def_count[*it], loop_depth[*it], degree[*it], cost);

            if(cost < min_cost) {
                m = *it;
                min_cost = cost;
            }
        }

        if(m.tag == ERRORTYPE && is_already_spilled) {
            m = *spilled.begin();
        }

        assert(m.tag != ERRORTYPE);

        spill_worklist.erase(m);
        simplify_worklist.insert(m);
        s_freeze_moves(m);
    }

    void color_register(MOperand& m) {
        if(m.tag == VIREG) {
            if(color.count(m) == 0) {
                printf("operand haven't assign color: ");
                // print_operand(m);
                printf("\n");
                fflush(stdout);
            }
            assert(color.find(m) != color.end());
            uint8 reg = color[m];
            m.value = reg;
            m.tag = IREG;
        }

        // if (m.tag == REG && isCalleeSave(m.value))
        //     if (needs_save.find(m.value) == -1)
        //         needs_save.push(m.value);
    }

    void s_color_register(MOperand& m) {
        if(m.tag == VFREG) {
            assert(s_color.count(m) > 0);
            uint8 reg = s_color[m];
            m.value = reg;
            m.tag = FREG;
        }

        // if (m.tag == SREG && sIsCalleeSave(m.value))
        //     if (s_needs_save.find(m.value) == -1)
        //         s_needs_save.push(m.value);
    }

    void allocate_register(MFunc* func_asm) {
        clear_all();

        printf(">>> Analysing liveness of function %s\n", func_asm->name.c_str());
        fflush(stdout);
        analyseLiveness(func_asm);
        printf(">>> Finished analysing liveness of function %s...\n\n", func_asm->name.c_str());
        fflush(stdout);

        // print_function_asm(func_asm);
        if(func_asm->vIRegCount >= 60000) {
            need_to_reserve_s11 = true;
        }

        // @Check: precolored register?
        for(int i = 0; i < REG_COUNT; i++) {
            auto r = makeIReg(i);
            precolored.insert(r);
            color[r] = i;
        }

        for(int i = 0; i < func_asm->vIRegCount; i++) {
            initial.insert(makeVIReg(i));
        }
        printf("function %s vreg_count: %d, vsreg_count: %d\n", func_asm->name.c_str(), func_asm->vIRegCount,
               func_asm->vFRegCount);

        printf(">>> Building interference graph for coloring...\n");
        build(func_asm);
        printf(">>> done building\n");
        fflush(stdout);
        make_worklist();
        printf(">>> worklist done\n");
        fflush(stdout);
        do {
            if(!simplify_worklist.empty())
                simplify();
            else if(!worklist_moves.empty())
                coalesce();
            else if(!freeze_worklist.empty())
                freeze();
            else if(!spill_worklist.empty())
                select_spill();
        } while(!simplify_worklist.empty() || !worklist_moves.empty() || !freeze_worklist.empty() || !spill_worklist.empty());

        printf("assigning\n");
        assign_colors();
        printf("assigned\n");

        printf("\n");
        if(!spilled_nodes.empty()) {
            // insert stores and restores for each spilled node
            for(MOperand n : spilled_nodes) {
                already_spilled.insert(n);

                // printf("spilling nodes: ");
                // print_operand(n);
                // printf("\n");
                spilled_stack_size += 8;
                bool is_defined = false;

                for(auto bb : func_asm->mBlocks) {
                    for(auto I = bb->firInst; I; I = I->next) {
                        auto defs = IGetDefs(I);
                        auto uses = IGetUses(I, func_asm->hasReturnValue);

                        bool inserted_store = false;
                        bool inserted_use = false;
                        // insert store after def
                        if(defs.find(n) != -1) {
                            // avoid load -> store
                            // if(I->tag == MI_LOAD && spill_store_offset.count(n)) {
                            //     auto load = (MI_Load *)I;
                            //     if(load->memTag == MEM_LOAD_SPILL) {
                            //         I->mark();
                            //         marked = true;
                            //         continue;
                            //     }
                            // }

                            auto str = new MI_Store(n, makeIReg(sp), makeImm(-(spilled_stack_size)));
                            // str->reg = makeVreg(func_asm->vreg_count++);
                            // @IMPORTANT: do not allocate new reg
                            str->memTag = MEM_SAVE_SPILL;
                            str->isRv64 = true;
                            // already_spilled.insert(str->reg);

                            // replaceDefs(I, &n, &str->reg, func_asm);
                            str->insertBefore(I->next);  // @Last?
                            I = I->next;                // jump past newly inserted str
                            inserted_store = true;
                            is_defined = true;
                            spill_store_offset[n] = -(spilled_stack_size);
                        }

                        // insert load before use
                        if(uses.find(n) != -1) {
                            // avoid load -> store
                            if(I->tag == MI_STORE && spill_load_offset.count(n)) {
                                auto store = (MI_Store*)I;
                                if(store->memTag == MEM_SAVE_SPILL) {
                                    I->deleteFromParent(func_asm);
                                    continue;
                                }
                            }

                            // @IMPORTANT: do not allocate new reg
                            // ldr->reg = n;
                            auto ldrReg = makeVIReg(func_asm->vIRegCount++);
                            auto ldrBase = makeIReg(sp);
                            auto ldrOffs = makeImm(-(spilled_stack_size));
                            auto ldr = new MI_Load(ldrReg, ldrBase, ldrOffs);
                            ldr->memTag = MEM_LOAD_SPILL;
                            ldr->isRv64 = true;
                            already_spilled.insert(ldr->reg);
                            I->IReplaceUses(&n, &ldr->reg, func_asm);
                            // @TODO: get uses based on arg count
                            ldr->insertBefore(I);
                            inserted_use = true;
                            spill_load_offset[ldr->reg] = -(spilled_stack_size);
                        }
                        assert(!(inserted_use && inserted_store));
                    }
                }
                if(!is_defined) {
                    fprintf(stderr, "error: vr%ld is not defined\n", n.value);
                    fflush(stderr);
                }
                assert(is_defined);
            }
            func_asm->stackSize += spilled_stack_size;
            func_asm->IRegSpillSize += spilled_stack_size;

            printf(">>> spilled stack size: %d\n", spilled_stack_size);
            printf(">>> after spilling: \n");
            printf("\n");
        }
    }

    void s_allocate_register(MFunc* func_asm) {
        clear_all();

        s_analyse_liveness(func_asm);
        // @Check: precolored register?
        for(int i = 0; i < SREG_COUNT; i++) {
            auto r = makeFReg(i);
            precolored.insert(r);
            s_color[r] = i;
        }

        for(int i = 0; i < func_asm->vFRegCount; i++) {
            initial.insert(makeVFReg(i));
        }

        // print_function_asm(func_asm);

        printf(">>> Building interference graph for coloring...\n");
        fflush(stdout);
        s_build(func_asm);
        printf(">>> done building\n");
        fflush(stdout);
        s_make_worklist();
        printf(">>> worklist done\n");
        fflush(stdout);
        do {
            if(!simplify_worklist.empty())
                s_simplify();
            else if(!s_worklist_moves.empty())
                s_coalesce();
            else if(!freeze_worklist.empty())
                s_freeze();
            else if(!spill_worklist.empty())
                s_select_spill();
        } while(!simplify_worklist.empty() || !s_worklist_moves.empty() || !freeze_worklist.empty() || !spill_worklist.empty());

        printf("assigning\n");
        fflush(stdout);
        s_assign_colors();
        printf("assigned\n");
        fflush(stdout);

        printf("\n");
        if(!spilled_nodes.empty()) {
            // insert stores and restores for each spilled node
            for(MOperand n : spilled_nodes) {
                already_spilled.insert(n);

                // printf("spilling nodes: ");
                // print_operand(n);
                // printf("\n");
                spilled_stack_size += 8;  // Alignment
                bool is_defined = false;
                for(auto bb : func_asm->mBlocks) {
                    
                    for(auto I = bb->firInst; I; I = I->next) {
                        auto defs = FGetDefs(I);
                        auto uses = FGetUses(I, func_asm->hasReturnValue);

                        bool inserted_store = false;
                        bool inserted_use = false;
                        // insert store after def
                        if(defs.find(n) != -1) {
                            // avoid load -> store(call args load, spilled load...)
                            // if(I->tag == MI_VLOAD && spill_store_offset.count(n)) {
                            //     auto load = (MI_VLoad *)I;
                            //     if(load->memTag == MEM_LOAD_SPILL) {
                            //         I->mark();
                            //         marked = true;
                            //         continue;
                            //     }
                            // }

                            // str->reg = makeVsreg(func_asm->vsreg_count++);
                            auto strReg = n;
                            auto strBase = makeIReg(sp);
                            auto strOffs = makeImm(-(spilled_stack_size));
                            auto str = new MI_FStore(strReg, strBase, strOffs);
                            str->memTag = MEM_SAVE_SPILL;
                            str->isRv64 = true;
                            already_spilled.insert(str->reg);

                            // sReplaceDefs(I, &n, &str->reg, func_asm);
                            str->insertBefore(I->next);  // @Last?
                            I = I->next;                // jump past newly inserted str
                            inserted_store = true;
                            is_defined = true;
                            spill_store_offset[n] = -(spilled_stack_size);
                        }

                        // insert restore before use
                        if(uses.find(n) != -1) {
                            // avoid load -> store
                            if(I->tag == MI_FSTORE && spill_load_offset.count(n)) {
                                auto store = (MI_FStore*)I;
                                if(store->memTag == MEM_SAVE_SPILL) {
                                    I->deleteFromParent(func_asm);
                                    continue;
                                }
                            }

                            auto ldrReg = makeVFReg(func_asm->vFRegCount++);
                            auto ldrBase = makeIReg(sp);
                            auto ldrOffs = makeImm(-(spilled_stack_size));
                            auto ldr = new MI_FLoad(ldrReg, ldrBase, ldrOffs);
                            ldr->memTag = MEM_LOAD_SPILL;
                            ldr->isRv64 = true;
                            already_spilled.insert(ldr->reg);
                            I->FReplaceUses(&n, &ldr->reg, func_asm);
                            // @TODO: get uses based on arg count
                            ldr->insertBefore(I);
                            inserted_use = true;
                            spill_load_offset[ldr->reg] = -(spilled_stack_size);
                        }
                        assert(!(inserted_use && inserted_store));
                    }
                }
                assert(is_defined);
            }

            func_asm->stackSize += spilled_stack_size;
            func_asm->FRegSpillSize += spilled_stack_size;

            printf(">>> spilled stack size: %d\n", spilled_stack_size);
            printf(">>> after spilling: \n");
            // print_function_asm(func_asm);
            printf("\n");
        }
    }

    void color_register_all(MFunc* func_asm) {
        // done, replace all vreg with actual regs
        for(auto b : func_asm->mBlocks) {
            for(auto I = func_asm->mBlocks[b->id]->firInst; I; I = I->next) {
                switch(I->tag) {

                    case MI_CLZ: {
                        color_register(((MI_Clz*)I)->dst);
                        color_register(((MI_Clz*)I)->operand);
                    } break;

                    case MI_ZEXT: {
                        color_register(((MI_Zext*)I)->dst);
                        color_register(((MI_Zext*)I)->src);
                    } break;

                    case MI_MOVE: {
                        color_register(((MI_Move*)I)->dst);
                        color_register(((MI_Move*)I)->src);
                    } break;

                    case MI_BINARY: {
                        color_register(((MI_Binary*)I)->dst);
                        color_register(((MI_Binary*)I)->lhs);
                        color_register(((MI_Binary*)I)->rhs);
                    } break;

                    case MI_COMPARE: {
                        color_register(((MI_Compare*)I)->lhs);
                        color_register(((MI_Compare*)I)->rhs);
                    } break;

                    case MI_SLT: {
                        color_register(((MI_Slt*)I)->dst);
                        color_register(((MI_Slt*)I)->lhs);
                        color_register(((MI_Slt*)I)->rhs);
                    } break;

                    case MI_LOAD:
                    case MI_STORE: {
                        color_register(((MI_Load*)I)->reg);
                        color_register(((MI_Load*)I)->base);
                        color_register(((MI_Load*)I)->offset);
                    } break;

                    case MI_BRANCH: {
                        if(((MI_Branch*)I)->cond != BR_JU) {
                            color_register(((MI_Branch*)I)->lhs);
                            color_register(((MI_Branch*)I)->rhs);
                        }
                    } break;

                    case MI_RETURN: {
                        auto return_addr = makeIReg(ra);
                        color_register(return_addr);
                    } break;

                    // ----------floating point operation----------
                    case MI_FLOAD:
                    case MI_FSTORE: {
                        auto vmv = ((MI_FLoad*)I);
                        if(vmv->base.tag == IREG || vmv->base.tag == VIREG) {
                            color_register(vmv->base);
                        }
                        if(vmv->offset.tag == IREG || vmv->offset.tag == VIREG) {
                            color_register(vmv->offset);
                        }
                    } break;

                    case MI_FCMP: {
                        color_register(((MI_FCmp*)I)->dst);
                    } break;
                    case MI_FCVT: {
                        auto fcvt = (MI_FCvt*)I;
                        if(fcvt->fromType == S32) {
                            color_register(fcvt->src);
                        }
                        if(fcvt->toType == S32) {
                            color_register(fcvt->dst);
                        }
                    } break;

                    case MI_FMOVE: {
                        if(((MI_FMove*)I)->fromS32) {
                            color_register(((MI_FMove*)I)->src);
                        }
                    } break;
                    case MI_FBINARY:
                    case MI_FPUSH:
                    case MI_FPOP:
                    case MI_FUNC_CALL:
                    case MI_PUSH:  // we don't push/pop virtual regs, for now
                    case MI_POP:
                        break;

                    default:
                        assert(false && "unknown MInst in allocate_register");
                }
            }
        }

        // printf(">>> Allocated registers:\n");
        // for (auto it = color.begin(); it != color.end(); it++)
        // {
        //     print_operand(it->first);
        //     printf(" <- ");
        //     print_operand(makeReg(it->second));
        //     printf("\n");
        // }
        // printf("\n");
    }

    void color_sregister_all(MFunc* func_asm) {
        // done, replace all vreg with actual regs
        for(auto b : func_asm->mBlocks) {
            for(auto I = func_asm->mBlocks[b->id]->firInst; I; I = I->next) {
                switch(I->tag) {

                    case MI_CLZ: {
                        s_color_register(((MI_Clz*)I)->dst);
                        s_color_register(((MI_Clz*)I)->operand);
                    } break;
                    case MI_ZEXT: {
                        s_color_register(((MI_Zext*)I)->dst);
                        s_color_register(((MI_Zext*)I)->dst);
                    } break;
                    case MI_FMOVE: {
                        s_color_register(((MI_FMove*)I)->dst);
                        if(!((MI_FMove*)I)->fromS32) {
                            s_color_register(((MI_FMove*)I)->src);
                        }
                    } break;

                    case MI_FBINARY: {
                        s_color_register(((MI_FBinary*)I)->dst);
                        s_color_register(((MI_FBinary*)I)->lhs);
                        s_color_register(((MI_FBinary*)I)->rhs);
                    } break;

                    case MI_FCMP: {
                        s_color_register(((MI_FCmp*)I)->lhs);
                        s_color_register(((MI_FCmp*)I)->rhs);
                    } break;

                    case MI_FLOAD:
                    case MI_FSTORE: {
                        s_color_register(((MI_FLoad*)I)->reg);
                        // s_color_register(((MI_Load *) I)->base);
                        // s_color_register(((MI_Load *) I)->offset);
                    } break;
                    case MI_FCVT: {
                        auto fcvt = (MI_FCvt*)I;
                        if(fcvt->fromType == F32) {
                            s_color_register(fcvt->src);
                        }
                        if(fcvt->toType == F32) {
                            s_color_register(fcvt->dst);
                        }
                    }

                    case MI_FPUSH:
                    case MI_FPOP:

                    case MI_RETURN:
                    case MI_FUNC_CALL:
                    case MI_PUSH:  // we don't push/pop virtual regs, for now
                    case MI_POP:
                    case MI_BRANCH:
                    case MI_MOVE:
                    case MI_BINARY:
                    case MI_COMPARE:
                    case MI_SLT:
                    case MI_LOAD:
                    case MI_STORE:
                        break;

                    default:
                        assert(false && "unknown MInst in s_allocate_register");
                }
            }
        }

        // printf(">>> Allocated registers:\n");
        // for (auto it = color.begin(); it != color.end(); it++)
        // {
        //     print_operand(it->first);
        //     printf(" <- ");
        //     print_operand(makeSreg(it->second));
        //     printf("\n");
        // }
        // printf("\n");
    }

    // recompute all offset related to sp
    void recompute_offset(MFunc* func_asm) {
        // int tmp_stack_size = func_asm->stack_size;

        // // convert fp to sp
        // // now func_asm->stack_size = size of (local arrays + spilled vars + max calling arguments);
        // // not including the callee-save registers' space(push/pop will automatically modify sp)
        // // allocate stack space for argument passing
        // int stack_arg_size = 0;
        // for (auto mb : func_asm->mbs) {
        //     for (auto I = mb->inst; I; I = I->next) {
        //         if (I->tag == MI_FUNC_CALL) {
        //             auto call = (MI_Func_Call *)I;
        //             if (call->argStackSize > stack_arg_size) {
        //                 stack_arg_size = call->argStackSize;
        //             }
        //         }
        //     }
        // }
        // if(stack_arg_size > 0)
        //     tmp_stack_size += stack_arg_size;

        // // align to 8B before stack allocation
        // int save_stack_size = (func_asm->regNeedsSave + func_asm->sregNeedsSave) * 8;
        // if ((tmp_stack_size + save_stack_size) % 16 != 0) {
        //     tmp_stack_size += 16 - ((tmp_stack_size + save_stack_size) % 16);
        // }

        // // we just compute the actual offset of each store instruction of spilled node,
        // // if illegal, use a vreg to load the constant
        // int all_spill_stack_size = func_asm->regSpillSize + func_asm->sregSpillSize;
        // for (auto bb : func_asm->mbs)
        // {
        //     for (auto I = bb->inst; I; I = I->next)
        //     {
        //         if (I->tag == MI_STORE)
        //         {
        //             auto str = (MI_Store *)I;
        //             if (str->memTag == MEM_SAVE_SPILL)
        //             {
        //                 assert(str->offset.tag != ERRORTYPE);
        //                 int32 offset_value = ((str->offset.tag == ERRORTYPE) ? 0 : str->offset.value);
        //                 int32 offset_relative_to_sp = all_spill_stack_size + stack_arg_size + offset_value;
        //             }
        //             // if store offset is overflow, need to legalize imm
        //             if(str->offset.tag == IMM && !canBeImm12(str->offset.value)) {
        //                 auto new_offset = makeVreg(func_asm->vreg_count++);
        //                 emitMove(new_offset, makeImm(str->offset.value), str->mb, str);
        //                 str->offset = new_offset;
        //                 need_to_legalize_imm = true;
        //             }
        //         }
        //         else if (I->tag == MI_VSTORE)
        //         {
        //             auto str = (MI_VStore *)I;
        //             if (str->memTag == MEM_SAVE_SPILL)
        //             {
        //                 assert(str->offset.tag != ERRORTYPE);
        //                 int32 offset_value = ((str->offset.tag == ERRORTYPE) ? 0 : str->offset.value);
        //                 int32 offset_relative_to_sp = all_spill_stack_size - func_asm->regSpillSize + stack_arg_size +
        //                 offset_value;
        //             }
        //             // if store offset is overflow, need to legalize imm
        //             if(str->offset.tag == IMM && !canBeImm12(str->offset.value)) {
        //                 auto new_offset = makeVreg(func_asm->vreg_count++);
        //                 emitMove(new_offset, makeImm(str->offset.value), str->mb, str);
        //                 str->offset = new_offset;
        //                 need_to_legalize_imm = true;
        //             }
        //         }
        //     }
        // }

        // if(need_to_legalize_imm)
        //     return;

        // now we begin to recompute and replace offset
        int stack_arg_size = 0;
        for(auto mb : func_asm->mBlocks) {
            for(auto I = mb->firInst; I; I = I->next) {
                if(I->tag == MI_FUNC_CALL) {
                    auto call = (MI_Func_Call*)I;
                    if(call->argStackSize > stack_arg_size) {
                        stack_arg_size = call->argStackSize;
                    }
                }
            }
        }
        if(stack_arg_size > 0)
            func_asm->stackSize += stack_arg_size;

        // align to 8B before stack allocation
        int save_stack_size = (func_asm->IRegNeedsSave + func_asm->FRegNeedsSave) * 8;
        if((func_asm->stackSize + save_stack_size) % 16 != 0) {
            func_asm->stackSize += 16 - ((func_asm->stackSize + save_stack_size) % 16);
        }

        // fixup sp when calculating base address for local arrays
        for(auto base : func_asm->localArrayBases) {
            if(base->mb == nullptr)
                continue;
            assert(base->tag == MI_BINARY);
            auto sub = (MI_Binary*)base;
            assert(sub->op == BINARY_SUB);
            assert(sub->lhs == makeIReg(sp));
            assert(sub->rhs.tag == IMM);

            int offset_relative_to_sp = func_asm->stackSize - sub->rhs.value;
            auto offset = makeImm(offset_relative_to_sp);
            // if array base offset is overflow, use array base to load constant
            if(!canBeImm12(offset_relative_to_sp)) {
                emitMove(sub->dst, makeImm(offset_relative_to_sp), sub->mb, sub);
                offset = sub->dst;
            }
            auto array_offset = new MI_Binary(BINARY_ADD, sub->dst, makeIReg(sp), offset);
            array_offset->isRv64 = true;
            array_offset->insertBefore(sub);
            sub->removeFromParent();
        }

        int all_spill_stack_size = func_asm->IRegSpillSize + func_asm->FRegSpillSize;
        for(auto bb : func_asm->mBlocks) {
            for(auto I = bb->firInst; I; I = I->next) {
                // int save_stack_size = 0;
                if(I->tag == MI_LOAD) {
                    auto ldr = (MI_Load*)I;
                    if(ldr->memTag == MEM_LOAD_SPILL) {
                        int32 offset_value = ((ldr->offset.tag == ERRORTYPE) ? 0 : ldr->offset.value);
                        int32 offset_relative_to_sp = all_spill_stack_size + stack_arg_size + offset_value;
                        ldr->base.value = sp;
                        ldr->offset = makeImm(offset_relative_to_sp);
                    } else if(ldr->memTag == MEM_LOAD_ARG) {
                        int32 offset_value = ((ldr->offset.tag == ERRORTYPE) ? 0 : ldr->offset.value);
                        int32 offset_relative_to_sp = func_asm->stackSize + offset_value + save_stack_size;
                        ldr->base.value = sp;
                        ldr->offset = makeImm(offset_relative_to_sp);
                    }
                    // if load offset is overflow, use load->reg to load constant
                    if(ldr->offset.tag == IMM && !canBeImm12(ldr->offset.value)) {
                        emitMove(ldr->reg, makeImm(ldr->offset.value), ldr->mb, ldr);
                        auto cal_new_base = new MI_Binary(BINARY_ADD, ldr->reg, ldr->base, ldr->reg);
                        cal_new_base->isRv64 = true;
                        cal_new_base->insertBefore(ldr);
                        ldr->offset = makeImm(0);
                        ldr->base = ldr->reg;
                    }
                } else if(I->tag == MI_STORE) {
                    auto str = (MI_Store*)I;
                    if(str->memTag == MEM_SAVE_SPILL) {
                        int32 offset_value = ((str->offset.tag == ERRORTYPE) ? 0 : str->offset.value);
                        int32 offset_relative_to_sp = all_spill_stack_size + stack_arg_size + offset_value;
                        str->base.value = sp;
                        str->offset = makeImm(offset_relative_to_sp);
                    }
                    // if store offset is overflow, need to legalize imm
                    if(str->offset.tag == IMM && !canBeImm12(str->offset.value)) {
                        // assert(need_to_reserve_s11);
                        auto new_offset = makeIReg(s11);
                        emitMove(new_offset, makeImm(str->offset.value), str->mb, str);
                        auto cal_new_base = new MI_Binary(BINARY_ADD, new_offset, str->base, new_offset);
                        cal_new_base->isRv64 = true;
                        cal_new_base->insertBefore(str);
                        str->base = cal_new_base->dst;
                        str->offset = makeImm(0);
                    }
                } else if(I->tag == MI_FLOAD) {
                    auto ldr = (MI_FLoad*)I;
                    if(ldr->memTag == MEM_LOAD_SPILL) {
                        int32 offset_value = ((ldr->offset.tag == ERRORTYPE) ? 0 : ldr->offset.value);
                        int32 offset_relative_to_sp =
                            all_spill_stack_size - func_asm->IRegSpillSize + stack_arg_size + offset_value;
                        ldr->base.value = sp;
                        ldr->offset = makeImm(offset_relative_to_sp);
                    } else if(ldr->memTag == MEM_LOAD_ARG) {
                        int32 offset_value = ((ldr->offset.tag == ERRORTYPE) ? 0 : ldr->offset.value);
                        int32 offset_relative_to_sp = func_asm->stackSize + offset_value + save_stack_size;
                        ldr->base.value = sp;
                        ldr->offset = makeImm(offset_relative_to_sp);
                    }
                    // if load offset is overflow, use load->reg to load constant
                    if(ldr->offset.tag == IMM && !canBeImm12(ldr->offset.value)) {
                        emitMove(ldr->reg, makeImm(ldr->offset.value), ldr->mb, ldr);
                        auto cal_new_base = new MI_Binary(BINARY_ADD, ldr->reg, ldr->base, ldr->reg);
                        cal_new_base->isRv64 = true;
                        cal_new_base->insertBefore(ldr);
                        ldr->offset = makeImm(0);
                        ldr->base = ldr->reg;
                    }
                } else if(I->tag == MI_FSTORE) {
                    auto str = (MI_FStore*)I;
                    if(str->memTag == MEM_SAVE_SPILL) {
                        int32 offset_value = ((str->offset.tag == ERRORTYPE) ? 0 : str->offset.value);
                        int32 offset_relative_to_sp =
                            all_spill_stack_size - func_asm->IRegSpillSize + stack_arg_size + offset_value;
                        str->base.value = sp;
                        str->offset = makeImm(offset_relative_to_sp);
                    }
                    // if store offset is overflow, need to legalize imm
                    if(str->offset.tag == IMM && !canBeImm12(str->offset.value)) {
                        assert(need_to_reserve_s11);
                        auto new_offset = makeIReg(s11);
                        emitMove(new_offset, makeImm(str->offset.value), str->mb, str);
                        auto cal_new_base = new MI_Binary(BINARY_ADD, new_offset, str->base, new_offset);
                        cal_new_base->isRv64 = true;
                        cal_new_base->insertBefore(str);
                        str->base = cal_new_base->dst;
                        str->offset = makeImm(0);
                    }
                }
            }
        }
    }

    // save callee-save registers(push and pop)
    void save_registers(MFunc* func_asm) {
        if(needs_save.find(ra) == -1)
            needs_save.push(ra);
        func_asm->IRegNeedsSave = needs_save.size();
        printf(">>> registers that need to save: ");
        for(auto r : needs_save) {
            // print_operand(makeReg(r));
            printf(" ");
        }
        printf("\n");
        // insert push/pops to save these registers
        if(!needs_save.empty()) {
            // insert push in the start block
            std::sort(needs_save.begin(), needs_save.end());
            Array<MOperand> toStore;
            for(auto r : needs_save) {
                toStore.push(makeIReg(r));
            }
            auto store = new MI_Push(toStore);
            store->insertBefore(func_asm->mBlocks[0]->firInst);

            // restore registers at exit

            for(auto bb : func_asm->mBlocks) {
                if(bb->lastInst && bb->lastInst->tag == MI_RETURN) {
                    Array<MOperand> toRestore;
                    for(auto r : needs_save) {
                        toRestore.push(makeIReg(r));
                    }
                    auto restore = new MI_Pop(toRestore);
                    restore->insertBefore(bb->lastInst);
                }
            }
        }
    }

    // save callee-save registers(push and pop)
    void save_sregisters(MFunc* func_asm) {
        func_asm->FRegNeedsSave = s_needs_save.size();
        printf(">>> sregisters that need to save: ");
        for(auto r : s_needs_save) {
            // print_operand(makeSreg(r));
            printf(" ");
        }
        printf("\n");

        // insert push/pops to save these registers
        // s_needs_save are recorded in s_color_register
        if(!s_needs_save.empty()) {
            // insert push in the start block
            std::sort(s_needs_save.begin(), s_needs_save.end());
            vector<MI_FPush*> store_list;
            vector<MI_FPop*> restore_list;
            Array<MOperand> toStore;
            Array<MOperand> toRestore;
            int i = 0;
            for(i = 0; i < s_needs_save.size(); i++) {
                toStore.push(makeFReg(s_needs_save[i]));
                toRestore.push(makeFReg(s_needs_save[i]));
            }
            auto store = new MI_FPush(toStore);
            auto restore = new MI_FPop(toRestore);
            store_list.push_back(store);
            restore_list.push_back(restore);

            for(int j = 0; j < store_list.size(); j++) {
                store_list[j]->insertBefore(func_asm->mBlocks[0]->firInst);
            }
            // restore registers at exit
            for(auto bb : func_asm->mBlocks) {
                if(bb->lastInst && bb->lastInst->tag == MI_RETURN) {
                    for(int j = 0; j < restore_list.size(); j++) {
                        restore_list[j]->insertBefore(bb->lastInst);
                    }
                }
            }
        }
    }

    int nearest_imm12(int x) {
        if(x <= -2048)
            return -2048;
        else if(x >= 2047)
            return 2040;
        else
            return x;
    }

    // allocate stack based on spilled size, callee-save reg size and call size
    void allocate_stack(MFunc* func_asm) {
        // insert add/sub to set up or destroy the stack
        // if stack is too large, allocate it for several times
        if(func_asm->stackSize != 0) {
            int cur_stack_size = func_asm->stackSize;
            MI_Binary *alloc_stack, *destroy_stack;
            auto offset_opr = makeImm(cur_stack_size);
            bool is_overflow = false;

            // at the start and end of a function, 't0' is empty
            if(!canBeImm12(cur_stack_size)) {
                is_overflow = true;
                offset_opr = makeIReg(t0);
            }
            alloc_stack = new MI_Binary(BINARY_SUB, makeIReg(sp), makeIReg(sp), offset_opr);
            destroy_stack = new MI_Binary(BINARY_ADD, makeIReg(sp), makeIReg(sp), offset_opr);
            alloc_stack->isRv64 = true;
            destroy_stack->isRv64 = true;

            // allocate stack after saving callee-save regs
            auto start_inst = func_asm->mBlocks[0]->firInst;
            while(start_inst != nullptr && (start_inst->tag == MI_PUSH || start_inst->tag == MI_FPUSH)) {
                start_inst = start_inst->next;
            }
            assert(start_inst != nullptr);
            if(is_overflow)
                emitMove(makeIReg(t0), makeImm(cur_stack_size), start_inst->mb, start_inst);
            alloc_stack->insertBefore(start_inst);

            // destroy stack before restoring callee-save regs
            for(auto bb : func_asm->mBlocks) {
                if(bb->lastInst && bb->lastInst->tag == MI_RETURN) {
                    auto last_inst = bb->lastInst;
                    while(last_inst != nullptr && last_inst->prev != nullptr &&
                          (last_inst->prev->tag == MI_POP || last_inst->prev->tag == MI_FPOP)) {
                        last_inst = last_inst->prev;
                    }
                    if(is_overflow)
                        emitMove(makeIReg(t0), makeImm(cur_stack_size), last_inst->mb, last_inst);
                    destroy_stack->insertBefore(last_inst);
                }
            }
        }
    }

    void clear_all() {
        precolored.clear();
        initial.clear();
        simplify_worklist.clear();
        freeze_worklist.clear();
        spill_worklist.clear();
        spilled_nodes.clear();
        coalesced_nodes.clear();
        colored_nodes.clear();
        coalesced_moves.clear();
        s_coalesced_moves.clear();
        constrained_moves.clear();
        s_constrained_moves.clear();
        frozen_moves.clear();
        s_frozen_moves.clear();
        select_set.clear();
        worklist_moves.clear();
        s_worklist_moves.clear();
        active_moves.clear();
        s_active_moves.clear();
        adj_set.clear();
        adj_list.clear();
        degree.clear();
        move_list.clear();
        s_move_list.clear();
        alias.clear();
        use_def_count.clear();
        loop_depth.clear();

        select_stack.len = 0;
    }

    void clear_before_allocation() {
        color.clear();
        s_color.clear();
        needs_save.len = 0;
        s_needs_save.len = 0;
        need_to_legalize_imm = false;
    }

    void allocate_single_function(MFunc* func_asm, bool opt) {
        spilled_stack_size = 0;
        done_post_work = false;
        already_spilled.clear();
        spill_load_offset.clear();
        spill_store_offset.clear();
        do {
            allocate_register(func_asm);
        } while(!spilled_nodes.empty());
    }

    void s_allocate_single_function(MFunc* func_asm, bool opt) {
        spilled_stack_size = 0;
        done_post_work = false;
        already_spilled.clear();
        spill_load_offset.clear();
        spill_store_offset.clear();
        do {
            s_allocate_register(func_asm);
        } while(!spilled_nodes.empty());
    }
}  // namespace RA

void registerAllocation(MProg* program_asm, bool opt) {
    printf(">>> Allocating registers...\n\n");
    fflush(stdout);
    for(auto f : program_asm->mFuncs) {
        do {
            RA::clear_before_allocation();
            printf(">>> Allocating registers for function %s\n", f->name.c_str());
            fflush(stdout);
            RA::allocate_single_function(f, opt);

            printf(">>> Allocating sregisters for function %s\n", f->name.c_str());
            fflush(stdout);
            RA::s_allocate_single_function(f, opt);

            RA::save_registers(f);
            RA::save_sregisters(f);
            printf(">>> Saved registers for function %s\n", f->name.c_str());
            fflush(stdout);
            RA::recompute_offset(f);
            printf(">>> Recompte_offset for function %s\n", f->name.c_str());
            fflush(stdout);
        } while(RA::need_to_legalize_imm);

        RA::allocate_stack(f);
        printf(">>> Allocated stack for function %s\n", f->name.c_str());
        fflush(stdout);

        RA::color_register_all(f);
        RA::color_sregister_all(f);
        printf(">>> Colored for function %s\n", f->name.c_str());
        fflush(stdout);
    }
    printf("------------------------------------------");
}

void sregister_allocation(MProg* program_asm, bool opt) {
    // printf(">>> Allocating sregisters...\n\n");
    // for (auto f : program_asm->functions)
    // {

    // }
}
// NOLINTEND
